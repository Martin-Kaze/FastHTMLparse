# Comparing with other parsers

Notes for a fair speed comparison against other HTML libraries.

## Candidates

| Library | Language | Kind | Why it is interesting |
|---|---|---|---|
| lol-html (Cloudflare) | Rust | streaming, CSS selectors | closest to fasthtml: no DOM, selectors on a byte stream |
| lexbor | C | DOM + CSS selectors | one of the fastest full HTML5 parsers |
| html5ever + scraper | Rust | DOM + CSS selectors | the Servo parser, widely used |
| Gumbo + gumbo-query | C / C++ | DOM + CSS selectors | Google's HTML5 parser |
| libxml2 (HTML parser + XPath) | C | DOM + XPath | very common baseline |
| selectolax | Python (lexbor inside) | DOM + CSS | quick sanity check from Python |

## Rules for a fair test

1. Same input file for everyone (for example `big.html`, 139 MB), loaded into memory **before** timing.
2. Same task: count the elements matching the same selector, and check that every library reports
   the **same count** before trusting its time. Start with `div`, `ul > li`, `a`, `div.some-class`.
3. Time only parse + match, best of several runs, one thread, same compiler flags (`-O3`, same `-march`).
4. Report MB/s = file size / time.
5. Say what each library does: DOM parsers build a full tree and follow the whole HTML5 spec,
   fasthtml and lol-html only match. Both numbers are useful, but they measure different work.
6. Also try feeding 64 KB pieces for the streaming libraries (fasthtml, lol-html), like a real network read.

## fasthtml side

`fasthtml_bench <file> <selector>` prints `selectors:` (the count) and `speed:` (MB/s, best of 7, 64 KB pieces).
For memory-only timing in your own harness use `fasthtml::count(html, selector)`.
