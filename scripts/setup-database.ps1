param([string]$AdminRole = 'postgres')
$ErrorActionPreference = 'Stop'
$psql = 'C:\Program Files\PostgreSQL\18\bin\psql.exe'
if (!(Test-Path -LiteralPath $psql)) { throw 'PostgreSQL 18 psql was not found.' }
Write-Host 'Local setup: only habit_tracker_tdd, habit_tracker_tdd_test and habit_tracker_tdd_app.'
Write-Host 'psql will request the administrator password and then the application role password twice.'
Write-Host 'Enter passwords only in this console. They are not saved in project files.'
& $psql -X -W -h localhost -p 5432 -U $AdminRole -d postgres -v ON_ERROR_STOP=1 -f (Join-Path $PSScriptRoot 'setup-database.sql')
if ($LASTEXITCODE -ne 0) { throw 'Database setup failed. Do not send passwords to chat.' }
