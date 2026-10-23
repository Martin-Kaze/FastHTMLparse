# Runs the benchmark for the 8 template variants (no id/class/relation ... all three).
#   .\bench\bench.ps1 big.html div some-id some-class -Exe .\build\fasthtml_bench.exe | Tee-Object new.txt
# Each row: counts from a run with a callback, speed = best of 3 runs without one.
param($F, $Tag, $Id, $Cls, $Exe = ".\build\fasthtml_bench.exe")
foreach ($p in $F, $Exe) { if (-not (Test-Path $p)) { throw "file not found: $p" } }

$sel = @("$Tag", "$Tag#$Id", "$Tag.$Cls", "$Tag#$Id.$Cls",
         "div > $Tag", "div > $Tag#$Id", "div > $Tag.$Cls", "div > $Tag#$Id.$Cls")
for ($i = 0; $i -lt 8; $i++) {
    $s = $sel[$i]
    $cb = & $Exe $F $s cb
    $sc = ($cb | sls '^selectors:\s*(\d+)').Matches.Groups[1].Value
    $cc = ($cb | sls '^callbacks:\s*(\d+)').Matches.Groups[1].Value
    $mb = 0
    1..3 | % {
        $o = & $Exe $F $s
        $v = [int](($o | sls '^speed:\s*([\d.]+)').Matches.Groups[1].Value)
        if ($v -gt $mb) { $mb = $v }
    }
    "{0}  {1,-34} sel={2} cb={3}  {4} MB/s" -f $i, $s, $sc, $cc, $mb
}
