#ifndef LOAD_GRAPH_H__
#define LOAD_GRAPH_H__

#include "multiload.h"

/* Create new load graph. */
G_GNUC_INTERNAL LoadGraph *
load_graph_new (MultiloadPlugin *ma, guint id);

/* (Re)size load graph. */
G_GNUC_INTERNAL void
load_graph_resize (LoadGraph *g);

/* Start load graph. */
G_GNUC_INTERNAL void
load_graph_start (LoadGraph *g);

/* Stop load graph. */
G_GNUC_INTERNAL void
load_graph_stop (LoadGraph *g);

/* free load graph */
G_GNUC_INTERNAL void
load_graph_unalloc (LoadGraph *g);

#endif
