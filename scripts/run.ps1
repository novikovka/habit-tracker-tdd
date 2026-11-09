param([switch]$SkipBuild)
$ErrorActionPreference = 'Stop'
if (!$SkipBuild) { & (Join-Path $PSScriptRoot 'build.ps1') }
$executable = Join-Path (Split-Path $PSScriptRoot -Parent) 'build\ascii\habit_tracker.exe'
if (!(Test-Path -LiteralPath $executable)) { throw 'Build the application first.' }
$secret = Read-Host 'PostgreSQL password for habit_tracker_tdd_app (local hidden input)' -AsSecureString
$pointer = [IntPtr]::Zero
try {
    $pointer = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($secret)
    $info = New-Object System.Diagnostics.ProcessStartInfo
    $info.FileName = $executable
    $info.WorkingDirectory = Split-Path $executable -Parent
    $info.UseShellExecute = $false
    $info.EnvironmentVariables['PATH'] = 'D:\Qt\6.9.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;C:\Program Files\PostgreSQL\18\bin;' + $env:PATH
    $info.EnvironmentVariables['HABIT_TRACKER_DB_PASSWORD'] = [Runtime.InteropServices.Marshal]::PtrToStringBSTR($pointer)
    $process = [Diagnostics.Process]::Start($info)
    $info.EnvironmentVariables.Remove('HABIT_TRACKER_DB_PASSWORD')
    $process.WaitForExit()
} finally {
    if ($pointer -ne [IntPtr]::Zero) { [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($pointer) }
    $secret.Dispose()
}
