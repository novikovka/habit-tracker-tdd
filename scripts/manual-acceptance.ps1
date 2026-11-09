param([switch]$SetupDatabase)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
if ($SetupDatabase) {
    & (Join-Path $PSScriptRoot 'setup-database.ps1')
}
$executable = Join-Path $projectRoot 'build\ascii\habit_tracker.exe'
if (!(Test-Path -LiteralPath $executable)) {
    & (Join-Path $PSScriptRoot 'build.ps1')
}
$profileRoot = Join-Path $projectRoot 'build\manual-acceptance-profile'
$configDirectory = Join-Path $profileRoot 'HabitTrackerTDD'
New-Item -ItemType Directory -Path $configDirectory -Force | Out-Null
@(
    'host=localhost'
    'port=5432'
    'database=habit_tracker_tdd_test'
    'user=habit_tracker_tdd_app'
) | Set-Content -LiteralPath (Join-Path $configDirectory 'connection.ini') -Encoding ASCII
$previousLocalAppData = $env:LOCALAPPDATA
try {
    $env:LOCALAPPDATA = $profileRoot
    Write-Host ''
    Write-Host 'Manual acceptance uses habit_tracker_tdd_test on localhost:5432.'
    Write-Host 'Existing database data is preserved. Your normal Windows app profile is unchanged.'
    Write-Host 'Launch 1: create habits, mark today/yesterday, rename a habit, then close the app.'
    & (Join-Path $PSScriptRoot 'run.ps1') -SkipBuild
    Write-Host ''
    Write-Host 'Launch 2: enter the SAME application role password; verify names, marks and statistics.'
    & (Join-Path $PSScriptRoot 'run.ps1') -SkipBuild
} finally {
    $env:LOCALAPPDATA = $previousLocalAppData
}
Write-Host ''
Write-Host 'Both application processes have ended. Report observed results in chat; never send passwords.'
