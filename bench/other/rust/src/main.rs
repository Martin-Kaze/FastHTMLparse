// Comparison harness: other_bench <lol_html|scraper> <file.html> <selector>
// Same output format as lexbor_bench: count + best of 7.
use std::cell::Cell;
use std::time::Instant;

const RUNS: usize = 7;
const CHUNK: usize = 64 * 1024; // like fasthtml_bench

fn lol_html_run(data: &[u8], sel: &str) -> (usize, f64, f64) {
    let count = Cell::new(0usize);
    let t0 = Instant::now();
    let mut rewriter = lol_html::HtmlRewriter::new(
        lol_html::Settings::new().append_element_content_handler(lol_html::element!(sel, |_el| {
            count.set(count.get() + 1);
            Ok(())
        })),
        |_: &[u8]| {}, // lol-html is a rewriter: it always produces output, we drop it
    );
    for chunk in data.chunks(CHUNK) {
        rewriter.write(chunk).expect("write");
    }
    rewriter.end().expect("end");
    let total = t0.elapsed().as_secs_f64();
    (count.get(), 0.0, total)
}

fn scraper_run(text: &str, sel: &str) -> (usize, f64, f64) {
    let selector = scraper::Selector::parse(sel).expect("bad selector");
    let t0 = Instant::now();
    let doc = scraper::Html::parse_document(text);
    let t1 = Instant::now();
    let n = doc.select(&selector).count();
    let t2 = Instant::now();
    let parse = (t1 - t0).as_secs_f64();
    let total = (t2 - t0).as_secs_f64();
    drop(doc); // freeing the tree is not timed (same as lexbor_bench)
    (n, parse, total)
}

fn main() {
    let args: Vec<String> = std::env::args().collect();
    if args.len() < 4 {
        eprintln!("usage: {} <lol_html|scraper> <file.html> <selector>", args[0]);
        std::process::exit(2);
    }
    let (lib, path, sel) = (args[1].as_str(), &args[2], args[3].as_str());
    let data = std::fs::read(path).expect("FILE NOT OPEN");
    let text = String::from_utf8_lossy(&data).into_owned();

    let (mut count, mut best_parse, mut best_total) = (0, f64::MAX, f64::MAX);
    for _ in 0..RUNS {
        let (n, parse, total) = match lib {
            "lol_html" => lol_html_run(&data, sel),
            "scraper" => scraper_run(&text, sel),
            _ => panic!("unknown library {lib}"),
        };
        count = n;
        best_parse = best_parse.min(parse);
        best_total = best_total.min(total);
    }

    println!("library:   {lib}");
    println!("selector:  {sel}");
    println!("selectors: {count}");
    if lib == "scraper" {
        println!("parse ms:  {:.1}", best_parse * 1e3);
        println!("match ms:  {:.1}", (best_total - best_parse) * 1e3);
    }
    println!("speed:     {:.1} MB/s", data.len() as f64 / 1e6 / best_total);
}
