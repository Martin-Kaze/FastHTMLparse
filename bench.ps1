param($F, $Tag, $Id, $Cls, $Exe = ".\build\main.exe")
$sel = @("$Tag", "$Tag#$Id", "$Tag.$Cls", "$Tag#$Id.$Cls",
         "div > $Tag", "div > $Tag#$Id", "div > $Tag.$Cls", "div > $Tag#$Id.$Cls")
for ($i = 0; $i -lt 8; $i++) {
    $s = $sel[$i]
    $cb  = & $Exe $F $s cb
    $spd = & $Exe $F $s
    $sc = ($cb  | sls '^selectors:\s*(\d+)').Matches.Groups[1].Value
    $cc = ($cb  | sls '^callbacks:\s*(\d+)').Matches.Groups[1].Value
    $mb = [int](($spd | sls '^speed:\s*([\d.]+)').Matches.Groups[1].Value)
    "{0}  {1,-34} sel={2} cb={3}  {4} MB/s" -f $i, $s, $sc, $cc, $mb
}