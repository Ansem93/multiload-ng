/*
 * Copyright (C) 2016 Mario Cianciolo <mr.udda@gmail.com>
 *               1997 The Free Software Foundation
 *                    (Authors: Tim P. Gerla, Martin Baulig, Todd Kulesza)
 *
 * With code from wmload.c, v0.9.2, apparently by Ryan Land, rland@bc1.com.
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

#include <glibtop.h>
#include <glib/gi18n-lib.h>

#include "gtk-compat.h"
#include "linux-proc.h"
#include "multiload.h"
#include "multiload-config.h"
#include "multiload-colors.h"
#include "preferences.h"
#include "util.h"


const char* MULTILOAD_CONFIG_PATH;

/* update the tooltip to the graph's current "used" percentage */
void
multiload_tooltip_update(LoadGraph *g)
{
	gchar *title = NULL;
	gchar *text;
	gchar *tooltip_markup;

	g_assert_nonnull(g);
	g_assert ( g->id >= 0 && g->id < GRAPH_MAX );

	switch (g->id) {
		case GRAPH_CPULOAD: {
			CpuData *xd = (CpuData*) g->extra_data;
			g_assert_nonnull(xd);


			if (g->config->tooltip_style == TOOLTIP_STYLE_DETAILS) {
				gchar *uptime = format_time_duration(xd->uptime);
				title = g_strdup(xd->cpu0_name);
				text = g_strdup_printf(_(	"%lu processors  -  %.2f GHz  -  Governor: %s\n"
											"%.1f%% in use by programs\n"
											"%.1f%% in wait for I/O\n"
											"%.1f%% total CPU use\n"
											"\n"
											"Uptime: %s"),
											xd->num_cpu, xd->cpu0_mhz/1000.0, xd->cpu0_governor,
											(xd->user*100),
											(xd->iowait*100),
											(xd->total_use*100),
											uptime);
				g_free(uptime);
			} else {
				text = g_strdup_printf("%.1f%%", xd->total_use*100);
			}
		}	break;

		case GRAPH_MEMLOAD: {
			MemoryData *xd = (MemoryData*) g->extra_data;
			g_assert_nonnull(xd);


			if (g->config->tooltip_style == TOOLTIP_STYLE_DETAILS) {
				gchar *total = g_format_size_full(xd->total, G_FORMAT_SIZE_IEC_UNITS);
				gchar *user = format_percent(xd->user, xd->total, 1);
				gchar *cache = format_percent(xd->cache, xd->total, 1);
				title = g_strdup_printf(_("%s of RAM"), total);
				text = g_strdup_printf(_(	"%s in use by programs\n"
											"%s in use as cache"),
											user, cache);
				g_free(total);
				g_free(user);
				g_free(cache);
			} else {
				gchar *use = format_percent(xd->user+xd->cache, xd->total, 0);
				text = g_strdup_printf("%s", use);
				g_free(use);
			}
		}	break;

		case GRAPH_NETLOAD: {
			NetData *xd = (NetData*) g->extra_data;
			g_assert_nonnull(xd);

			gchar *tx_in = format_rate_for_display(xd->in_speed);
			gchar *tx_out = format_rate_for_display(xd->out_speed);
			gchar *tx_local = format_rate_for_display(xd->local_speed);

			if (g->config->tooltip_style == TOOLTIP_STYLE_DETAILS) {
				text = g_strdup_printf(_(	"Monitored interfaces: %s\n"
											"\n"
											"Receiving: %s\n"
											"Sending: %s\n"
											"Local: %s"),
											xd->ifaces, tx_in, tx_out, tx_local);
			} else {
				text = g_strdup_printf("\xe2\xac\x87%s \xe2\xac\x86%s", tx_in, tx_out);
			}

			g_free(tx_in);
			g_free(tx_out);
			g_free(tx_local);
		}	break;

		case GRAPH_SWAPLOAD: {
			SwapData *xd = (SwapData*) g->extra_data;
			g_assert_nonnull(xd);

			if (xd->total == 0) {
				text = g_strdup_printf(_("No swap"));
			} else {
				gchar *used = format_percent(xd->used, xd->total, 0);
				gchar *total = g_format_size_full(xd->total, G_FORMAT_SIZE_IEC_UNITS);

				if (g->config->tooltip_style == TOOLTIP_STYLE_DETAILS) {
					title = g_strdup_printf(_("%s of swap"), total);
					text = g_strdup_printf(_("%s used"), used);
				} else {
					text = g_strdup_printf("%s", used);
				}

				g_free(used);
				g_free(total);
			}
		}	break;

		case GRAPH_LOADAVG: {
			LoadAvgData *xd = (LoadAvgData*) g->extra_data;
			g_assert_nonnull(xd);

			if (g->config->tooltip_style == TOOLTIP_STYLE_DETAILS) {
				text = g_strdup_printf(_(	"Last minute: %0.02f\n"
											"Last 5 minutes: %0.02f\n"
											"Last 15 minutes: %0.02f"),
											xd->loadavg[0], xd->loadavg[1], xd->loadavg[2]);
			} else {
				text = g_strdup_printf("%0.02f", xd->loadavg[0]);
			}
		}	break;

		case GRAPH_DISKLOAD: {
			DiskData *xd = (DiskData*) g->extra_data;
			g_assert_nonnull(xd);

			gchar *disk_read = format_rate_for_display(xd->read_speed);
			gchar *disk_write = format_rate_for_display(xd->write_speed);

			if (g->config->tooltip_style == TOOLTIP_STYLE_DETAILS) {
				text = g_strdup_printf(_(	"Read: %s\n"
											"Write: %s"),
											disk_read, disk_write);
			} else {
				text = g_strdup_printf("\xe2\xac\x86%s \xe2\xac\x87%s", disk_read, disk_write);
			}
			g_free(disk_read);
			g_free(disk_write);
		}	break;

		case GRAPH_TEMPERATURE: {
			TemperatureData *xd = (TemperatureData*) g->extra_data;
			g_assert_nonnull(xd);

			if (g->config->tooltip_style == TOOLTIP_STYLE_DETAILS) {
				text = g_strdup_printf(_(	"Current: %.1f °C\n"
											"Critical: %.1f °C"),
											(xd->value/1000.0), (xd->max/1000.0));
			} else {
				text = g_strdup_printf("%.1f °C", xd->value/1000.0);
			}
		}	break;

#ifdef MULTILOAD_EXPERIMENTAL
		case GRAPH_PARAMETRIC: {
			ParametricData *xd = (ParametricData*) g->extra_data;
			g_assert_nonnull(xd);

			if (g->config->tooltip_style == TOOLTIP_STYLE_DETAILS) {
				if (xd->error)
					text = g_strdup_printf(_(	"Command: %s\n"
												"ERROR: %s"),
												xd->command, xd->message);
				else
					text = g_strdup_printf(_(	"Command: %s\n"
												"Result: %lu\n"
												"Message: %s"),
												xd->command, xd->result, xd->message);
			} else {
				text = g_strdup_printf("%lu", xd->result);
			}
		}	break;
#endif

		default: {
			g_assert_not_reached();
		}	break;
	}

	if (title == NULL)
		title = g_strdup(graph_types[g->id].label);

	if (g->config->tooltip_style == TOOLTIP_STYLE_DETAILS) {
		tooltip_markup = g_strdup_printf("<span weight='bold' size='larger'>%s</span>\n%s", title, text);
	} else {
		tooltip_markup = g_strdup_printf("%s: %s", title, text);
	}

	gtk_widget_set_tooltip_markup(g->disp, tooltip_markup);
	g_free(title);
	g_free(text);
	g_free(tooltip_markup);
}

/* get current orientation */
GtkOrientation
multiload_get_orientation(MultiloadPlugin *ma) {
	if (ma->orientation_policy == MULTILOAD_ORIENTATION_HORIZONTAL)
		return GTK_ORIENTATION_HORIZONTAL;
	else if (ma->orientation_policy == MULTILOAD_ORIENTATION_VERTICAL)
		return GTK_ORIENTATION_VERTICAL;
	else // if (ma->orientation_policy == MULTILOAD_ORIENTATION_AUTO)
		return ma->panel_orientation;
}

/* remove the old graphs and rebuild them */
void
multiload_refresh(MultiloadPlugin *ma)
{
	gint i;
	guint n;

	// stop and free the old graphs
	for (n=0, i=0; i < GRAPH_MAX; i++) {
		if (!ma->graphs[i])
			continue;

		load_graph_stop(ma->graphs[i]);
		gtk_widget_destroy(ma->graphs[i]->main_widget);

		load_graph_unalloc(ma->graphs[i]);
		g_free(ma->graphs[i]);

		n++;
	}


	if (ma->box)
		gtk_widget_destroy(ma->box);

	ma->box = gtk_box_new (multiload_get_orientation(ma), ma->spacing);
	gtk_container_set_border_width(GTK_CONTAINER(ma->box), ma->padding);

	gtk_event_box_set_visible_window(GTK_EVENT_BOX(ma->container), ma->fill_between);

	gtk_widget_show (ma->box);
	gtk_container_add (ma->container, ma->box);

	// Children (graphs) are individually shown/hidden to control visibility
	gtk_widget_set_no_show_all (ma->box, TRUE);

	// Create the GRAPH_MAX graphs, with user properties from ma->graph_config.
	// Only start and display the graphs the user has turned on
	for (n=0, i=0; i < GRAPH_MAX; i++) {
		ma->graphs[i] = load_graph_new (ma, i);

		gtk_box_pack_start(GTK_BOX(ma->box),
						   ma->graphs[i]->main_widget,
						   TRUE, TRUE, 0);

		if (ma->graph_config[i].visible) {
			gtk_widget_show_all (ma->graphs[i]->main_widget);
			load_graph_start(ma->graphs[i]);
			n++;
		}
	}

	g_debug("[multiload] Started %d of %d graphs", n, GRAPH_MAX);

	return;
}

void
multiload_init()
{
	static int initialized = 0;
	if ( initialized )
		return;
	initialized = 1;

	glibtop *glt = glibtop_init();
	g_assert_nonnull(glt);


	MULTILOAD_CONFIG_PATH = g_build_filename(
			g_get_home_dir (),
			".config",
			"multiload-ng",
			NULL);

	if (g_mkdir_with_parents (MULTILOAD_CONFIG_PATH, 0755) != 0)
		g_error("[multiload] Error creating directory '%s'", MULTILOAD_CONFIG_PATH);


	multiload_config_init();

	g_debug("[multiload] Initialization complete");
}

void multiload_defaults(MultiloadPlugin *ma)
{
	guint i;

	/* default settings */
	ma->padding = DEFAULT_PADDING;
	ma->spacing = DEFAULT_SPACING;
	ma->fill_between = DEFAULT_FILL_BETWEEN;
	strncpy(ma->color_scheme, DEFAULT_COLOR_SCHEME, sizeof(ma->color_scheme));
	for ( i = 0; i < GRAPH_MAX; i++ ) {
		ma->graph_config[i].border_width = DEFAULT_BORDER_WIDTH;
		ma->graph_config[i].visible = i == 0 ? TRUE : FALSE;
		ma->graph_config[i].interval = DEFAULT_INTERVAL;
		ma->graph_config[i].size = DEFAULT_SIZE;
		ma->graph_config[i].tooltip_style = DEFAULT_TOOLTIP_STYLE;
		ma->graph_config[i].dblclick_policy = DEFAULT_DBLCLICK_POLICY;
		multiload_colors_default(ma, i);
	}
}

void
multiload_sanitize(MultiloadPlugin *ma)
{
	guint i, visible_count = 0;

	/* Keep values between max and min */
	ma->padding = CLAMP(ma->padding, MIN_PADDING, MAX_PADDING);
	ma->spacing = CLAMP(ma->spacing, MIN_SPACING, MAX_SPACING);
	ma->fill_between = ma->fill_between? TRUE:FALSE;
	ma->orientation_policy = CLAMP(ma->orientation_policy, 0, MULTILOAD_ORIENTATION_N_VALUES);

	for ( i=0; i<GRAPH_MAX; i++ ) {
		ma->graph_config[i].border_width = CLAMP(ma->graph_config[i].border_width, MIN_BORDER_WIDTH, MAX_BORDER_WIDTH);

		ma->graph_config[i].interval = CLAMP(ma->graph_config[i].interval, MIN_INTERVAL, MAX_INTERVAL);
		ma->graph_config[i].size = CLAMP(ma->graph_config[i].size, MIN_SIZE, MAX_SIZE);
		ma->graph_config[i].tooltip_style = CLAMP(ma->graph_config[i].tooltip_style, 0, TOOLTIP_STYLE_N_VALUES);
		ma->graph_config[i].dblclick_policy = CLAMP(ma->graph_config[i].dblclick_policy, 0, DBLCLICK_POLICY_N_VALUES);

		if (ma->graph_config[i].visible) {
			ma->graph_config[i].visible = TRUE;
			visible_count++;
		}
	}

	/* Ensure at lease one graph is visible */
	if (visible_count == 0)
		ma->graph_config[0].visible = TRUE;
}

void
multiload_destroy(MultiloadPlugin *ma)
{
	gint i;

	/* Stop the graphs */
	for (i = 0; i < GRAPH_MAX; i++) {
		load_graph_stop(ma->graphs[i]);
		gtk_widget_destroy(ma->graphs[i]->main_widget);

		load_graph_unalloc(ma->graphs[i]);
		g_free(ma->graphs[i]);
	}

	g_debug("[multiload] Destroyed");
}


int
multiload_find_graph_by_name(char *str, char **suffix)
{
	guint i;
	for ( i = 0; i < GRAPH_MAX; i++ ) {
		int n = strlen(graph_types[i].name);
		if ( strncasecmp(str, graph_types[i].name, n) == 0 ) {
			if ( suffix )
				*suffix = str+n;
			return i;
		}
	}
	return -1;
}

