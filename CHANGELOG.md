# Changelog

## 1.0.0

First release as a library.

### Library
- Public API in `include/fasthtml/fasthtml.hpp`, namespace `fasthtml`:
  `Selector::compile`, `Matcher` (`on_match`, `feed`, `finish`, `reset`, `match_count`, `stats`),
  and the helpers `count` / `extract_text`.
- Internals moved to `src/` under `fasthtml::detail` and renamed:
  `Parser` -> `Engine`, `SelectorParser` -> `SelectorCompiler`, `parsring` -> `feed` / `compile`, typos fixed.
- CMake library target `fasthtml::fasthtml`, options for tests, bench, examples, `-march=native`, LTO, install.

### Fixed (HTML)
- `<!-->` and `<!--->` now close the comment.
- `</scriptx` no longer ends a `<script>`; the end tag needs `>`, `/` or whitespace after the name.
- `</>` is ignored; `</3>` and `</ x>` are skipped as bogus comments.
- Nested matches each get their own text (inner first). Before, an inner match dropped the outer one's text.
- End tags close the nearest open element with the same name (up to 32 levels down);
  unclosed elements inside are closed too, stray end tags are ignored. Before, every end tag closed one level,
  so unclosed tags made the depth drift on real pages.
- Implied end tags for `p`, `li`, `dt`, `dd`, `option`, `tr`, `td`, `th` of the same kind.
- `finish()` reports matched elements that were never closed.
- Elements nested deeper than 1024 levels still match (with empty text) instead of corrupting the stack.

### Fixed (selectors)
- `div .x`, `#a .b`, `.a .b` are descendant selectors (the space was lost before, giving `div.x`).
- Tabs and newlines count as whitespace.
- Tag names in selectors are lowercased (`DIV` matches `<div>`).
- Reusing a lexer or compiler no longer keeps old tokens or relation masks.
- The selector text is copied; the caller's string may change or go away afterwards.
- Invalid selectors are rejected with an error message: `div#`, `div.`, `div >`, `> div`, `div > > a`,
  `.a.b`, `#a#b`, `*`, `a:hover`, `a,b`, `[x]`, `a+b`, more than 16 parts or 32 tokens.

### Faster
- Comment and `<!DOCTYPE>` content is skipped with `memchr`.
- Void/raw tag detection happens once per tag (no second loop at the end of the tag).

## Before 1.0 (branch `v3`)

| Step | Change | Speed (139 MB page) |
|---|---|---|
| V2 | bitmask matching, templates, `memchr` skips | ~880–910 MB/s |
| v3a | tag name scanned at once, compared once | ~970 MB/s |
| v3b | attribute names scanned at once (+ `id = "x"`, `ID=` fixes) | ~960 MB/s |
| v3c | `<script>` / `<style>` content skipped with `memchr` | ~1340 MB/s |
| v3d | fallthrough chain: a whole tag in one loop pass | ~1500–1610 MB/s |
