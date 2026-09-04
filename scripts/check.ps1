# Build and run YOUR exercise attempt with the handbook's canonical flags - MSVC.
#
#   scripts\check.ps1 <your.cpp> [more.cpp...] [fakesdk|fakedevice|comlab] [args passed to the run...]
#
#   scripts\check.ps1 attempt.cpp                     # plain exercise
#   scripts\check.ps1 attempt.cpp fakesdk             # + vendor code
#   scripts\check.ps1 registry.cpp main.cpp 100       # several TUs + run args
#   $env:STD='c++20'; scripts\check.ps1 attempt.cpp   # C++20 (words, invalid)
#   $env:SAN='none'; scripts\check.ps1 a.cpp b.cpp    # no sanitizer at all
#   $env:OPT='2'; scripts\check.ps1 a.cpp b.cpp       # optimised (/O2)
#
# Every leading argument ending in .cpp is a source file; they are compiled
# together in the order given (also the link order - Chapter 32's two-order
# test depends on that). The first argument that is neither a .cpp file nor
# a vendor name starts the run args.
#
# Run it from a Developer PowerShell for VS, so cl.exe is on PATH (Chapter 13).
# This mirrors scripts/check.sh; the differences are MSVC's, not the script's:
# /fsanitize=address only - MSVC has no UBSan (Chapter 31) - and no
# ThreadSanitizer at all, so Chapter 29's TSan step needs clang or gcc (WSL
# is the usual Windows answer). CI smoke-tests this script in the
# buildlab-msvc job so it cannot rot unnoticed.
#
# Env overrides mirror check.sh's: STD (default c++17), SAN (default address;
# `none` builds with no sanitizer) and OPT (default 0, so /Od; OPT=2 is /O2).
# SAN=none exists for the exercises whose subject is what the tools do NOT
# catch - Chapter 27's ODR diamond, where step 5 asks you to see that nothing
# warned you and then rebuild optimised to watch the symptom change, before
# step 6 turns the sanitizer back on. A build that warns cannot show step 5.
param(
    [Parameter(Mandatory = $true)][string]$Source,
    [Parameter(ValueFromRemainingArguments = $true)][string[]]$Rest
)
$ErrorActionPreference = 'Stop'

if (-not (Get-Command cl -ErrorAction SilentlyContinue)) {
    Write-Error "cl.exe not found - run this from a Developer PowerShell for VS (Chapter 13)."
}

$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
# The one vendored header (Appendix F's JSON recipes), reachable from any attempt.
$thirdParty = Join-Path $root "exercises/third_party"
$std = if ($env:STD) { $env:STD } else { 'c++17' }
$san = if ($env:SAN) { $env:SAN } else { 'address' }
$opt = if ($env:OPT) { $env:OPT } else { '0' }
# MSVC spells "no optimisation" /Od rather than /O0, which is the one place
# this mapping is not a straight copy of the -O$OPT on the sh side.
$optFlag = if ($opt -eq '0') { '/Od' } else { "/O$opt" }
if ($san -eq 'thread') {
    Write-Error "SAN=thread: MSVC has no ThreadSanitizer. Chapter 29's second build needs clang or gcc - WSL is the usual Windows answer."
}

# @(...) throughout: without it a one-element result decays to a scalar and
# .Count / [i..j] slicing stop meaning what they say (found the hard way).
$sources = @($Source)
$rest = if ($Rest) { @($Rest) } else { @() }
$i = 0
while ($i -lt $rest.Count -and $rest[$i] -like '*.cpp') {
    $sources += $rest[$i]
    $i++
}
$sdk = ''
if ($i -lt $rest.Count -and $rest[$i] -in @('fakesdk', 'fakedevice', 'comlab')) {
    $sdk = $rest[$i]
    $i++
}
$runArgs = @()
if ($i -lt $rest.Count) { $runArgs = @($rest[$i..($rest.Count - 1)]) }

$tmp = Join-Path ([System.IO.Path]::GetTempPath()) ([System.IO.Path]::GetRandomFileName())
New-Item -ItemType Directory -Path $tmp | Out-Null
try {
    $out = Join-Path $tmp 'attempt.exe'
    $flags = @('/nologo', "/std:$std", '/W4', '/EHsc', '/Zi', $optFlag)
    if ($san -ne 'none') { $flags += "/fsanitize=$san" }
    if ($sdk) {
        $sdkDir = Join-Path $root "exercises/$sdk"
        # @(...) forces an array even for a single vendor file: splatting a
        # bare string hands cl one argument per CHARACTER (found the hard way).
        $vendor = @(Get-ChildItem $sdkDir -Filter 'Fake*.cpp' | ForEach-Object FullName)
        cl @flags "/I$sdkDir" "/I$thirdParty" $vendor $sources "/Fe:$out" "/Fo$tmp/" "/Fd$tmp/"
    } else {
        cl @flags "/I$thirdParty" $sources "/Fe:$out" "/Fo$tmp/" "/Fd$tmp/"
    }
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    Write-Host "== built clean: $($flags -join ' ')"
    & $out @runArgs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    if ($san -eq 'none') {
        # Not "ASan quiet": there was no ASan. See check.sh for the same rule.
        Write-Host "== ran (exit 0) - NO sanitizer was built in, so this says nothing about UB"
    } else {
        Write-Host "== ran clean (exit 0, ASan quiet)"
    }
} finally {
    Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
}
