param($Old = "v3a.txt", $New = "new.txt")
foreach ($f in $Old, $New) { if (-not (Test-Path $f)) { throw "file not found: $f" } }
$rx = '^(\d)\s+(.+?)\s+sel=(\d+) cb=(\d+)\s+(\d+) MB/s'
$a = gc $Old | % { [regex]::Match($_, $rx) } | ? Success
$b = gc $New | % { [regex]::Match($_, $rx) } | ? Success

$bad = 0
for ($i = 0; $i -lt [Math]::Min($a.Count, $b.Count); $i++) {
    $x = $a[$i].Groups; $y = $b[$i].Groups
    $same = $x[3].Value -eq $y[3].Value -and $x[4].Value -eq $y[4].Value
    if (-not $same) { $bad++ }
    $pct  = ([double]$y[5].Value / [double]$x[5].Value) - 1
    $note = if ($same) { "same" } else { "DIFF sel $($x[3].Value)->$($y[3].Value) cb $($x[4].Value)->$($y[4].Value)" }
    "{0}  {1,-28} {2,5} -> {3,5} MB/s  {4,7:P1}  {5}" -f $x[1].Value, $x[2].Value, $x[5].Value, $y[5].Value, $pct, $note
}
if ($bad) { "$bad row(s) with DIFFERENT counts" } else { "all counts same" }