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

#ifndef FASTCHART_GRAPH_H
#define FASTCHART_GRAPH_H

#include "php.h"

/* Shared node/edge data model for the graph chart family (ArcDiagram,
 * ChordDiagram, NetworkChart). Same shape SankeyChart parses inline;
 * these charts share the parse/free/clone helpers below instead of
 * duplicating the logic per class. SankeyChart predates this header
 * and keeps its own copy — folding it in is a later dedupe. */

#define FASTCHART_MAX_GRAPH_NODES   512    /* per chart */
#define FASTCHART_MAX_GRAPH_LINKS   2048   /* per chart */

typedef struct {
    char *label;          /* emalloc'd, NUL-terminated; NULL = no label */
    int   color_rgb;      /* -1 = palette default */
} fastchart_graph_node;

typedef struct {
    int    from;          /* node index, 0-based */
    int    to;            /* node index, 0-based */
    double value;         /* > 0 */
} fastchart_graph_link;

/* Parse a PHP node array (`[['label' => string?, 'color' => int?], ...]`)
 * into a freshly emalloc'd fastchart_graph_node[]. Caps element count at
 * `max`. A non-array entry becomes an unlabeled palette-colored node so
 * indices stay aligned with the caller's input. On success writes *out /
 * *count and returns 0; an empty input yields *count == 0 and *out NULL. */
int fastchart_graph_parse_nodes(zval *arr, int max,
                                fastchart_graph_node **out, int *count);

/* Parse a PHP link array (`[['from' => int, 'to' => int, 'value' => num],
 * ...]`) into a freshly emalloc'd fastchart_graph_link[]. Drops entries
 * that are malformed, reference an out-of-range index (>= node_count),
 * are self-loops, or carry a non-finite / non-positive value. Caps at
 * `max`. Returns 0 with *count possibly 0 (and *out NULL). */
int fastchart_graph_parse_links(zval *arr, int node_count, int max,
                                fastchart_graph_link **out, int *count);

void fastchart_graph_free_nodes(fastchart_graph_node *nodes, int count);
void fastchart_graph_free_links(fastchart_graph_link *links);

/* Deep-copy helpers for clone_object (addref) handlers. Return NULL when
 * count <= 0 or the source pointer is NULL. */
fastchart_graph_node *fastchart_graph_clone_nodes(
    const fastchart_graph_node *nodes, int count);
fastchart_graph_link *fastchart_graph_clone_links(
    const fastchart_graph_link *links, int count);

#endif /* FASTCHART_GRAPH_H */
