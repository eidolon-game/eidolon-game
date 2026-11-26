param(
    [Parameter(Mandatory = $true)][string]$EngineRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if (-not $IsWindows) { throw 'Run this script in PowerShell 7 on Windows.' }

$root = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
$project = Join-Path $root 'Eidolon.uproject'
$engine = (Resolve-Path -LiteralPath $EngineRoot).Path
$version = Get-Content -LiteralPath (Join-Path $engine 'Engine/Build/Build.version') -Raw | ConvertFrom-Json
if ($version.MajorVersion -ne 5 -or $version.MinorVersion -ne 6) {
    throw 'An authorized Unreal Engine 5.6 installation is required.'
}
$editor = Join-Path $engine 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$requiredEngineFiles = @(
    $editor
    (Join-Path $engine 'Engine/Binaries/Win64/UnrealEditor.exe')
    (Join-Path $engine 'Engine/Build/BatchFiles/Build.bat')
    (Join-Path $engine 'Engine/Build/BatchFiles/RunUAT.bat')
)
foreach ($file in $requiredEngineFiles) {
    if (-not (Test-Path -LiteralPath $file -PathType Leaf)) { throw "Missing engine tool: $file" }
}

$commit = git -C $root rev-parse HEAD
if ($LASTEXITCODE -ne 0) { throw 'Unable to determine the source commit.' }
$status = git -C $root status --porcelain --untracked-files=all
if ($LASTEXITCODE -ne 0 -or $status) { throw 'Use a clean source checkout.' }
if ($env:GITHUB_SHA -and $env:GITHUB_SHA -ne $commit) {
    throw 'The checkout does not match the dispatched source commit.'
}

$evidence = Join-Path $root 'Saved/StudioWin64'
$distribution = Join-Path $root 'Saved/Distribution'
$arenaMap = Join-Path $root 'Content/Eidolon/Maps/Arena.umap'
$buildOutputs = @($evidence, $distribution, (Join-Path $root 'Binaries'), (Join-Path $root 'Intermediate'))
foreach ($path in $buildOutputs) {
    if (Test-Path -LiteralPath $path) { throw "Use a fresh checkout. Existing build output: $path" }
}
$arenaTracked = git -C $root ls-files -- 'Content/Eidolon/Maps/Arena.umap'
if ($LASTEXITCODE -ne 0 -or ((Test-Path -LiteralPath $arenaMap) -and -not $arenaTracked)) {
    throw 'Unable to establish Arena provenance. Use a fresh checkout.'
}
$diagnostics = Join-Path $evidence 'diagnostics'
New-Item -ItemType Directory -Path $diagnostics | Out-Null

& (Join-Path $root 'scripts/Build-Editor.ps1') -EngineRoot $engine 2>&1 |
    Tee-Object -FilePath (Join-Path $diagnostics 'compile.log')
$report = Join-Path $diagnostics 'automation'
$automationLog = Join-Path $diagnostics 'automation.log'
& $editor $project '-ExecCmds=Automation RunTests Eidolon' '-TestExit=Automation Test Queue Empty' `
    "-ReportExportPath=$report" "-abslog=$automationLog" -unattended -nop4 -nullrhi -nosplash -stdout -FullStdOutLogOutput
if ($LASTEXITCODE -ne 0) { throw 'Unreal Editor automation exited unsuccessfully.' }

$results = Get-Content -LiteralPath (Join-Path $report 'index.json') -Raw | ConvertFrom-Json
$tests = @($results.tests)
$campaignTests = @($tests | Where-Object { $_.fullTestPath -eq 'Eidolon.Campaign.Rules' })
if ($campaignTests.Count -ne 1 -or @($tests | Where-Object { $_.state -ne 'Success' }).Count -ne 0) {
    throw 'Automation must include Eidolon.Campaign.Rules and every reported test must succeed.'
}

& (Join-Path $root 'scripts/Package-Windows.ps1') -EngineRoot $engine 2>&1 |
    Tee-Object -FilePath (Join-Path $diagnostics 'package.log')
$windows = Join-Path $distribution 'Windows'
$executable = Get-Item -LiteralPath (Join-Path $windows 'Eidolon.exe')
if ($executable.PSIsContainer -or $executable.Length -lt 2) { throw 'The Win64 executable is missing or empty.' }
$binary = $executable.OpenRead()
try {
    if ($binary.ReadByte() -ne 0x4D -or $binary.ReadByte() -ne 0x5A) {
        throw 'The Win64 executable is missing the Windows executable signature.'
    }
} finally {
    $binary.Dispose()
}

$delivery = Join-Path $evidence 'delivery'
New-Item -ItemType Directory -Path $delivery | Out-Null
$archiveName = "Eidolon-Arena-Win64-$commit.zip"
$archive = Join-Path $delivery $archiveName
[System.IO.Compression.ZipFile]::CreateFromDirectory(
    $windows, $archive, [System.IO.Compression.CompressionLevel]::Optimal, $false
)
[ordered]@{
    commit = $commit
    engine = $version
    platform = 'Win64'
    configuration = 'Development'
    map = '/Game/Eidolon/Maps/Arena'
    automationTestsPassed = $tests.Count
    archive = $archiveName
    sha256 = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash
    playtest = 'not run'
} | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $delivery 'manifest.json')
Write-Host "Built and packaged the Win64 Development Arena at $commit. Playtest pending."
