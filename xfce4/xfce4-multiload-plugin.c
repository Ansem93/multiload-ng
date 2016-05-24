/*
 * Copyright (C) 2016 Mario Cianciolo <mr.udda@gmail.com>
 *               2012 nandhp <nandhp@gmail.com>
 *
 * This file is part of multiload-nandhp.
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
#include <string.h>

#ifdef HAVE_XFCE4UI
#include <libxfce4ui/libxfce4ui.h>
#elif HAVE_XFCEGUI4
#include <libxfcegui4/libxfcegui4.h>
#else
#error Must have one of libxfce4ui or xfcegui4
#endif
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/xfce-panel-plugin.h>

#include "common/multiload.h"
#include "common/ui.h"


gpointer
multiload_ps_settings_open_for_read(MultiloadPlugin *ma)
{
	gchar *file;
	XfceRc *rc;
	file = xfce_panel_plugin_lookup_rc_file((XfcePanelPlugin*)(ma->panel_data));
	if (G_UNLIKELY(file == NULL))
		return NULL;

	rc = xfce_rc_simple_open (file, TRUE);
	g_free(file);

	return (gpointer)rc;
}
gpointer
multiload_ps_settings_open_for_save(MultiloadPlugin *ma)
{
	gchar *file;
	XfceRc *rc;

	file = xfce_panel_plugin_save_location((XfcePanelPlugin*)(ma->panel_data), TRUE);
	if (G_UNLIKELY(file == NULL))
		return NULL;

	rc = xfce_rc_simple_open (file, FALSE);
	g_free(file);

	return (gpointer)rc;
}

void
multiload_ps_settings_close(gpointer settings)
{
	xfce_rc_close((XfceRc*)settings);
}

void
multiload_ps_settings_get_int(gpointer settings, const gchar *key, int *destination)
{
	*destination = xfce_rc_read_int_entry((XfceRc*)settings, key, *destination);
}
void
multiload_ps_settings_get_boolean(gpointer settings, const gchar *key, gboolean *destination)
{
	*destination = xfce_rc_read_bool_entry((XfceRc*)settings, key, *destination);
}
void
multiload_ps_settings_get_string(gpointer settings, const gchar *key, gchar *destination, size_t maxlen)
{
	const gchar* temp = xfce_rc_read_entry((XfceRc*)settings, key, NULL);
	if (G_LIKELY(temp != NULL))
		strncpy(destination, temp, maxlen);
}

void
multiload_ps_settings_set_int(gpointer settings, const gchar *key, int value)
{
	xfce_rc_write_int_entry((XfceRc*)settings, key, value);
}
void
multiload_ps_settings_set_boolean(gpointer settings, const gchar *key, gboolean value)
{
	xfce_rc_write_bool_entry((XfceRc*)settings, key, value);
}
void
multiload_ps_settings_set_string(gpointer settings, const gchar *key, const gchar *value)
{
	xfce_rc_write_entry((XfceRc*)settings, key, value);
}

void
multiload_ps_preferences_closed_cb(MultiloadPlugin *ma)
{
	xfce_panel_plugin_unblock_menu ((XfcePanelPlugin*)(ma->panel_data));
}



void
xfce_configure_cb (XfcePanelPlugin *plugin, MultiloadPlugin *ma)
{
	xfce_panel_plugin_block_menu (plugin);
	GtkWidget *dialog = multiload_ui_configure_dialog_new(ma,
		GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))));
	gtk_widget_show (dialog);
}

void
xfce_about_cb (XfcePanelPlugin *plugin)
{
	multiload_ui_show_about(GTK_WINDOW(gtk_widget_get_toplevel (GTK_WIDGET(plugin))));
}

void
xfce_save_cb (XfcePanelPlugin *plugin, MultiloadPlugin *multiload)
{
	multiload_ui_save(multiload);
}

void
xfce_free_cb (XfcePanelPlugin *plugin, MultiloadPlugin *multiload)
{
	if (G_UNLIKELY (multiload->pref_dialog != NULL))
		gtk_widget_destroy (multiload->pref_dialog);

	multiload_destroy (multiload);
	gtk_widget_destroy (GTK_WIDGET(multiload->container));

	g_slice_free (MultiloadPlugin, multiload);
}

static gboolean
xfce_size_changed_cb (XfcePanelPlugin *plugin, gint size, MultiloadPlugin *ma)
{
	/* set the widget size */
	if ( ma->panel_orientation == GTK_ORIENTATION_HORIZONTAL)
		gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, size);
	else
		gtk_widget_set_size_request (GTK_WIDGET (plugin), size, -1);

	multiload_refresh(ma);

	return TRUE;
}

static void
xfce_orientation_changed_cb (XfcePanelPlugin *plugin, GtkOrientation orientation, MultiloadPlugin *ma)
{
	gint size[2];

	ma->panel_orientation = orientation;

	gtk_widget_get_size_request (GTK_WIDGET (plugin), size+0, size+1);
	xfce_size_changed_cb(plugin, MAX(size[0], size[1]), ma);
}


static void
xfce_constructor (XfcePanelPlugin *plugin)
{
	MultiloadPlugin *multiload;

	xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
	multiload_init ();

	multiload = g_slice_new0 (MultiloadPlugin);

	multiload->panel_data = plugin;
	multiload_ui_read (multiload);

	multiload->container = GTK_CONTAINER(gtk_event_box_new());
	gtk_widget_show (GTK_WIDGET(multiload->container));

	multiload->panel_orientation = xfce_panel_plugin_get_orientation (plugin);

	multiload_refresh(multiload);

	gtk_container_add (GTK_CONTAINER (plugin), GTK_WIDGET(multiload->container));

	/* show the panel's right-click menu on this ebox */
	xfce_panel_plugin_add_action_widget (plugin, GTK_WIDGET(multiload->container));

	/* connect plugin signals */
	g_signal_connect (G_OBJECT (plugin), "free-data",
						G_CALLBACK (xfce_free_cb), multiload);

	g_signal_connect (G_OBJECT (plugin), "save",
						G_CALLBACK (xfce_save_cb), multiload);

	g_signal_connect (G_OBJECT (plugin), "size-changed",
						G_CALLBACK (xfce_size_changed_cb), multiload);

	g_signal_connect (G_OBJECT (plugin), "orientation-changed",
						G_CALLBACK (xfce_orientation_changed_cb), multiload);

	/* show the configure menu item and connect signal */
	xfce_panel_plugin_menu_show_configure (plugin);
	g_signal_connect (G_OBJECT (plugin), "configure-plugin",
						G_CALLBACK (xfce_configure_cb), multiload);

	/* show the about menu item and connect signal */
	xfce_panel_plugin_menu_show_about (plugin);
	g_signal_connect (G_OBJECT (plugin), "about",
						G_CALLBACK (xfce_about_cb), NULL);
}

/* register the plugin */
#ifdef XFCE_PANEL_PLUGIN_REGISTER
XFCE_PANEL_PLUGIN_REGISTER (xfce_constructor);           /* Xfce 4.8 */
#else
XFCE_PANEL_PLUGIN_REGISTER_INTERNAL (xfce_constructor);  /* Xfce 4.6 */
#endif
