$ErrorActionPreference = 'Stop'
foreach ($suite in @('Logic', 'Database', 'Ui')) {
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'test.ps1') -Suite $suite -EvidenceName "final-$($suite.ToLower())"
    if ($LASTEXITCODE -ne 0) { throw "Suite $suite failed." }
}
