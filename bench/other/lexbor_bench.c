// lexbor comparison: lexbor_bench <file.html> <selector>
// Times parse (full HTML5 DOM) + selector search separately, best of 7.
#include <lexbor/html/html.h>
#include <lexbor/css/css.h>
#include <lexbor/selectors/selectors.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define RUNS 7

static double now(void)
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC); // portable C11 (clock_gettime is missing on some Windows toolchains)
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static lxb_status_t count_cb(lxb_dom_node_t *node, lxb_css_selector_specificity_t spec, void *ctx)
{
    (void)node;
    (void)spec;
    ++*(size_t *)ctx;
    return LXB_STATUS_OK;
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "usage: %s <file.html> <selector>\n", argv[0]);
        return 2;
    }
    FILE *f = fopen(argv[1], "rb");
    if (!f)
    {
        fprintf(stderr, "FILE NOT OPEN: %s\n", argv[1]);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size_t size = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    lxb_char_t *data = malloc(size);
    if (fread(data, 1, size, f) != size)
        return 1;
    fclose(f);

    const lxb_char_t *sel = (const lxb_char_t *)argv[2];

    lxb_css_parser_t *parser = lxb_css_parser_create();
    lxb_css_parser_init(parser, NULL);
    lxb_selectors_t *selectors = lxb_selectors_create();
    lxb_selectors_init(selectors);
    lxb_css_selector_list_t *list = lxb_css_selectors_parse(parser, sel, strlen(argv[2]));
    if (list == NULL)
    {
        fprintf(stderr, "BAD SELECTOR\n");
        return 1;
    }

    double best_parse = 1e9, best_match = 1e9, best_total = 1e9;
    size_t count = 0;
    for (int r = 0; r < RUNS; ++r)
    {
        lxb_html_document_t *doc = lxb_html_document_create();
        double t0 = now();
        if (lxb_html_document_parse(doc, data, size) != LXB_STATUS_OK)
            return 1;
        double t1 = now();
        count = 0;
        lxb_selectors_find(selectors, lxb_dom_interface_node(doc), list, count_cb, &count);
        double t2 = now();
        lxb_html_document_destroy(doc);

        if (t1 - t0 < best_parse) best_parse = t1 - t0;
        if (t2 - t1 < best_match) best_match = t2 - t1;
        if (t2 - t0 < best_total) best_total = t2 - t0;
    }

    printf("library:   lexbor\n"
           "selector:  %s\n"
           "selectors: %zu\n"
           "parse ms:  %.1f\n"
           "match ms:  %.1f\n"
           "speed:     %.1f MB/s\n",
           argv[2], count, best_parse * 1e3, best_match * 1e3, size / 1e6 / best_total);
    return 0;
}
