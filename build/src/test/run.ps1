<#
.SYNOPSIS
    Minimal tests for the built KAWARI binaries (kosui.exe and shiori.dll).
.DESCRIPTION
    Suites driven by plain text case files:

    check   Runs "kosui --check" (sandbox mode, diagnostics with locations and exit codes) on the
            dictionary of every folder of test/check and compares the output and the exit code.

    kosui   Feeds one line to kosui.exe (echo mode or command mode) and compares the output with the
            expected text. This tests the dictionary / KIS engine. Every case runs in a fresh copy of
            test/kosui, so cases do not affect each other.
            kosui must be told to quit with "exit": at the end of its input it loops forever, so the
            runner always sends "exit" and kills kosui after -TimeoutSeconds.

    shiori  Builds test/harness/shiori_harness.c, loads shiori.dll with the ghost in test/shiori
            (load), sends the SHIORI/3.0 requests of test/shiori/cases.txt (request) and unloads it.
            Every expected text must be found in the matching response. This tests the SHIORI layer.
            The harness is built with -CC and must have the same bitness as shiori.dll (32bit).

    baseware  Same as shiori, with the ghost and cases in test/baseware and a stub baseware window in the
            harness (HARNESS_BASEWARE): getproperty / setproperty / executesstp and SAORI over DirectSSTP.

    kosui7  Same as kosui, with the folder test/kosui7: it has kawari.ini and no kawarirc.kis, so kosui runs
            in the Kawari 7 compatible mode. Most of the expected outputs are the outputs of the real
            Kawari 7.5.0 kosui.exe (test/gen_kosui7_cases.py makes them).

    shiori7 Same as shiori, with the ghost and cases in test/shiori7 (Kawari 7 compatible mode, SHIORI/2.x).
            The verb REQ sends a request line as it is. With -BinDir pointing at the Kawari 7.5.0 files, the
            cases before "extensions" also pass on the real 7.5.0 shiori.dll.

    multi   Builds test/harness/multi_harness.c and tests the multi instance exports (multi_load / multi_loadu /
            multi_unload / multi_request) together with the legacy load / request / unload: independent
            random numbers per instance, invalid ids, unload in either order, OnUnload for every instance.

    The binaries must already be built (build/src: gmake -f gcc.mak). Nothing is written to the
    repository: the work files go to a temporary folder.

    Exit codes: 0 = all passed, 1 = a case failed, 2 = the tests could not run (missing binaries,
    compiler problems, wrong bitness).
.PARAMETER BinDir
    Folder with kosui.exe and shiori.dll (default: build/mach/mingw).
.PARAMETER Suite
    all (default), kosui, check, shiori, baseware, kosui7, shiori7 or multi.
.PARAMETER CC
    C compiler for the harness (default: gcc). Put the 32bit MinGW first in PATH or give its full path.
.PARAMETER KeepWork
    Keep the temporary work folder and print its path.
.EXAMPLE
    powershell -NoProfile -ExecutionPolicy Bypass -File build/src/test/run.ps1
.EXAMPLE
    powershell -NoProfile -ExecutionPolicy Bypass -File build/src/test/run.ps1 -Suite kosui
#>
[CmdletBinding()]
param(
    [string]$BinDir,
    [ValidateSet('all', 'kosui', 'check', 'shiori', 'baseware', 'kosui7', 'shiori7', 'multi')]
    [string]$Suite = 'all',
    [string]$CC = 'gcc',
    [int]$TimeoutSeconds = 30,
    [switch]$KeepWork
)
$ErrorActionPreference = 'Stop'

# Bytes are passed through unchanged (one byte = one char), so no character code conversion happens.
$Latin1 = [Text.Encoding]::GetEncoding(28591)

$script:Passed = 0
$script:Failed = 0

function Stop-Setup([string]$Message) {
    Write-Host "test: CANNOT RUN - $Message"
    exit 2
}

function Quote-Arg([string]$Text) {
    return '"' + $Text.TrimEnd('\') + '"'
}

# Runs a program, optionally feeding it text, and returns its output. Kills it after the timeout.
# The input goes through a file and "<" of cmd.exe: a redirected stdin of .NET Framework starts with a
# UTF-8 BOM, which breaks the first line that the program reads.
function Invoke-Program([string]$File, [string[]]$ArgList, [string]$WorkDir, [string]$InputText) {
    $psi = New-Object Diagnostics.ProcessStartInfo
    $psi.WorkingDirectory = $WorkDir
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.StandardOutputEncoding = $Latin1
    $psi.StandardErrorEncoding = $Latin1
    $inputFile = $null
    if ($InputText) {
        $inputFile = [IO.Path]::GetTempFileName()
        [IO.File]::WriteAllBytes($inputFile, $Latin1.GetBytes($InputText))
        $psi.FileName = $env:ComSpec
        $psi.Arguments = '/d /s /c ""' + $File + '" ' + ($ArgList -join ' ') + ' < "' + $inputFile + '""'
    } else {
        $psi.FileName = $File
        $psi.Arguments = ($ArgList -join ' ')
    }
    try {
        $proc = [Diagnostics.Process]::Start($psi)
        $outTask = $proc.StandardOutput.ReadToEndAsync()
        $errTask = $proc.StandardError.ReadToEndAsync()
        $timedOut = -not $proc.WaitForExit($TimeoutSeconds * 1000)
        if ($timedOut) {
            # Kill the whole tree: cmd.exe is only the parent of the program.
            # It may have ended just now, so ignore a failure of taskkill.
            try { & taskkill /F /T /PID $proc.Id 2>&1 | Out-Null } catch { }
            $proc.WaitForExit()
        } else {
            $proc.WaitForExit()
        }
    } finally {
        if ($inputFile) { Remove-Item -LiteralPath $inputFile -Force -ErrorAction SilentlyContinue }
    }
    return @{
        ExitCode = $proc.ExitCode
        StdOut   = $outTask.Result
        StdErr   = $errTask.Result
        TimedOut = $timedOut
    }
}

# Reads a case file: a line that starts with $ExpectMark adds an expected line to the current case,
# any other line (not blank, not "#") starts a new case ("<verb> <rest>").
function Read-Cases([string]$Path, [string]$ExpectMark, [string[]]$Verbs) {
    $cases = New-Object System.Collections.Generic.List[object]
    $current = $null
    $lineNo = 0
    foreach ($line in [IO.File]::ReadAllLines($Path, $Latin1)) {
        $lineNo++
        if ($line.Trim() -eq '' -or $line.StartsWith('#')) { continue }
        if ($line.StartsWith($ExpectMark)) {
            if (-not $current) { Stop-Setup "${Path}:${lineNo}: expected text before any case" }
            $text = $line.Substring(1)
            if ($text.StartsWith(' ')) { $text = $text.Substring(1) }
            $current.Expected.Add($text)
            continue
        }
        $verb, $rest = $line -split '\s+', 2
        if ($Verbs -notcontains $verb) { Stop-Setup "${Path}:${lineNo}: unknown verb '$verb' (expected: $($Verbs -join ', '))" }
        $current = @{ Line = $lineNo; Verb = $verb; Rest = $rest; Expected = (New-Object System.Collections.Generic.List[string]) }
        $cases.Add($current)
    }
    foreach ($case in $cases) {
        if ($case.Expected.Count -eq 0) { Stop-Setup "${Path}:$($case.Line): the case has no expected text" }
    }
    return $cases
}

function Write-Result([bool]$Ok, [string]$Name, [string[]]$Detail) {
    if ($Ok) {
        $script:Passed++
        Write-Host "ok  $Name"
    } else {
        $script:Failed++
        Write-Host "NG  $Name"
        foreach ($d in $Detail) { Write-Host "      $d" }
    }
}

# Returns i386 or x64 from the PE header.
function Get-PeMachine([string]$Path) {
    $b = [IO.File]::ReadAllBytes($Path)
    $pe = [BitConverter]::ToInt32($b, 0x3C)
    $machine = [BitConverter]::ToUInt16($b, $pe + 4)
    if ($machine -eq 0x14c) { return 'i386' }
    if ($machine -eq 0x8664) { return 'x64' }
    return ('0x{0:x}' -f $machine)
}

function Show-Text([string]$Text) {
    return ($Text -replace "`r", '\r' -replace "`n", '\n')
}

# ---- kosui ----------------------------------------------------------------------------------

function Test-Kosui([string]$Work, [string]$Suite7 = 'kosui') {
    # (PowerShell variable names are not case sensitive: $name below is the label of a case, not $Name)
    $exe = Join-Path $BinDir 'kosui.exe'
    if (-not (Test-Path -LiteralPath $exe)) { Stop-Setup "kosui.exe was not found in $BinDir" }
    $fixture = Join-Path $PSScriptRoot $Suite7
    $cases = Read-Cases (Join-Path $fixture 'cases.txt') '=' @('echo', 'cmd')

    Write-Host "== $Suite7 ($($cases.Count) cases)"
    $banner = "'.' is mode change command.`n"
    $index = 0
    $attachAnswer = ''
    foreach ($case in $cases) {
        $index++
        $name = "${Suite7}:$($case.Line) $($case.Verb) $($case.Rest)"
        $dir = Join-Path $Work ($Suite7 + $index)
        New-Item -ItemType Directory -Path $dir | Out-Null
        # every file of the fixture folder except the case file (kawarirc.kis, or kawari.ini and the dictionaries)
        foreach ($f in Get-ChildItem -LiteralPath $fixture -File) {
            if ($f.Name -ne 'cases.txt') { Copy-Item -LiteralPath $f.FullName -Destination $dir }
        }

        if ($case.Verb -eq 'cmd') { $session = ".`n$($case.Rest)`nexit`n" } else { $session = "$($case.Rest)`nexit`n" }
        $run = Invoke-Program $exe @() $dir ($attachAnswer + $session)
        $out = $run.StdOut -replace "`r`n", "`n"
        # A running ghost (SSP etc.) makes kosui ask "Attach :" first, which swallows the first line
        # of the session (the prompt is still reached afterwards, so every case would just fail).
        # Answer "0" (interpreter mode) in front of the sessions and run this case again.
        if (-not $attachAnswer -and -not $run.TimedOut -and $out.Contains('Attach :')) {
            Write-Host "note: a running ghost was found; answering 'Attach :' with 0 (interpreter mode)"
            $attachAnswer = "0`n"
            $run = Invoke-Program $exe @() $dir ($attachAnswer + $session)
            $out = $run.StdOut -replace "`r`n", "`n"
        }
        $expected = $case.Expected -join "`n"

        if ($run.TimedOut) {
            Write-Result $false $name @("kosui did not finish within $TimeoutSeconds seconds")
            continue
        }
        $pos = $out.IndexOf($banner)
        if ($pos -lt 0) {
            Write-Result $false $name @("kosui did not reach its prompt", "output: $(Show-Text $out)")
            continue
        }
        $rest = $out.Substring($pos + $banner.Length)
        $m = [regex]::Match($rest, '(?s)^(?:(?:echo|command)-mode > )+(.*?)\n(?:echo|command)-mode > $')
        if (-not $m.Success) {
            Write-Result $false $name @("could not find the result in the output", "output: $(Show-Text $rest)")
            continue
        }
        $actual = $m.Groups[1].Value
        Write-Result ($actual -ceq $expected) $name @("expected: $(Show-Text $expected)", "actual:   $(Show-Text $actual)")
    }
}

# ---- check ----------------------------------------------------------------------------------

# Splits a line of arguments; 'single' and "double" quotes group words (nothing is expanded).
function Split-Args([string]$Text) {
    $tokens = New-Object System.Collections.Generic.List[string]
    foreach ($m in [regex]::Matches($Text, '''([^'']*)''|"([^"]*)"|(\S+)')) {
        if ($m.Groups[1].Success) { $tokens.Add($m.Groups[1].Value) }
        elseif ($m.Groups[2].Success) { $tokens.Add($m.Groups[2].Value) }
        else { $tokens.Add($m.Groups[3].Value) }
    }
    return $tokens
}

# kosui --check: every folder of test/check is one case.
#   kawarirc.kis (and other dictionary files)  copied to a work folder as they are
#   args    the arguments after "--check"
#   expect  the whole standard output (the work folder path is removed from it)
#   exit    the exit code
# The folder must have the same files after the run: --check must not write anything.
function Test-Check([string]$Work) {
    $exe = Join-Path $BinDir 'kosui.exe'
    if (-not (Test-Path -LiteralPath $exe)) { Stop-Setup "kosui.exe was not found in $BinDir" }
    $root = Join-Path $PSScriptRoot 'check'
    $cases = @(Get-ChildItem -LiteralPath $root -Directory | Sort-Object Name)

    Write-Host "== check ($($cases.Count) cases)"
    foreach ($case in $cases) {
        $name = "check:$($case.Name)"
        $dir = Join-Path $Work ('check_' + $case.Name)
        New-Item -ItemType Directory -Path $dir | Out-Null
        foreach ($f in Get-ChildItem -LiteralPath $case.FullName -File) {
            if (@('args', 'expect', 'exit') -notcontains $f.Name) { Copy-Item -LiteralPath $f.FullName -Destination $dir }
        }
        $before = (@(Get-ChildItem -LiteralPath $dir -File | ForEach-Object { $_.Name }) | Sort-Object) -join ','

        $argText = ([IO.File]::ReadAllText((Join-Path $case.FullName 'args'), $Latin1)).Trim()
        $argList = @('--check') + @(Split-Args $argText | ForEach-Object { Quote-Arg $_ })
        $run = Invoke-Program $exe $argList $dir ''

        $out = ($run.StdOut -replace "`r`n", "`n").Replace($dir + '\', '')
        $expected = [IO.File]::ReadAllText((Join-Path $case.FullName 'expect'), $Latin1) -replace "`r`n", "`n"
        $expectedExit = [int](([IO.File]::ReadAllText((Join-Path $case.FullName 'exit'), $Latin1)).Trim())
        $after = (@(Get-ChildItem -LiteralPath $dir -File | ForEach-Object { $_.Name }) | Sort-Object) -join ','

        if ($run.TimedOut) {
            Write-Result $false $name @("kosui did not finish within $TimeoutSeconds seconds")
            continue
        }
        $detail = @()
        if ($run.ExitCode -ne $expectedExit) { $detail += "exit code: expected $expectedExit, actual $($run.ExitCode)" }
        if ($out -cne $expected) { $detail += "expected: $(Show-Text $expected)"; $detail += "actual:   $(Show-Text $out)" }
        if ($before -ne $after) { $detail += "files changed: [$before] -> [$after]" }
        Write-Result ($detail.Count -eq 0) $name $detail
    }
}

# ---- shiori ---------------------------------------------------------------------------------

# Builds a C harness from test/harness. The harness loads the DLL into its own process, so the
# compiler and shiori.dll must have the same bitness. Returns the path of the executable.
function Build-Harness([string]$Work, [string]$Source, [string]$Exe) {
    $dll = Join-Path $BinDir 'shiori.dll'
    if (-not (Test-Path -LiteralPath $dll)) { Stop-Setup "shiori.dll was not found in $BinDir" }
    $dllMachine = Get-PeMachine $dll
    try {
        $target = (Invoke-Program $CC @('-dumpmachine') $PSScriptRoot '').StdOut.Trim()
    } catch {
        Stop-Setup "cannot run the C compiler '$CC' (use -CC to give a 32bit MinGW gcc)"
    }
    $compilerMachine = 'other'
    if ($target -match '^(i[3-6]86)-') { $compilerMachine = 'i386' } elseif ($target -match '^x86_64-') { $compilerMachine = 'x64' }
    if ($dllMachine -ne $compilerMachine) {
        Stop-Setup "shiori.dll is $dllMachine but '$CC' builds for $target. Put a matching MinGW first in PATH or use -CC"
    }

    $harness = Join-Path $Work $Exe
    $build = Invoke-Program $CC @('-O1', '-Wall', '-o', (Quote-Arg $harness), (Quote-Arg (Join-Path $PSScriptRoot $Source))) $Work ''
    if ($build.ExitCode -ne 0 -or -not (Test-Path -LiteralPath $harness)) {
        Stop-Setup "building the harness failed:`n$($build.StdErr)"
    }
    return $harness
}

function Test-Shiori([string]$Work, [string]$Name) {
    $dll = Join-Path $BinDir 'shiori.dll'
    if (-not (Test-Path -LiteralPath $dll)) { Stop-Setup "shiori.dll was not found in $BinDir" }
    $fixture = Join-Path $PSScriptRoot $Name
    $cases = Read-Cases (Join-Path $fixture 'cases.txt') '~' @('GET', 'NOTIFY', 'REQ')

    $harness = Build-Harness $Work 'harness\shiori_harness.c' 'shiori_harness.exe'

    $ghost = Join-Path $Work "ghost_$Name"
    New-Item -ItemType Directory -Path $ghost | Out-Null
    foreach ($f in Get-ChildItem -LiteralPath $fixture -File) {
        if ($f.Name -ne 'cases.txt') { Copy-Item -LiteralPath $f.FullName -Destination $ghost }
    }
    Copy-Item -Path $dll -Destination $ghost

    # One request file per case.
    $requests = New-Object System.Collections.Generic.List[string]
    $index = 0
    foreach ($case in $cases) {
        $index++
        $headers = New-Object System.Collections.Generic.List[string]
        if ($case.Verb -eq 'REQ') {
            # "REQ <first line> | <Header>: <value> | ..." : the request line as it is (SHIORI/2.x and so on)
            $parts = $case.Rest -split '\s\|\s'
            $headers.Add($parts[0])
            $headers.Add('Charset: Shift_JIS')
            # "Sender: SSP" and "SecurityLevel: local" unless the case gives its own
            $custom = @($parts | Select-Object -Skip 1)
            if (-not ($custom | Where-Object { $_ -like 'Sender:*' })) { $headers.Add('Sender: SSP') }
            if (-not ($custom | Where-Object { $_ -like 'SecurityLevel:*' })) { $headers.Add('SecurityLevel: local') }
            foreach ($h in $custom) { $headers.Add($h) }
        } else {
            $id, $refs = $case.Rest -split '\s+', 2
            $headers.Add("$($case.Verb) SHIORI/3.0")
            $headers.Add('Charset: Shift_JIS')
            $headers.Add('Sender: SSP')
            $headers.Add('SecurityLevel: local')
            $headers.Add("ID: $id")
            if ($refs) {
                $n = 0
                foreach ($ref in ($refs -split ',')) { $headers.Add("Reference${n}: $($ref.Trim())"); $n++ }
            }
        }
        $file = Join-Path $Work ('req{0:D3}.txt' -f $index)
        [IO.File]::WriteAllBytes($file, $Latin1.GetBytes(($headers -join "`r`n") + "`r`n`r`n"))
        $requests.Add((Quote-Arg $file))
    }

    $outFile = Join-Path $Work "responses_$Name.txt"
    $argList = @((Quote-Arg $dll), (Quote-Arg $ghost), (Quote-Arg $outFile)) + $requests
    # The harness loads the copy in the ghost folder, so shiori.dll itself is never touched.
    $argList[0] = Quote-Arg (Join-Path $ghost 'shiori.dll')
    # The stub baseware window is only for the baseware suite.
    if ($Name -eq 'baseware') { $env:HARNESS_BASEWARE = '1' } else { Remove-Item Env:HARNESS_BASEWARE -ErrorAction SilentlyContinue }
    $run = Invoke-Program $harness $argList $Work ''
    Remove-Item Env:HARNESS_BASEWARE -ErrorAction SilentlyContinue
    if ($run.TimedOut) { Stop-Setup "the harness did not finish within $TimeoutSeconds seconds" }
    if ($run.ExitCode -ne 0) { Stop-Setup "the harness failed (exit code $($run.ExitCode)): $($run.StdErr.Trim())" }

    $responses = $Latin1.GetString([IO.File]::ReadAllBytes($outFile)) -split "`n=====`n"
    # The text ends with a separator, so there is one empty piece after the last response.
    if ($responses.Count -ne $cases.Count + 1) {
        Stop-Setup "expected $($cases.Count) responses but the harness wrote $($responses.Count - 1)"
    }

    Write-Host "== $Name ($($cases.Count) cases)"
    $index = 0
    foreach ($case in $cases) {
        $response = $responses[$index]
        $index++
        $label = "${Name}:$($case.Line) $($case.Verb) $($case.Rest)"
        $missing = @($case.Expected | Where-Object { -not $response.Contains($_) })
        $detail = @()
        foreach ($text in $missing) { $detail += "missing: $text" }
        if ($missing.Count -gt 0) { $detail += "response: $(Show-Text $response)" }
        Write-Result ($missing.Count -eq 0) $label $detail
    }
}

# ---- multi ----------------------------------------------------------------------------------

# The multi instance exports (multi_load / multi_loadu / multi_unload / multi_request) next to the
# legacy load / request / unload. The checks are in harness/multi_harness.c; it prints "ok <name>" or
# "NG <name> | <detail>" per check.
function Test-Multi([string]$Work) {
    $dll = Join-Path $BinDir 'shiori.dll'
    $harness = Build-Harness $Work 'harness\multi_harness.c' 'multi_harness.exe'

    # Three copies of the fixture ghost: L (legacy load), A and B (multi_load*).
    $dirs = @()
    foreach ($n in 'L', 'A', 'B') {
        $d = Join-Path $Work "ghost_$n"
        New-Item -ItemType Directory -Path $d | Out-Null
        Copy-Item -Path (Join-Path $PSScriptRoot 'multi\*') -Destination $d
        $dirs += $d
    }
    $copy = Join-Path $dirs[0] 'shiori.dll'
    Copy-Item -Path $dll -Destination $copy

    $argList = @((Quote-Arg $copy)) + @($dirs | ForEach-Object { Quote-Arg $_ })
    $run = Invoke-Program $harness $argList $Work ''
    if ($run.TimedOut) { Stop-Setup "the multi harness did not finish within $TimeoutSeconds seconds" }
    if ($run.ExitCode -ge 100) { Stop-Setup "the multi harness could not run (exit code $($run.ExitCode)): $($run.StdErr.Trim())" }

    Write-Host '== multi'
    $seen = 0
    foreach ($line in ($run.StdOut -split "`r?`n")) {
        if ($line -match '^ok (.+)$') {
            $seen++
            Write-Result $true "multi: $($Matches[1])" @()
        } elseif ($line -match '^NG ([^|]+?)\s*\|\s*(.*)$') {
            $seen++
            Write-Result $false "multi: $($Matches[1])" @("detail: $($Matches[2])")
        }
    }
    if ($seen -eq 0) { Write-Result $false 'multi: the harness printed no result' @($run.StdErr.Trim()) }
    if ($run.ExitCode -ne 0 -and $run.StdOut -notmatch '(?m)^NG ') {
        Write-Result $false 'multi: the harness exit code' @("exit code $($run.ExitCode) without an NG line")
    }
}

# ---- main -----------------------------------------------------------------------------------

if (-not $BinDir) { $BinDir = Join-Path $PSScriptRoot '..\..\mach\mingw' }
if (-not (Test-Path -LiteralPath $BinDir)) { Stop-Setup "BinDir does not exist: $BinDir (build first: gmake -f gcc.mak in build/src)" }
$BinDir = (Resolve-Path -LiteralPath $BinDir).Path

$work = Join-Path ([IO.Path]::GetTempPath()) ('kawari-test-' + [guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Path $work | Out-Null
try {
    if ($Suite -eq 'all' -or $Suite -eq 'kosui') { Test-Kosui $work }
    if ($Suite -eq 'all' -or $Suite -eq 'check') { Test-Check $work }
    if ($Suite -eq 'all' -or $Suite -eq 'shiori') { Test-Shiori $work 'shiori' }
    if ($Suite -eq 'all' -or $Suite -eq 'baseware') { Test-Shiori $work 'baseware' }
    if ($Suite -eq 'all' -or $Suite -eq 'kosui7') { Test-Kosui $work 'kosui7' }
    if ($Suite -eq 'all' -or $Suite -eq 'shiori7') { Test-Shiori $work 'shiori7' }
    if ($Suite -eq 'all' -or $Suite -eq 'multi') { Test-Multi $work }
} finally {
    if ($KeepWork) {
        Write-Host "work folder kept: $work"
    } else {
        Remove-Item -LiteralPath $work -Recurse -Force -ErrorAction SilentlyContinue
    }
}

Write-Host ''
Write-Host "test: $($script:Passed) passed, $($script:Failed) failed"
if ($script:Failed -gt 0) { exit 1 }
exit 0
