/*
 * Copyright (C) 2016 Mario Cianciolo <mr.udda@gmail.com>
 *                    The Free Software Foundation
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

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "colors.h"
#include "gtk-compat.h"
#include "multiload.h"
#include "multiload-colors.h"
#include "multiload-config.h"
#include "preferences.h"
#include "util.h"


static void
gdk_rgba_to_argb_string(GdkRGBA* color, gchar *out_str)
{
	// note: out_str must be at least 10 characters long
	int rc = snprintf(out_str, 10, "#%02X%02X%02X%02X",
					(guint8)(color->alpha * 255),
					(guint8)(color->red * 255),
					(guint8)(color->green * 255),
					(guint8)(color->blue * 255));
	g_assert(rc == 9);
}

static gboolean
argb_string_to_gdk_rgba(const gchar *gspec, GdkRGBA *color)
{
	gchar buf[8];
	guint16 alpha;
	gboolean ret;

	if (strlen(gspec) == 7) {
		// may be a standard RGB hex string, fallback to standard parse
		return gdk_rgba_parse(color, gspec);
	} else if (G_UNLIKELY (strlen(gspec) != 9) ) {
		return FALSE;
	}

	// alpha part
	buf[0] = gspec[1];
	buf[1] = gspec[2];
	buf[2] = 0;
	errno = 0;
	alpha = (guint16)strtol(buf, NULL, 16);
	if (errno)
		alpha = 0xFF; // error in strtol, set alpha=max

	// color part
	buf[0] = '#';
	strncpy(buf+1, gspec+3, 6);
	buf[7] = 0;

	ret = gdk_rgba_parse(color, buf);
	if (ret)
		color->alpha = ((gdouble)alpha)/255.0f;

	return ret;
}


guint multiload_colors_get_extra_index(guint i, MultiloadExtraColor col)
{
	g_assert(col >= 0 && col < EXTRA_COLORS);
	return multiload_config_get_num_colors(i) - EXTRA_COLORS + col;
}


/* Convert graph configuration into a string of the form "#aarrggbb,#aarrggbb,..."
   Output string must have size at least 10*MAX_COLORS. */
void
multiload_colors_stringify(MultiloadPlugin *ma, guint i, char *list)
{
	guint ncolors = multiload_config_get_num_colors(i);
	guint j;
	GdkRGBA *colors = ma->graph_config[i].colors;
	char *listpos = list;

	if ( G_UNLIKELY (!list) )
		return;

	// Create color list
	for ( j = 0; j < ncolors; j++ ) {
		gdk_rgba_to_argb_string(&colors[j], listpos);
		if ( j == ncolors-1 )
			listpos[9] = 0;
		else
			listpos[9] = ',';
		listpos += 10;
	}
	g_assert (strlen(list) == 10*ncolors-1);
}


/* Set the colors for graph i to the default values */
void
multiload_colors_default(MultiloadPlugin *ma, guint i)
{
	multiload_color_scheme_apply_single(&multiload_builtin_color_schemes[0], ma, i);
}

/* Set graph colors from a string, as produced by multiload_colors_stringify */
gboolean
multiload_colors_unstringify(MultiloadPlugin *ma, guint i, const char *list)
{
	guint ncolors = multiload_config_get_num_colors(i);
	guint j;
	GdkRGBA *colors = ma->graph_config[i].colors;
	const char *listpos = list;

	if ( G_UNLIKELY (!listpos) ) {
		multiload_colors_default(ma, i);
		return FALSE;
	}

	for ( j = 0; j < ncolors; j++ ) {
		/* Check the length of the list item. */
		int pos = 0;
		if ( j == ncolors-1 )
			pos = strlen(listpos);
		else
			pos = (int)(strchr(listpos, ',')-listpos);

		/* Try to parse the color */
		if ( G_UNLIKELY (pos != 9) ) {
			multiload_colors_default(ma, i);
			return FALSE;
		}

		/* Extract the color into a null-terminated buffer */
		char buf[10];
		strncpy(buf, listpos, 9);
		buf[9] = 0;
		if ( G_UNLIKELY (argb_string_to_gdk_rgba(buf, &colors[j]) != TRUE) ) {
			multiload_colors_default(ma, i);
			return FALSE;
		}

		listpos += 10;
	}

	//ignore alpha value of last two colors (background and border)
	colors[ncolors-1].alpha = 1;
	colors[ncolors-2].alpha = 1;

	return TRUE;
}
