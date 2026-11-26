param(
    [Parameter(Mandatory=$true)][string]$EngineRoot,
    [switch]$Launch
)
$ErrorActionPreference = 'Stop'
$Project = (Resolve-Path (Join-Path $PSScriptRoot '..\Eidolon.uproject')).Path
$Build = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
if (!(Test-Path $Build) -or !(Test-Path $Editor)) {
    throw 'EngineRoot must point to a complete Windows Unreal Engine installation.'
}
& $Build EidolonEditor Win64 Development "-Project=$Project" -WaitMutex
if ($LASTEXITCODE -ne 0) { throw "Unreal build failed: $LASTEXITCODE" }
if ($Launch) { Start-Process -FilePath $Editor -ArgumentList ('"' + $Project + '"') }
