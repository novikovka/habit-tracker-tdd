param(
    [ValidatePattern('^[A-Za-z]:$')][string]$BuildDrive = 'H:',
    [ValidatePattern('^[a-zA-Z0-9-]+$')][string]$EvidenceName,
    [switch]$ExpectFailure,
    [ValidateSet('Logic','Database','Ui')][string]$Suite = 'Logic',
    [switch]$Capture,
    [ValidateSet('1','1.25','1.5')][string]$ScaleFactor = '1'
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$oldPath = $env:PATH
$mapped = $false
$testDatabase = $null
$oldDbHost = $env:HABIT_TRACKER_TDD_DB_HOST
$oldDbPort = $env:HABIT_TRACKER_TDD_DB_PORT
$oldDbPassword = $env:HABIT_TRACKER_TDD_DB_PASSWORD
$oldPlatform = $env:QT_QPA_PLATFORM
$oldFontDirectory = $env:QT_QPA_FONTDIR
$oldCapture = $env:HABIT_TRACKER_CAPTURE_DIR
$oldScaleFactor = $env:QT_SCALE_FACTOR
try {
    if (Test-Path "$BuildDrive\") { throw "Drive $BuildDrive is occupied. Choose another -BuildDrive." }
    & subst.exe $BuildDrive $projectRoot
    if ($LASTEXITCODE -ne 0 -or !(Test-Path "$BuildDrive\")) { throw 'Cannot map build drive.' }
    $mapped = $true
    $env:PATH = 'D:\Qt\Tools\mingw1310_64\bin;D:\Qt\6.9.1\mingw_64\bin;C:\Program Files\PostgreSQL\18\bin;' + $oldPath
    $cmake = 'D:\Qt\Tools\CMake_64\bin\cmake.exe'
    $build = "$BuildDrive/build/ascii"
    & $cmake -S "$BuildDrive/" -B $build -G Ninja -DCMAKE_MAKE_PROGRAM=D:/Qt/Tools/Ninja/ninja.exe -DCMAKE_CXX_COMPILER=D:/Qt/Tools/mingw1310_64/bin/g++.exe -DCMAKE_PREFIX_PATH=D:/Qt/6.9.1/mingw_64
    if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
    & $cmake --build $build
    if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
    if ($Suite -ne 'Logic') {
        . (Join-Path $PSScriptRoot 'test-database.ps1')
        $testDatabase = Start-IsolatedTestDatabase $BuildDrive
        $env:HABIT_TRACKER_TDD_DB_HOST = '127.0.0.1'
        $env:HABIT_TRACKER_TDD_DB_PORT = [string]$testDatabase.Port
        $env:HABIT_TRACKER_TDD_DB_PASSWORD = $testDatabase.Secret
    }
    if ($Suite -eq 'Ui') {
        $env:QT_QPA_PLATFORM = 'offscreen'
        $env:QT_QPA_FONTDIR = 'C:\Windows\Fonts'
        $env:QT_SCALE_FACTOR = $ScaleFactor
        if ($Capture) { $env:HABIT_TRACKER_CAPTURE_DIR = "$BuildDrive/docs/screenshots" }
    }
    $testStarted = [DateTimeOffset]::UtcNow
    $output = if ($EvidenceName) { "$BuildDrive/docs/evidence/$EvidenceName.txt" } else { "$build/latest-tests.txt" }
    $executable = switch ($Suite) { 'Database' { 'database_tests' }; 'Ui' { 'window_tests' }; default { 'logic_tests' } }
    & "$build/$executable.exe" -o "$output,txt"
    $testExit = $LASTEXITCODE
    $testFinished = [DateTimeOffset]::UtcNow
    Get-Content $output -Encoding UTF8
    if ($EvidenceName) {
        [ordered]@{ startedUtc = $testStarted.ToString('o'); finishedUtc = $testFinished.ToString('o'); exitCode = $testExit; expectedFailure = [bool]$ExpectFailure } |
            ConvertTo-Json | Set-Content "$BuildDrive/docs/evidence/$EvidenceName.json" -Encoding UTF8
    }
    if ($ExpectFailure) {
        if ($testExit -eq 0 -or !(Select-String -LiteralPath $output -Pattern '^FAIL!')) { throw 'Expected an assertion failure, but did not get one.' }
        return
    }
    if ($testExit -ne 0) { throw "Qt Test failed: $testExit" }
    $filter = switch ($Suite) { 'Database' { 'habit_database' }; 'Ui' { 'habit_window' }; default { 'habit_logic' } }
    & 'D:\Qt\Tools\CMake_64\bin\ctest.exe' --test-dir $build --output-on-failure -R "^$filter`$"
    if ($LASTEXITCODE -ne 0) { throw 'CTest failed.' }
} finally {
    $env:HABIT_TRACKER_TDD_DB_HOST = $oldDbHost
    $env:HABIT_TRACKER_TDD_DB_PORT = $oldDbPort
    $env:HABIT_TRACKER_TDD_DB_PASSWORD = $oldDbPassword
    $env:QT_QPA_PLATFORM = $oldPlatform
    $env:QT_QPA_FONTDIR = $oldFontDirectory
    $env:HABIT_TRACKER_CAPTURE_DIR = $oldCapture
    $env:QT_SCALE_FACTOR = $oldScaleFactor
    if ($testDatabase) { Stop-IsolatedTestDatabase $testDatabase }
    $env:PATH = $oldPath
    if ($mapped) { & subst.exe $BuildDrive /D }
}
