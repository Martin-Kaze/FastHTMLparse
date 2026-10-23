# Runs fasthtml and the other libraries on the same file and selectors.
# Prints one row per (selector, library): match count and MB/s. A count that differs from fasthtml is marked.
param(
    $F      = "src\big.html",
    $Fast   = ".\build\fasthtml_bench.exe",
    $Lexbor = ".\lexbor_bench.exe",
    $Rust   = ".\other_bench.exe",
    $Py     = "python",
    $Libs   = @("fasthtml", "lol_html", "lexbor", "selectolax", "scraper", "lxml"),
    $Sel    = @("div", "div > div", "a", "ul > li", "span", "a.external", "div .mw-heading", "table td")
)
foreach ($s in $Sel) {
    $ref = $null
    foreach ($lib in $Libs) {
        $o = switch ($lib) {
            "fasthtml"   { & $Fast $F $s }
            "lexbor"     { & $Lexbor $F $s }
            "lol_html"   { & $Rust lol_html $F $s }
            "scraper"    { & $Rust scraper $F $s }
            default      { & $Py -I "$PSScriptRoot\py_bench.py" $lib $F $s }
        }
        $n = ($o | sls '^selectors:\s*(\d+)').Matches.Groups[1].Value
        $v = ($o | sls '^speed:\s*([\d.]+)').Matches.Groups[1].Value
        if ($null -eq $ref) { $ref = $n }
        $note = if ($n -ne $ref) { "COUNT DIFFERS" } else { "" }
        "{0,-18} {1,-11} count={2,-7} {3,8:N0} MB/s  {4}" -f $s, $lib, $n, [double]$v, $note
    }
}
