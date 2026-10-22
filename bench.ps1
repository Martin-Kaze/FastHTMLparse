param($F, $Tag, $Id, $Cls, $Exe = ".\build\main.exe")
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