param(
    [Parameter(Mandatory=$true)][string]$EngineRoot
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$project = Join-Path $root 'Eidolon.uproject'
$version = Get-Content (Join-Path $EngineRoot 'Engine/Build/Build.version') -Raw | ConvertFrom-Json
if ($version.MajorVersion -ne 5 -or $version.MinorVersion -ne 6) { throw 'UE 5.6 required' }
$editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
if (!(Test-Path $editor)) { throw 'Unreal editor unavailable' }
$sha = & git -C $root rev-parse HEAD
if ($LASTEXITCODE -ne 0) { throw 'Git checkout required' }
$dirty = & git -C $root status --porcelain
if ($LASTEXITCODE -ne 0 -or $dirty) { throw 'Clean checkout required' }
$evidence = Join-Path $root 'Saved/BuildEvidence'
$output = Join-Path $root 'Saved/Distribution'
foreach ($path in @($evidence, $output)) {
    if (Test-Path $path) { Remove-Item $path -Recurse -Force }
}
New-Item -ItemType Directory -Path $evidence | Out-Null
& (Join-Path $root 'scripts/Build-Editor.ps1') -EngineRoot $EngineRoot 2>&1 | Tee-Object (Join-Path $evidence 'compile.log')
$report = Join-Path $evidence 'automation'
& $editor $project '-ExecCmds=Automation RunTests Eidolon' '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$report" -unattended -nop4 -nullrhi -nosplash -stdout -FullStdOutLogOutput 2>&1 | Tee-Object (Join-Path $evidence 'automation.log')
if ($LASTEXITCODE -ne 0) { throw 'Unreal automation failed' }
$results = Get-Content (Join-Path $report 'index.json') -Raw | ConvertFrom-Json
$tests = @($results.tests)
if (!$tests.Count -or !($tests | Where-Object { $_.fullTestPath -eq 'Eidolon.Campaign.Rules' }) -or @($tests | Where-Object { $_.state -ne 'Success' }).Count) { throw 'Missing or unsuccessful Unreal tests' }
& (Join-Path $root 'scripts/Package-Windows.ps1') -EngineRoot $EngineRoot 2>&1 | Tee-Object (Join-Path $evidence 'package.log')
$windows = Join-Path $output 'Windows'
$exe = Join-Path $windows 'Eidolon.exe'
if (!(Test-Path $exe -PathType Leaf) -or (Get-Item $exe).Length -eq 0) { throw 'Executable missing' }
$zip = Join-Path $evidence "Eidolon-Win64-$sha.zip"
Compress-Archive -Path (Join-Path $windows '*') -DestinationPath $zip
@{commit=$sha; engine=$version; tests=$tests.Count; archive=(Split-Path $zip -Leaf); sha256=(Get-FileHash $zip -Algorithm SHA256).Hash; playtest='NOT RUN'} | ConvertTo-Json -Depth 8 | Set-Content (Join-Path $evidence 'manifest.json')
Write-Host 'Compilation, automation and packaging evidence collected. Manual playtest still required.'
