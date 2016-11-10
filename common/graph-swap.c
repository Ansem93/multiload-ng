/*
 * Copyright (C) 2016 Mario Cianciolo <mr.udda@gmail.com>
 *
 * This file is part of multiload-ng.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#include <config.h>

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>

#include "graph-data.h"
#include "info-file.h"
#include "preferences.h"
#include "util.h"

#define PATH_MEMINFO "/proc/meminfo"

void
multiload_graph_swap_get_data (int Maximum, int data [1], LoadGraph *g, SwapData *xd)
{
	// displayed keys
	static guint64 kb_swap_total = 0;
	static guint64 kb_swap_free = 0;

	static const InfoFileMappingEntry table[] = {
		{ "SwapTotal",	'u',	&kb_swap_total },
		{ "SwapFree",	'u',	&kb_swap_free }
	};

	gint r = info_file_read_keys (PATH_MEMINFO, table, 2);
	g_assert_cmpint(r, ==, 2);

	xd->used = (kb_swap_total - kb_swap_free) * 1024;
	xd->total = kb_swap_total * 1024;

	if (kb_swap_total == 0)
	   data [0] = 0;
	else
	   data [0] = rint (Maximum * (float)(kb_swap_total - kb_swap_free) / kb_swap_total);
}


void
multiload_graph_swap_cmdline_output (LoadGraph *g, SwapData *xd)
{
	g_snprintf(g->output_str[0], sizeof(g->output_str[0]), "%"G_GUINT64_FORMAT, xd->used);
}


void
multiload_graph_swap_tooltip_update (char **title, char **text, LoadGraph *g, SwapData *xd)
{
	if (xd->total == 0) {
		*text = g_strdup_printf(_("No swap"));
	} else {
		gchar *used = format_size_for_display(xd->used, g->multiload->size_format_iec);
		gchar *used_percent = format_percent(xd->used, xd->total, 0);

		gchar *total = format_size_for_display(xd->total, g->multiload->size_format_iec);

		if (g->config->tooltip_style == MULTILOAD_TOOLTIP_STYLE_DETAILED) {
			*title = g_strdup_printf(_("%s of swap"), total);
			*text = g_strdup_printf(_("%s (%s) used"), used_percent, used);
		} else {
			*text = g_strdup_printf("%s", used_percent);
		}

		g_free(used);
		g_free(used_percent);
		g_free(total);
	}
}
