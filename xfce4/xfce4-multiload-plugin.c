/*  $Id$
 *
 *  Copyright (C) 2012 nandhp <nandhp@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "multiload.h"
#include "xfce4-multiload-plugin.h"
#include "xfce4-multiload-dialogs.h"
#include "xfce4-multiload-settings.h"

/* prototypes */
static void
multiload_construct (XfcePanelPlugin *plugin);

/* register the plugin */
XFCE_PANEL_PLUGIN_REGISTER (multiload_construct);

static MultiloadXfcePlugin *
multiload_new (XfcePanelPlugin *plugin)
{
  MultiloadXfcePlugin *multiload;
  GtkOrientation orientation;

  /* allocate memory for the plugin structure */
  multiload = panel_slice_new0 (MultiloadXfcePlugin);

  /* pointer to plugin */
  multiload->plugin = plugin;

  /* read the user settings */
  multiload_read (plugin, &multiload->ma);

  /* create a container widget */
  multiload->ebox = gtk_event_box_new ();
  gtk_widget_show (multiload->ebox);

  /* get the current orientation */
  orientation = xfce_panel_plugin_get_orientation (plugin);

  /* Initialize the applet */
  multiload->ma.container = GTK_CONTAINER(multiload->ebox);
  multiload_refresh(&(multiload->ma), orientation);

  return multiload;
}

static void
multiload_free (XfcePanelPlugin *plugin,
                MultiloadXfcePlugin *multiload)
{
  GtkWidget *dialog;

  /* check if the dialog is still open. if so, destroy it */
  dialog = g_object_get_data (G_OBJECT (plugin), "dialog");
  if (G_UNLIKELY (dialog != NULL))
    gtk_widget_destroy (dialog);

  multiload_destroy (&multiload->ma);

  /* destroy the panel widgets */
  gtk_widget_destroy (multiload->ebox);

  /* cleanup the settings */
  //if (G_LIKELY (multiload->setting1 != NULL))
  //  g_free (multiload->setting1);
  // FIXME

  /* free the plugin structure */
  panel_slice_free (MultiloadXfcePlugin, multiload);
}

static void
multiload_orientation_changed (XfcePanelPlugin *plugin,
                               GtkOrientation   orientation,
                               MultiloadPlugin *ma)
{
  /* Get the plugin's current size request */
  gint size = -1, size_alt = -1;
  gtk_widget_get_size_request (GTK_WIDGET (plugin), &size, &size_alt);
  if ( size < 0 )
    size = size_alt;

  /* Rotate the plugin size to the new orientation */
  if ( orientation == GTK_ORIENTATION_HORIZONTAL)
    gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, size);
  else
    gtk_widget_set_size_request (GTK_WIDGET (plugin), size, -1);

  multiload_refresh(ma, orientation);
}

static gboolean
multiload_size_changed (XfcePanelPlugin *plugin,
                        gint             size,
                        MultiloadPlugin *ma)
{

  /* get the orientation of the plugin */
  GtkOrientation orientation = xfce_panel_plugin_get_orientation (plugin);

  /* set the widget size */
  if ( orientation == GTK_ORIENTATION_HORIZONTAL)
    gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, size);
  else
    gtk_widget_set_size_request (GTK_WIDGET (plugin), size, -1);

  multiload_refresh(ma, orientation);

  /* we handled the orientation */
  return TRUE;
}

static void
multiload_construct (XfcePanelPlugin *plugin)
{
  MultiloadXfcePlugin *multiload;
  
  /* Initialize libgtop */
  multiload_init ();

  /* setup transation domain */
  xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  /* create the plugin */
  multiload = multiload_new (plugin);

  /* add the ebox to the panel */
  gtk_container_add (GTK_CONTAINER (plugin), multiload->ebox);

  /* show the panel's right-click menu on this ebox */
  xfce_panel_plugin_add_action_widget (plugin, multiload->ebox);

  /* connect plugin signals */
  g_signal_connect (G_OBJECT (plugin), "free-data",
                    G_CALLBACK (multiload_free), multiload);

  g_signal_connect (G_OBJECT (plugin), "save",
                    G_CALLBACK (multiload_save), &multiload->ma);

  g_signal_connect (G_OBJECT (plugin), "size-changed",
                    G_CALLBACK (multiload_size_changed), &multiload->ma);

  g_signal_connect (G_OBJECT (plugin), "orientation-changed",
                    G_CALLBACK (multiload_orientation_changed), &multiload->ma);

  /* show the configure menu item and connect signal */
  xfce_panel_plugin_menu_show_configure (plugin);
  g_signal_connect (G_OBJECT (plugin), "configure-plugin",
                    G_CALLBACK (multiload_configure), multiload);

  /* show the about menu item and connect signal */
  xfce_panel_plugin_menu_show_about (plugin);
  g_signal_connect (G_OBJECT (plugin), "about",
                    G_CALLBACK (multiload_about), NULL);
  /* FIXME: void xfce_panel_plugin_menu_insert_item  (XfcePanelPlugin *plugin,
                                                         GtkMenuItem *item);
  */
}
