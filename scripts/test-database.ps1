# Infrastructure for disposable PostgreSQL tests. Never connects to the user's server.
function Invoke-PrivateInputProcess([string]$Executable, [string]$Arguments, [string]$InputText) {
    $info = New-Object System.Diagnostics.ProcessStartInfo
    $info.FileName = $Executable
    $info.Arguments = $Arguments
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardInput = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $info
    [void]$process.Start()
    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()
    $process.StandardInput.WriteLine($InputText)
    $process.StandardInput.Close()
    $process.WaitForExit()
    [void]$stdout.Result
    $diagnostic = $stderr.Result
    if ($process.ExitCode -ne 0) {
        $safeDiagnostic = $diagnostic.Replace($InputText, '[private input]').Replace($InputText.Trim(), '[private input]') -replace '[a-fA-F0-9]{64}', '[secret]'
        throw "Private database setup failed ($Executable): $safeDiagnostic"
    }
}

function Start-IsolatedTestDatabase([string]$Drive) {
    $bin = 'C:\Program Files\PostgreSQL\18\bin'
    $base = "$Drive/build/pg-tests"
    New-Item -ItemType Directory -Path $base -Force | Out-Null
    $data = "$base/$([Guid]::NewGuid().ToString('N'))"
    $secret = [Guid]::NewGuid().ToString('N') + [Guid]::NewGuid().ToString('N')
    # initdb on Windows does not accept '-' as a password input file.
    # Bootstrap on loopback only, then switch to SCRAM before any tests run.
    Invoke-PrivateInputProcess "$bin/initdb.exe" "-D $data -U postgres -A trust --encoding=UTF8 --locale=C" ''
    $listener = New-Object System.Net.Sockets.TcpListener([System.Net.IPAddress]::Loopback, 0)
    $listener.Start()
    $port = $listener.LocalEndpoint.Port
    $listener.Stop()
    $startInfo = New-Object System.Diagnostics.ProcessStartInfo
    $startInfo.FileName = "$bin/pg_ctl.exe"
    $startInfo.Arguments = "-D $data -l $data/server.log -o `"-h 127.0.0.1 -p $port -F`" -w start"
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $control = New-Object System.Diagnostics.Process
    $control.StartInfo = $startInfo
    [void]$control.Start()
    if (!$control.WaitForExit(15000) -or $control.ExitCode -ne 0) { throw 'Isolated PostgreSQL could not start.' }
    $control.Dispose()
    $oldPgPassword = $env:PGPASSWORD
    try {
        $env:PGPASSWORD = $secret
        $sql = "ALTER ROLE postgres PASSWORD '$secret';`nCREATE ROLE habit_tracker_tdd_app LOGIN PASSWORD '$secret';`nCREATE DATABASE habit_tracker_tdd_test OWNER habit_tracker_tdd_app;"
        Invoke-PrivateInputProcess "$bin/psql.exe" "-X -w -h 127.0.0.1 -p $port -U postgres -d postgres -v ON_ERROR_STOP=1" $sql
        Set-Content "$data/pg_hba.conf" 'host all all 127.0.0.1/32 scram-sha-256' -Encoding ASCII
        & "$bin/pg_ctl.exe" -D $data reload | Out-Null
        if ($LASTEXITCODE -ne 0) { throw 'Could not enable SCRAM for disposable tests.' }
    } catch {
        & "$bin/pg_ctl.exe" -D $data -m immediate -w stop | Out-Null
        throw
    } finally { $env:PGPASSWORD = $oldPgPassword }
    return @{ Data = $data; Base = $base; Port = $port; Secret = $secret }
}

function Stop-IsolatedTestDatabase($Database) {
    & 'C:\Program Files\PostgreSQL\18\bin\pg_ctl.exe' -D $Database.Data -m fast -w stop | Out-Null
    if ($LASTEXITCODE -ne 0) { throw 'Could not stop disposable PostgreSQL; its files were preserved.' }
    $target = [IO.Path]::GetFullPath($Database.Data)
    $allowed = [IO.Path]::GetFullPath($Database.Base).TrimEnd('\') + '\'
    if (!$target.StartsWith($allowed, [StringComparison]::OrdinalIgnoreCase)) { throw 'Invalid disposable database cleanup path.' }
    Remove-Item -LiteralPath $target -Recurse -Force
}
