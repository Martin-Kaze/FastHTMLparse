# fasthtml

A small, very fast C++20 library that finds HTML elements with a CSS selector, without building a DOM tree.

You give it a selector like `div.content > p` and feed it the HTML. It reads the bytes once and tells you
every element that matches: how many there are, and optionally the text inside each one.
The HTML can arrive in pieces of any size (from a file, a socket, a download), split anywhere.

Measured speed: **about 1.5–1.6 GB/s** on one core (Ryzen 5 5600H), on a 139 MB real-world page.

---

## What it can do

**Selectors**

| Selector | Meaning |
|---|---|
| `div` | every `<div>` |
| `#main` | the element with `id="main"` |
| `.note` | every element whose `class` list contains `note` |
| `div#main.note` | all three at once (one id and one class per part, any order) |
| `ul > li` | an `li` whose parent is a `ul` |
| `div p` | a `p` anywhere inside a `div` |
| `div.box > ul li.item` | any mix of the above, up to 16 parts |

- Tag names are case-insensitive (`DIV` = `div`), ids and classes are case-sensitive, as in HTML.
- Invalid or unsupported selectors are rejected with a clear message, never silently mis-parsed.

**HTML it handles correctly**

- Quoted (`"…"`, `'…'`) and unquoted attribute values, spaces around `=`, uppercase attribute names,
  boolean attributes (`<input disabled id="x">`), `>` inside quoted values.
- Several classes in one attribute: `class="a b c"` matches `.b`.
- Comments (including `<!-->` and `<!--->`), `<!DOCTYPE>`, `<?xml …?>`, a stray `<` in text (`a < b`).
- `<script>` and `<style>` content is skipped as raw text, so a `"<div>"` inside JavaScript is not a tag.
- Void elements (`<br>`, `<img>`, `<input>`, …) with or without `/>`.
- Messy real-world nesting:
  - an end tag closes the nearest open element with the same name, so `<div><p>text</div>` also closes the `<p>`;
  - stray end tags (`</span>` with nothing open, `</br>`) are ignored;
  - `<li>a<li>b`, `<p>a<p>b`, `<td>`, `<tr>`, `<option>`, `<dt>`, `<dd>` close the previous one of the same kind.
- Nested matches: with `div` on `<div>a<div>b</div></div>` you get both, the inner one (`b`) first, then the outer (`ab`).

**What you get back**

- A counter of matches (fastest: no callback at all), plus a few statistics.
- Or a callback per match with the element's text content (tags removed).

---

## Quick start

### Add it to a CMake project

```cmake
include(FetchContent)
FetchContent_Declare(fasthtml
    GIT_REPOSITORY https://github.com/Martin-Kaze/FastHTMLparse.git
    GIT_TAG        v1.0.0)
FetchContent_MakeAvailable(fasthtml)

target_link_libraries(my_app PRIVATE fasthtml::fasthtml)
```

Or copy the folder into your project and use `add_subdirectory(fasthtml)`.
As a sub-project only the library is built (no tests, no downloads).

### Use it

```cpp
#include <fasthtml/fasthtml.hpp>
#include <iostream>

int main()
{
    std::string html = R"(<ul><li class="x">one</li><li>two</li><li class="x">three</li></ul>)";

    // one line
    std::cout << fasthtml::count(html, "li.x") << "\n";             // 2
    for (const auto &text : fasthtml::extract_text(html, "ul > li"))
        std::cout << text << "\n";                                   // one, two, three

    // streaming: compile once, feed pieces, get a callback per match
    std::string error;
    auto selector = fasthtml::Selector::compile("li.x", &error);
    if (!selector) { std::cerr << error << "\n"; return 1; }

    fasthtml::Matcher matcher(*selector);
    auto print = [](std::string_view text) { std::cout << "match: " << text << "\n"; };
    matcher.on_match(print);              // the lambda must outlive the matcher

    matcher.feed(html.substr(0, 20));     // pieces may be split anywhere
    matcher.feed(html.substr(20));
    matcher.finish();                     // end of document

    std::cout << matcher.match_count() << " matches\n";
}
```

### API in short

| | |
|---|---|
| `Selector::compile(text, &error)` | returns `std::optional<Selector>`; empty if invalid, `error` says why |
| `Matcher m(selector)` | one matcher per document stream (and per thread) |
| `m.on_match(fn, user)` | `void fn(std::string_view text, void *user)`, or any lambda via `m.on_match(lambda)` |
| `m.feed(data, size)` / `m.feed(view)` | the next piece of the document |
| `m.finish()` | end of document: also reports matches that were never closed |
| `m.reset()` | start a new document with the same matcher |
| `m.match_count()`, `m.stats()` | results so far |
| `count(html, sel)`, `extract_text(html, sel)` | one-call helpers (throw `std::invalid_argument` for a bad selector string) |

Good to know:

- The text passed to the callback is only valid during the call; copy it if you keep it.
- A `Selector` is immutable and can be shared by many matchers and threads. A `Matcher` must not be used from two threads at once.
- Void elements (`<img>`, `<br>`, …) have no content, their callback text is `""`.

---

## Limits (by design, to stay fast)

- No DOM tree, no JavaScript, no CSS layout. It matches and extracts, that is all.
- Selectors: no `*`, `[attr=value]`, `:pseudo-classes`, `,` lists, `+` / `~`, and at most one class per part (`.a.b` is rejected).
- Text is returned as written: entities like `&amp;` are not decoded, and whitespace is kept.
- Not a full HTML5 tree builder: the common cases of unclosed and stray tags are handled (see above),
  rare ones such as adoption-agency rules or `<table>` foster-parenting are not.
- `<textarea>` and `<title>` content is parsed like normal HTML (only `script` and `style` are raw).
- Nesting is tracked up to 1024 levels; deeper elements still match, but their callback text is empty.
- Bytes in, bytes out: UTF-8 passes through untouched, other encodings are not converted.

---

## Performance

Test: a 139 MB file (one Wikipedia page repeated 100 times), fed in 64 KB pieces, best of 7 runs,
Ryzen 5 5600H, Windows, Clang (MSYS2 clang64), `-O3 -march=native`, LTO, "Best performance" power mode.

| Version | What changed | Speed |
|---|---|---|
| V1 | first state machine | ~150 MB/s * |
| V2 | bitmask matching, templates per selector type, `memchr` for text and skipped values | ~880–910 MB/s |
| v3a | whole tag name read at once, then compared | ~970 MB/s |
| v3b | attribute names read at once | ~960 MB/s |
| v3c | `<script>`/`<style>` content skipped with `memchr` | ~1340 MB/s |
| v3d | a whole tag in one loop pass (no state hand-offs) | **~1500–1610 MB/s** |
| 1.0 | v3d + correctness fixes + library API | ~1450–1550 MB/s (0–5% below v3d) |

\* measured in a power-saving mode, so not directly comparable.

Run it yourself:

```powershell
cmake -B build -DCMAKE_BUILD_TYPE=Release -DFASTHTML_NATIVE=ON
cmake --build build -j
.\bench\bench.ps1 big.html div some-id some-class -Exe .\build\fasthtml_bench.exe | Tee-Object new.txt
.\bench\compare.ps1 old.txt new.txt
```

On Linux/macOS: `./bench/bench.sh big.html div some-id some-class ./build/fasthtml_bench`.

`fasthtml_state_stats file.html "div"` shows how many bytes and loop passes each parser state takes,
which is the best guide for the next optimization.

Benchmark tips: use the same file, the same power mode and a quiet machine; differences under ~10% are noise.

### Compared with other libraries

Same machine, same 139 MB file, MB/s for parse + match (higher is better). Every library reported exactly
the same match count before its time was used.

| Selector | fasthtml | lol-html | lexbor | selectolax | html5ever + scraper |
|---|---:|---:|---:|---:|---:|
| `div` | **1,503** | 1,048 | 159 | 166 | 60 |
| `a` | **1,486** | 801 | 162 | 163 | 61 |
| `a.external` | **1,359** | 746 | 162 | 166 | 59 |
| `div .mw-heading` | **1,210** | 403 | 162 | 167 | 57 |

- **lol-html** (Cloudflare, Rust) is the closest match: it also streams and matches selectors without a tree.
  fasthtml is 1.4–3× faster than it.
- **lexbor, selectolax, html5ever, lxml** build a full HTML5 DOM you can query many times. That is much more work,
  so being ~9–25× faster than them is expected and is not a like-for-like contest.
- lxml (libxml2) only reads the first of the 100 joined pages, so it is compared on a single page only (~50 MB/s).

All selectors, the single-page results, versions and how to re-run: [`bench/OTHER_PARSERS.md`](bench/OTHER_PARSERS.md).

---

## How it works (short version)

- **One pass, no tree.** A small state machine reads the bytes once. Only the elements that are still open are remembered.
- **Bitmasks.** Each selector part is one bit. "Which parts could this element still match?" is a single integer,
  updated for all parts at once. Parent/ancestor relations are a bit shift of the parent's mask.
- **Skip what does not matter.** Text between tags, attribute values the selector does not ask about,
  comments and script/style content are jumped over with `memchr`, which checks 16–32 bytes per instruction.
- **Specialized loops.** The parser is compiled 8 times (with/without id, class, relations); each selector
  runs the version without the code it does not need.
- **One loop pass per tag.** A whole tag with all its attributes is read in one go, falling from step to step,
  instead of going around the main loop for every small step.

---

## Ideas for improvement

Roughly in order of expected benefit:

1. **Compare id/class values in one step.** Values the selector cares about are still checked one character
   at a time; finding the closing quote with `memchr` and comparing the whole value would be faster
   for selectors like `div.item`.
2. **Faster tag recognition.** Turn the first 8 bytes of a tag name into one number and compare numbers,
   instead of comparing strings against the void/raw tag lists on every tag.
3. **A lookup table for "is this a delimiter".** Name scanning checks 5–8 characters per byte; one table lookup would do.
4. **Own SIMD scanning.** Search for `<`, `>`, quotes and spaces 16 or 32 bytes at a time without the
   call overhead of `memchr`, which matters for the many short attribute values.
5. **Profile-guided optimization (PGO).** Let the compiler optimize using a real run; often +5–15%.
6. **More selectors.** `[attr]`, `[attr=value]`, several classes per part, `*`, `,` lists.
7. **More HTML.** Decode entities in the returned text, treat `<textarea>`/`<title>` as raw text,
   more implied end tags (`<p>` closed by block elements).
8. **Several selectors in one pass.** Up to 32 parts fit in one bitmask, so a few selectors could share a single read of the document.
9. **Faster descendant selectors.** `div .mw-heading` runs at ~1210 MB/s against ~1500 for `div > div`;
   it is still 3× lol-html, but it is the slowest case.

---

## Building and testing

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Options (all `ON`/`OFF`):

| Option | Default | |
|---|---|---|
| `FASTHTML_BUILD_TESTS` | on when top-level | unit tests (downloads googletest) |
| `FASTHTML_BUILD_BENCH` | on when top-level | `fasthtml_bench`, `fasthtml_state_stats` |
| `FASTHTML_BUILD_EXAMPLES` | on when top-level | `fasthtml_count`, `fasthtml_extract_text` |
| `FASTHTML_NATIVE` | off | `-march=native` (`/arch:AVX2` on MSVC): fastest, but the binary needs a similar CPU |
| `FASTHTML_LTO` | on when top-level | link-time optimization of this project's targets |
| `FASTHTML_INSTALL` | on when top-level | `cmake --install` + `find_package(fasthtml)` |

Examples: `fasthtml_count examples/sample.html "p.text"` and `fasthtml_extract_text examples/sample.html "p.text"`.

Project layout:

```
include/fasthtml/fasthtml.hpp   public API (the only header users include)
src/                            engine (state machine), selector compiler and lexer, API glue
tests/                          unit tests
bench/                          benchmark tools and scripts
examples/                       small command-line programs
```

---

## License

MIT, see [LICENSE](LICENSE).
