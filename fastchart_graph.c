/*
  +----------------------------------------------------------------------+
  | Copyright (c) 2025-2026, Ilia Alshanetsky                            |
  | Copyright (c) 2025-2026, Advanced Internet Designs Inc.              |
  +----------------------------------------------------------------------+
  | This source file is subject to the BSD 3-Clause license that is      |
  | bundled with this package in the file LICENSE.                       |
  +----------------------------------------------------------------------+
  | Author: Ilia Alshanetsky <ilia@ilia.ws>                              |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <string.h>

#include "php.h"

#include "php_fastchart.h"
#include "fastchart_graph.h"
#include "fastchart_axis.h"

int fastchart_graph_validate_node_labels(zval *arr, const char *method)
{
    size_t total = 0;
    zval *entry;
    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(arr), entry) {
        if (entry) ZVAL_DEREF(entry);
        if (Z_TYPE_P(entry) != IS_ARRAY) continue;
        zval *label = zend_hash_str_find(Z_ARRVAL_P(entry),
            "label", sizeof("label") - 1);
        if (label) ZVAL_DEREF(label);
        if (!label || Z_TYPE_P(label) != IS_STRING
            || Z_STRLEN_P(label) > FASTCHART_MAX_TEXT_BYTES
            || memchr(Z_STRVAL_P(label), 0, Z_STRLEN_P(label)) != NULL) {
            continue;
        }
        if (Z_STRLEN_P(label) > FASTCHART_MAX_GRAPH_LABEL_BYTES - total) {
            zend_value_error("%s labels exceed the %u-byte aggregate limit",
                method, (unsigned)FASTCHART_MAX_GRAPH_LABEL_BYTES);
            return -1;
        }
        total += Z_STRLEN_P(label);
    } ZEND_HASH_FOREACH_END();
    return 0;
}

int fastchart_graph_parse_nodes(zval *arr, int max,
                                fastchart_graph_node **out, int *count)
{
    *out = NULL;
    *count = 0;

    HashTable *ht = Z_ARRVAL_P(arr);
    int n = zend_hash_num_elements(ht);
    if (n > max) n = max;
    if (n <= 0) return 0;

    fastchart_graph_node *parsed = ecalloc(n, sizeof(*parsed));
    int kept = 0;
    zval *entry;
    ZEND_HASH_FOREACH_VAL(ht, entry) {
        if (kept >= n) break;
        if (entry) ZVAL_DEREF(entry);
        if (Z_TYPE_P(entry) != IS_ARRAY) {
            parsed[kept].label = NULL;
            parsed[kept].color_rgb = -1;
            kept++;
            continue;
        }
        HashTable *eht = Z_ARRVAL_P(entry);
        const char *lbl = fastchart_label_or_null(
            zend_hash_str_find(eht, "label", sizeof("label") - 1));
        parsed[kept].label = lbl ? estrdup(lbl) : NULL;
        parsed[kept].color_rgb = -1;
        zval *zc = zend_hash_str_find(eht, "color", sizeof("color") - 1);
        if (zc) ZVAL_DEREF(zc);
        if (zc && Z_TYPE_P(zc) == IS_LONG) {
            zend_long c = Z_LVAL_P(zc);
            if (c >= 0 && c <= 0xFFFFFF) parsed[kept].color_rgb = (int)c;
        }
        kept++;
    } ZEND_HASH_FOREACH_END();

    *out = parsed;
    *count = kept;
    return 0;
}

int fastchart_graph_parse_links(zval *arr, int node_count, int max,
                                fastchart_graph_link **out, int *count)
{
    *out = NULL;
    *count = 0;

    HashTable *ht = Z_ARRVAL_P(arr);
    int n = zend_hash_num_elements(ht);
    if (n > max) n = max;
    if (n <= 0) return 0;

    fastchart_graph_link *parsed = ecalloc(n, sizeof(*parsed));
    int kept = 0;
    zval *entry;
    ZEND_HASH_FOREACH_VAL(ht, entry) {
        if (kept >= n) break;
        if (entry) ZVAL_DEREF(entry);
        if (Z_TYPE_P(entry) != IS_ARRAY) continue;
        HashTable *eht = Z_ARRVAL_P(entry);
        zval *zf = zend_hash_str_find(eht, "from",  sizeof("from")  - 1);
        zval *zt = zend_hash_str_find(eht, "to",    sizeof("to")    - 1);
        zval *zv = zend_hash_str_find(eht, "value", sizeof("value") - 1);
        if (!zf || !zt || !zv) continue;
        ZVAL_DEREF(zf);
        ZVAL_DEREF(zt);
        if (Z_TYPE_P(zf) != IS_LONG || Z_TYPE_P(zt) != IS_LONG) continue;
        zend_long from = Z_LVAL_P(zf), to = Z_LVAL_P(zt);
        double val;
        if (fastchart_zval_to_double(zv, &val) != 0 || !isfinite(val) ||
            val <= 0 || val > FASTCHART_MAX_DATA_MAG) {
            continue;
        }
        if (from < 0 || from >= node_count) continue;
        if (to   < 0 || to   >= node_count) continue;
        if (from == to) continue;
        parsed[kept].from = (int)from;
        parsed[kept].to   = (int)to;
        parsed[kept].value = val;
        kept++;
    } ZEND_HASH_FOREACH_END();

    if (kept == 0) { efree(parsed); return 0; }
    *out = parsed;
    *count = kept;
    return 0;
}

void fastchart_graph_free_nodes(fastchart_graph_node *nodes, int count)
{
    if (!nodes) return;
    for (int i = 0; i < count; i++) {
        if (nodes[i].label) efree(nodes[i].label);
    }
    efree(nodes);
}

void fastchart_graph_free_links(fastchart_graph_link *links)
{
    if (links) efree(links);
}

fastchart_graph_node *fastchart_graph_clone_nodes(
    const fastchart_graph_node *nodes, int count)
{
    if (!nodes || count <= 0) return NULL;
    fastchart_graph_node *copy = emalloc(sizeof(*copy) * count);
    for (int i = 0; i < count; i++) {
        copy[i] = nodes[i];
        copy[i].label = nodes[i].label ? estrdup(nodes[i].label) : NULL;
    }
    return copy;
}

fastchart_graph_link *fastchart_graph_clone_links(
    const fastchart_graph_link *links, int count)
{
    if (!links || count <= 0) return NULL;
    size_t bytes = (size_t)count * sizeof(*links);
    fastchart_graph_link *copy = emalloc(bytes);
    memcpy(copy, links, bytes);
    return copy;
}

void fastchart_graph_fields_release(fastchart_graph_node **nodes, int *ncount,
                                    fastchart_graph_link **links, int *lcount)
{
    fastchart_graph_free_nodes(*nodes, *ncount);
    *nodes = NULL;
    *ncount = 0;
    fastchart_graph_free_links(*links);
    *links = NULL;
    *lcount = 0;
}

void fastchart_graph_fields_addref(fastchart_graph_node **nodes, int ncount,
                                   fastchart_graph_link **links, int lcount)
{
    *nodes = fastchart_graph_clone_nodes(*nodes, ncount);
    *links = fastchart_graph_clone_links(*links, lcount);
}

void fastchart_graph_fields_set_nodes(fastchart_graph_node **nodes, int *ncount,
                                      fastchart_graph_link **links, int *lcount,
                                      zval *arr)
{
    fastchart_graph_fields_release(nodes, ncount, links, lcount);
    fastchart_graph_parse_nodes(arr, FASTCHART_MAX_GRAPH_NODES, nodes, ncount);
}

void fastchart_graph_fields_set_links(int node_count,
                                      fastchart_graph_link **links, int *lcount,
                                      zval *arr)
{
    fastchart_graph_free_links(*links);
    *links = NULL;
    *lcount = 0;
    fastchart_graph_parse_links(arr, node_count, FASTCHART_MAX_GRAPH_LINKS,
                                links, lcount);
}
