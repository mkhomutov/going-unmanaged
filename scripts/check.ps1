# Build and run YOUR exercise attempt with the handbook's canonical flags - MSVC.
#
#   scripts\check.ps1 <your.cpp> [fakesdk|fakedevice|comlab] [args passed to the run...]
#
#   scripts\check.ps1 attempt.cpp                     # plain exercise
#   scripts\check.ps1 attempt.cpp fakesdk             # + vendor code
#   $env:STD='c++20'; scripts\check.ps1 attempt.cpp   # C++20 (words, invalid)
#
# Run it from a Developer PowerShell for VS, so cl.exe is on PATH (Chapter 13).
# This mirrors scripts/check.sh; the differences are MSVC's, not the script's:
# /fsanitize=address only - MSVC has no UBSan (Chapter 31) - and no
# ThreadSanitizer at all, so Chapter 29's TSan step needs clang or gcc (WSL
# is the usual Windows answer). CI smoke-tests this script in the
# buildlab-msvc job so it cannot rot unnoticed.
param(
    [Parameter(Mandatory = $true)][string]$Source,
    [Parameter(ValueFromRemainingArguments = $true)][string[]]$Rest
)
$ErrorActionPreference = 'Stop'

if (-not (Get-Command cl -ErrorAction SilentlyContinue)) {
    Write-Error "cl.exe not found - run this from a Developer PowerShell for VS (Chapter 13)."
}

$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$std = if ($env:STD) { $env:STD } else { 'c++17' }

$sdk = ''
$runArgs = @()
if ($Rest -and $Rest.Count -gt 0 -and $Rest[0] -in @('fakesdk', 'fakedevice', 'comlab')) {
    $sdk = $Rest[0]
    if ($Rest.Count -gt 1) { $runArgs = @($Rest[1..($Rest.Count - 1)]) }
} elseif ($Rest) {
    $runArgs = @($Rest)
}

$tmp = Join-Path ([System.IO.Path]::GetTempPath()) ([System.IO.Path]::GetRandomFileName())
New-Item -ItemType Directory -Path $tmp | Out-Null
try {
    $out = Join-Path $tmp 'attempt.exe'
    $flags = @('/nologo', "/std:$std", '/W4', '/EHsc', '/Zi', '/fsanitize=address')
    if ($sdk) {
        $sdkDir = Join-Path $root "exercises/$sdk"
        # @(...) forces an array even for a single vendor file: splatting a
        # bare string hands cl one argument per CHARACTER (found the hard way).
        $vendor = @(Get-ChildItem $sdkDir -Filter 'Fake*.cpp' | ForEach-Object FullName)
        cl @flags "/I$sdkDir" $vendor $Source "/Fe:$out" "/Fo$tmp/" "/Fd$tmp/"
    } else {
        cl @flags $Source "/Fe:$out" "/Fo$tmp/" "/Fd$tmp/"
    }
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    Write-Host "== built clean: /std:$std /W4 /EHsc /fsanitize=address"
    & $out @runArgs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    Write-Host "== ran clean (exit 0, ASan quiet)"
} finally {
    Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
}
