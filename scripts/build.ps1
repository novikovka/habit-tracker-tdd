param([ValidatePattern('^[A-Za-z]:$')][string]$BuildDrive = 'H:')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$oldPath = $env:PATH
$mapped = $false
try {
    if (Test-Path "$BuildDrive\") { throw "Drive $BuildDrive is occupied. Choose another -BuildDrive." }
    & subst.exe $BuildDrive $projectRoot
    if ($LASTEXITCODE -ne 0 -or !(Test-Path "$BuildDrive\")) { throw 'Cannot map build drive.' }
    $mapped = $true
    $env:PATH = 'D:\Qt\Tools\mingw1310_64\bin;D:\Qt\6.9.1\mingw_64\bin;' + $oldPath
    $cmake = 'D:\Qt\Tools\CMake_64\bin\cmake.exe'
    & $cmake -S "$BuildDrive/" -B "$BuildDrive/build/ascii" -G Ninja -DCMAKE_MAKE_PROGRAM=D:/Qt/Tools/Ninja/ninja.exe -DCMAKE_CXX_COMPILER=D:/Qt/Tools/mingw1310_64/bin/g++.exe -DCMAKE_PREFIX_PATH=D:/Qt/6.9.1/mingw_64
    if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
    & $cmake --build "$BuildDrive/build/ascii"
    if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
} finally {
    $env:PATH = $oldPath
    if ($mapped) { & subst.exe $BuildDrive /D }
}
