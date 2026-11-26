param(
    [string]$EngineRoot,
    [switch]$Launch,
    [switch]$Campaign
)
$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Project = Join-Path $Root 'Eidolon.uproject'
if (!$EngineRoot) {
    $engines = @(Get-ChildItem 'C:\Program Files\Epic Games' -Directory -ErrorAction SilentlyContinue | Where-Object { Test-Path (Join-Path $_.FullName 'Engine\Build\BatchFiles\RunUAT.bat') })
    if ($engines.Count -ne 1) { throw 'Specify -EngineRoot with the installed UE5 directory.' }
    $EngineRoot = $engines[0].FullName
}
$Cmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$UAT = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
if (!(Test-Path $Cmd) -or !(Test-Path $UAT)) { throw 'Unreal Engine installation not found.' }
$Output = Join-Path $Root 'Saved\Distribution'
if ($Campaign) {
    $BuildVersion = Join-Path $EngineRoot 'Engine\Build\Build.version'
    $Version = Get-Content -LiteralPath $BuildVersion -Raw -Encoding UTF8 | ConvertFrom-Json
    if ($Version.MajorVersion -ne 5 -or $Version.MinorVersion -ne 6) {
        throw 'Campaign prototype packaging requires Unreal Engine 5.6.'
    }

    $Output = Join-Path $Root 'Saved\DistributionCampaignPrototype'
    $ManifestPath = Join-Path $Root 'Saved\EidolonAuthoredMaps.json'
    foreach ($Path in @($Output, $ManifestPath)) {
        if (Test-Path -LiteralPath $Path) {
            throw "Existing campaign prototype output. Move it aside before packaging: $Path"
        }
    }

    $MapsDirectory = Join-Path $Root 'Content\Eidolon\Maps'
    if (Test-Path -LiteralPath $MapsDirectory -PathType Container) {
        foreach ($ExistingMap in (Get-ChildItem -LiteralPath $MapsDirectory -Filter '*.umap' -File -Force)) {
            if ($ExistingMap.BaseName -match '\A(ValdrisPrologue|AbyssFloor[0-9]+)\z') {
                throw "Use a fresh checkout to generate campaign maps. Existing package: $($ExistingMap.FullName)"
            }
        }
    }
}
& (Join-Path $PSScriptRoot 'Build-Editor.ps1') -EngineRoot $EngineRoot

if ($Campaign) {
    $MapScript = Join-Path $PSScriptRoot 'create_authored_campaign_maps.py'
    & $Cmd $Project '-run=pythonscript' "-script=$MapScript" -unattended -nop4 -nullrhi -nosplash -stdout -FullStdOutLogOutput
    if ($LASTEXITCODE -ne 0) { throw "Campaign map generation failed: $LASTEXITCODE" }
    if (!(Test-Path -LiteralPath $ManifestPath -PathType Leaf)) {
        throw "Campaign map generation did not write a fresh manifest: $ManifestPath"
    }

    $Manifest = Get-Content -LiteralPath $ManifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
    $StartupMap = '/Game/Eidolon/Maps/ValdrisPrologue'
    if ($Manifest.startup_map -isnot [string] -or $Manifest.startup_map -cne $StartupMap -or
        $Manifest.maps -isnot [array] -or
        $Manifest.maps.Count -eq 0 -or $Manifest.maps[0] -cne $StartupMap) {
        throw 'The campaign manifest must list ValdrisPrologue first and identify it as the startup map.'
    }

    $MapNames = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
    foreach ($Map in $Manifest.maps) {
        if ($Map -isnot [string] -or
            $Map -cnotmatch '\A/Game/Eidolon/Maps/(ValdrisPrologue|AbyssFloor(?:00[1-9]|0[1-9][0-9]|100))\z' -or
            !$MapNames.Add($Map)) {
            throw 'The campaign manifest contains an invalid or duplicate package name.'
        }
        $RelativeMapFile = 'Content/' + $Map.Substring('/Game/'.Length) + '.umap'
        $MapPath = Join-Path $Root $RelativeMapFile
        if (!(Test-Path -LiteralPath $MapPath -PathType Leaf) -or (Get-Item -LiteralPath $MapPath).Length -eq 0) {
            throw "Campaign map package is missing or empty: $Map"
        }
    }
    $MapArgument = '-map=' + ($Manifest.maps -join '+')
} else {
    $MapScript = Join-Path $PSScriptRoot 'create_arena_map.py'
    & $Cmd $Project '-run=pythonscript' "-script=$MapScript" -unattended -nop4 -nullrhi -nosplash -stdout -FullStdOutLogOutput
    if ($LASTEXITCODE -ne 0) { throw "Map generation failed: $LASTEXITCODE" }
    if (!(Test-Path (Join-Path $Root 'Content\Eidolon\Maps\Arena.umap'))) { throw 'Arena map not generated.' }
    $MapArgument = '-map=/Game/Eidolon/Maps/Arena'
}

& $UAT BuildCookRun "-project=$Project" -noP4 -platform=Win64 -clientconfig=Development -build -cook $MapArgument -stage -pak -package -archive "-archivedirectory=$Output" -unattended -utf8output
if ($LASTEXITCODE -ne 0) { throw "Packaging failed: $LASTEXITCODE" }
$Exe = Join-Path $Output 'Windows\Eidolon.exe'
if (!(Test-Path $Exe)) { throw "Expected executable missing: $Exe" }

if ($Campaign) {
    $Launcher = Join-Path (Split-Path $Exe) 'Launch-Campaign-Prototype.bat'
    @(
        '@echo off'
        'setlocal'
        'pushd "%~dp0" || exit /b 1'
        "Eidolon.exe $StartupMap -EidolonAuthoredCampaign"
        'set "EIDOLON_EXIT=%ERRORLEVEL%"'
        'popd'
        'exit /b %EIDOLON_EXIT%'
    ) | Set-Content -LiteralPath $Launcher -Encoding ASCII
    Write-Host "Windows authored campaign prototype: $Launcher"
    if ($Launch) {
        Start-Process -FilePath $Exe -ArgumentList @($StartupMap, '-EidolonAuthoredCampaign') -WorkingDirectory (Split-Path $Exe)
    }
} else {
    Write-Host "Windows demo: $Exe"
    if ($Launch) { Start-Process -FilePath $Exe -WorkingDirectory (Split-Path $Exe) }
}
