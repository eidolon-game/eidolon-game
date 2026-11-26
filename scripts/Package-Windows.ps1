param(
    [string]$EngineRoot,
    [switch]$Launch
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
& (Join-Path $PSScriptRoot 'Build-Editor.ps1') -EngineRoot $EngineRoot
$MapScript = Join-Path $PSScriptRoot 'create_arena_map.py'
& $Cmd $Project '-run=pythonscript' "-script=$MapScript" -unattended -nop4 -nullrhi -nosplash -stdout -FullStdOutLogOutput
if ($LASTEXITCODE -ne 0) { throw "Map generation failed: $LASTEXITCODE" }
if (!(Test-Path (Join-Path $Root 'Content\Eidolon\Maps\Arena.umap'))) { throw 'Arena map not generated.' }
$Output = Join-Path $Root 'Saved\Distribution'
& $UAT BuildCookRun "-project=$Project" -noP4 -platform=Win64 -clientconfig=Development -build -cook '-map=/Game/Eidolon/Maps/Arena' -stage -pak -package -archive "-archivedirectory=$Output" -unattended -utf8output
if ($LASTEXITCODE -ne 0) { throw "Packaging failed: $LASTEXITCODE" }
$Exe = Join-Path $Output 'Windows\Eidolon.exe'
if (!(Test-Path $Exe)) { throw "Expected executable missing: $Exe" }
Write-Host "Windows demo: $Exe"
if ($Launch) { Start-Process -FilePath $Exe -WorkingDirectory (Split-Path $Exe) }
