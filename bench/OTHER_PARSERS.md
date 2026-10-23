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

## Results (2026-10-23)

Ryzen 5 5600H, Windows 11, "Best performance" power mode. MB/s = file size / (parse + match time), best of 7
(best of 3 for the Python libraries). Raw output: `bench/results/compare-big.txt`, `bench/results/compare-test1.txt`.

### big.html (139 MB, one Wikipedia page repeated 100 times)

| Selector | Matches | fasthtml | lol-html | lexbor | selectolax | html5ever + scraper |
|---|---:|---:|---:|---:|---:|---:|
| `div` | 37,700 | **1,503** | 1,048 | 159 | 166 | 60 |
| `div > div` | 11,900 | **1,484** | 1,028 | 162 | 166 | 60 |
| `a` | 215,100 | **1,486** | 801 | 162 | 163 | 61 |
| `ul > li` | 72,300 | **1,489** | 996 | 162 | 165 | 59 |
| `span` | 271,400 | **1,493** | 761 | 163 | 160 | 60 |
| `a.external` | 29,800 | **1,359** | 746 | 162 | 166 | 59 |
| `div .mw-heading` | 6,200 | **1,210** | 403 | 162 | 167 | 57 |
| `table td` | 11,400 | **1,498** | 1,077 | 162 | 169 | 59 |

All five libraries report exactly the same count for every selector.

**lxml is left out of this table:** its libxml2 (2.11.9) keeps only the first `<html>…</html>` document when
several are joined, so it finds 1/100 of the matches (377 `div`s). Its time is not for the same work.

### test1.html (1.4 MB, the same page once)

| Selector | Matches | fasthtml | lol-html | lexbor | selectolax | scraper | lxml |
|---|---:|---:|---:|---:|---:|---:|---:|
| `div` | 377 | **1,616** | 1,073 | 171 | 178 | 60 | 50 |
| `div > div` | 119 | **1,542** | 1,082 | 166 | 170 | 59 | 51 |
| `a` | 2,151 | **1,553** | 816 | 165 | 159 | 59 | 48 |
| `ul > li` | 723 | **1,527** | 985 | 161 | 167 | 59 | 48 |
| `span` | 2,714 | **1,566** | 772 | 176 | 168 | 60 | 51 |
| `a.external` | 298 | **1,408** | 757 | 175 | 166 | 56 | 45 |
| `div .mw-heading` | 62 | **1,183** | 403 | 160 | 156 | 54 | 19 |
| `table td` | 114 | **1,514** | 1,050 | 162 | 162 | 58 | 48 |

All six libraries agree on every count here.

### How to read this

- **lol-html** is the fair comparison (streaming, no tree, 64 KB pieces like fasthtml). fasthtml is
  **1.4–1.5×** faster on plain tags and child selectors, **~2×** on selectors with many matches or a class,
  and **3×** on descendant selectors (`div .mw-heading`). lol-html also writes the (unchanged) document back
  out, because it is a rewriter: that is part of its cost and can't be turned off.
- **lexbor / selectolax / html5ever / lxml** build a full HTML5 DOM (all spec rules, every node kept in memory)
  that can then be queried many times. They do much more work, so the ~9× (lexbor) to ~25× (html5ever) gap is
  expected. Almost all of their time is parsing: lexbor parses `big.html` in ~780 ms and matches `div` in ~85 ms.
- selectolax is lexbor inside Python, so it runs at the same speed.
- Freeing the DOM is not timed.

### Versions and build

| Library | Version | Built with |
|---|---|---|
| fasthtml | 1.0.0 | clang (MSYS2 clang64), `-O3 -march=native`, LTO |
| lol-html | 3.0.2 | rustc 1.95, `-C target-cpu=native`, LTO |
| html5ever + scraper | 0.40.1 + 0.29.0 | same as lol-html |
| lexbor | master `f4cbbcd` | clang, `-O3 -march=native`, static |
| selectolax | 1.0.2 (PyPI wheel) | Python 3.13 |
| lxml | 6.1.3 (PyPI wheel, libxml2 2.11.9) | Python 3.13 |

## Running it again

The harnesses are in `bench/other/` (they print the same `selectors:` / `speed:` lines as `fasthtml_bench`):

```powershell
# lexbor: build it once, then the harness
git clone --depth 1 https://github.com/lexbor/lexbor.git ..\lexbor
cmake -S ..\lexbor -B ..\lexbor\b -G Ninja -DCMAKE_BUILD_TYPE=Release -DLEXBOR_BUILD_SHARED=OFF -DLEXBOR_BUILD_STATIC=ON -DCMAKE_C_FLAGS="-O3 -march=native"
cmake --build ..\lexbor\b
cc -O3 -march=native -DLEXBOR_STATIC -I..\lexbor\source bench\other\lexbor_bench.c ..\lexbor\b\liblexbor_static.a -o lexbor_bench.exe

# lol-html + html5ever/scraper
$env:RUSTFLAGS = "-C target-cpu=native"
cargo build --release --manifest-path bench\other\rust\Cargo.toml
copy bench\other\rust\target\release\other_bench.exe .

# selectolax + lxml
python -m pip install selectolax lxml cssselect

.\bench\other\run_all.ps1 -F src\big.html
```

`run_all.ps1` marks any row whose count differs from fasthtml with `COUNT DIFFERS`.
