# Comparison harness: python py_bench.py <selectolax|lxml> <file.html> <selector>
# Same output format as lexbor_bench. Best of 3 (these are slow on a 139 MB file).
# Needs: pip install selectolax lxml cssselect
import sys
import time

RUNS = 3


def selectolax_run(data, sel):
    from selectolax.lexbor import LexborHTMLParser

    t0 = time.perf_counter()
    tree = LexborHTMLParser(data)
    t1 = time.perf_counter()
    n = len(tree.css(sel))
    t2 = time.perf_counter()
    return n, t1 - t0, t2 - t0


def lxml_run(data, sel):
    import lxml.html
    from lxml.cssselect import CSSSelector

    match = CSSSelector(sel)
    t0 = time.perf_counter()
    root = lxml.html.document_fromstring(data)
    t1 = time.perf_counter()
    n = len(match(root))
    t2 = time.perf_counter()
    return n, t1 - t0, t2 - t0


def main():
    if len(sys.argv) < 4:
        print(f"usage: {sys.argv[0]} <selectolax|lxml> <file.html> <selector>", file=sys.stderr)
        return 2
    lib, path, sel = sys.argv[1:4]
    run = {"selectolax": selectolax_run, "lxml": lxml_run}[lib]
    with open(path, "rb") as f:
        data = f.read()

    best_parse = best_total = float("inf")
    count = 0
    for _ in range(RUNS):
        count, parse, total = run(data, sel)
        best_parse = min(best_parse, parse)
        best_total = min(best_total, total)

    print(f"library:   {lib}")
    print(f"selector:  {sel}")
    print(f"selectors: {count}")
    print(f"parse ms:  {best_parse * 1e3:.1f}")
    print(f"match ms:  {(best_total - best_parse) * 1e3:.1f}")
    print(f"speed:     {len(data) / 1e6 / best_total:.1f} MB/s")
    return 0


if __name__ == "__main__":
    sys.exit(main())
