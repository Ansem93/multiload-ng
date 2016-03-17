/* GNOME multiload panel applet
 * (C) 1997 The Free Software Foundation
 *
 * Authors: Tim P. Gerla
 *          Martin Baulig
 *          Todd Kulesza
 *
 * With code from wmload.c, v0.9.2, apparently by Ryan Land, rland@bc1.com.
 *
 */

#include <config.h>
#include <glibtop.h>
#include "multiload.h"
#include "multiload-config.h"
#include "linux-proc.h"
#include "util.h"

/* update the tooltip to the graph's current "used" percentage */
void
multiload_tooltip_update(LoadGraph *g)
{
	gchar *text;
	gchar *tooltip_markup;
	const gchar *name;

	g_assert_nonnull(g);

	/* label the tooltip intuitively */
	if ( g->id >= 0 && g->id < GRAPH_MAX )
		name = graph_types[g->id].label_noninteractive;
	else
		g_assert_not_reached();

	switch (g->id) {
		case GRAPH_CPULOAD: {
			CpuData *xd = (CpuData*) g->extra_data;
			g_assert_nonnull(xd);

			gchar *uptime = format_time_duration(xd->uptime);

			text = g_strdup_printf(_(	"Processors: %ld\n"
										"%.1f%% in use by programs\n"
										"%.1f%% in wait for I/O\n"
										"%.1f%% total use\n"
										"Uptime: %s"),
										xd->num_cpu, (xd->user*100),
										(xd->iowait*100), (xd->total_use*100),
										uptime);
			g_free(uptime);
		}	break;

		case GRAPH_MEMLOAD: {
			MemoryData *xd = (MemoryData*) g->extra_data;
			g_assert_nonnull(xd);

			gchar *total = g_format_size_full(xd->total, G_FORMAT_SIZE_IEC_UNITS);
			gchar *user = format_percent(xd->user, xd->total, 0);
			gchar *cache = format_percent(xd->cache, xd->total, 0);

			// xgettext: use and cache are > 1 most of the time, assume that they always are.
			text = g_strdup_printf(_(	"Total memory: %s\n"
										"%s in use by programs\n"
										"%s in use as cache"),
										total, user, cache);
			g_free(user);
			g_free(cache);
		}	break;

		case GRAPH_NETLOAD: {
			NetData *xd = (NetData*) g->extra_data;
			g_assert_nonnull(xd);

			gchar *tx_in = netspeed_get(xd->in);
			gchar *tx_out = netspeed_get(xd->out);

			gchar *tx_in2 = format_rate_for_display(xd->in_speed);
			gchar *tx_out2 = format_rate_for_display(xd->out_speed);

			text = g_strdup_printf(_(	"Receiving %s - %s\n"
										"Sending %s - %s"),
										tx_in, tx_in2, tx_out, tx_out2);
			g_free(tx_in);
			g_free(tx_out);
		}	break;

		case GRAPH_SWAPLOAD: {
			SwapData *xd = (SwapData*) g->extra_data;
			g_assert_nonnull(xd);

			if (xd->total == 0) {
				text = g_strdup_printf(_("Swap is not used"));
			} else {
				gchar *used = format_percent(xd->used, xd->total, 0);
				gchar *total = g_format_size_full(xd->total, G_FORMAT_SIZE_IEC_UNITS);

				text = g_strdup_printf(_("%s used of %s"), used, total);

				g_free(used);
				g_free(total);
			}
		}	break;

		case GRAPH_LOADAVG: {
			LoadAvgData *xd = (LoadAvgData*) g->extra_data;
			g_assert_nonnull(xd);

			text = g_strdup_printf(_(	"Last minute: %0.02f\n"
										"Last 5 minutes: %0.02f\n"
										"Last 15 minutes: %0.02f"),
										xd->loadavg[0], xd->loadavg[1], xd->loadavg[2]);
		}	break;

		case GRAPH_DISKLOAD: {
			DiskData *xd = (DiskData*) g->extra_data;
			g_assert_nonnull(xd);

			gchar *disk_read = format_rate_for_display(xd->read_speed);
			gchar *disk_write = format_rate_for_display(xd->write_speed);

			text = g_strdup_printf(_(	"Read %s\n"
										"Write %s"),
										disk_read, disk_write);

			g_free(disk_read);
			g_free(disk_write);
		}	break;

		case GRAPH_TEMPERATURE: {
			TemperatureData *xd = (TemperatureData*) g->extra_data;
			g_assert_nonnull(xd);

			text = g_strdup_printf(_(	"%.1f °C"),
										(xd->temperature/1000.0));
		}	break;

		default: {
			guint i;
			gchar *percent;
			guint total_used = 0;

			for (i = 0; i < multiload_config_get_num_data(g->id); i++)
				total_used += g->data[0][i];

			percent = format_percent(total_used, g->draw_height, 0);
			text = g_strdup_printf(_(	"%s in use"),
										percent);

			g_free(percent);
		}	break;
	}

	tooltip_markup = g_strdup_printf("<span underline='single' weight='bold' size='larger'>%s</span>\n%s", name, text);

	gtk_widget_set_tooltip_markup(g->disp, tooltip_markup);
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

	// stop and free the old graphs
	for (i = 0; i < GRAPH_MAX; i++) {
		if (!ma->graphs[i])
			continue;

		load_graph_stop(ma->graphs[i]);
		gtk_widget_destroy(ma->graphs[i]->main_widget);

		load_graph_unalloc(ma->graphs[i]);
		g_free(ma->graphs[i]);
	}

	if (ma->box)
		gtk_widget_destroy(ma->box);

	ma->box = gtk_vbox_new (FALSE, ma->spacing);
	gtk_container_set_border_width(GTK_CONTAINER(ma->box), ma->padding);

	// Switch between GtkVBox and GtkHBox depending of orientation
	gtk_orientable_set_orientation(GTK_ORIENTABLE(ma->box), multiload_get_orientation(ma));

	gtk_event_box_set_visible_window(GTK_EVENT_BOX(ma->container), ma->fill_between);

	gtk_widget_show (ma->box);
	gtk_container_add (ma->container, ma->box);

	// Children (graphs) are individually shown/hidden to control visibility
	gtk_widget_set_no_show_all (ma->box, TRUE);

	// Create the GRAPH_MAX graphs, with user properties from ma->graph_config.
	// Only start and display the graphs the user has turned on
	for (i = 0; i < GRAPH_MAX; i++) {
		ma->graphs[i] = load_graph_new (ma, i);

		gtk_box_pack_start(GTK_BOX(ma->box),
						   ma->graphs[i]->main_widget,
						   TRUE, TRUE, 0);

		if (ma->graph_config[i].visible) {
			gtk_widget_show_all (ma->graphs[i]->main_widget);
			load_graph_start(ma->graphs[i]);
		}
	}

	return;
}

void
multiload_init()
{
	static int initialized = 0;
	if ( initialized )
		return;

	glibtop *glt = glibtop_init();
	g_assert_nonnull(glt);

	multiload_config_init();
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

	return;
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

