#include <config.h>
#include "common/ui.h"

#include "common/about-data.h"
#include "common/multiload.h"
#include "common/multiload-config.h"
#include "common/multiload-colors.h"
#include "common/properties.h"
#include "common/util.h"


// ps = Panel Specific - implement these for every panel
extern gpointer multiload_ps_settings_open_for_read(MultiloadPlugin *ma);
extern gpointer multiload_ps_settings_open_for_save(MultiloadPlugin *ma);
extern void multiload_ps_settings_close(gpointer settings);
extern void multiload_ps_settings_get_int(gpointer settings, const gchar *key, int *destination);
extern void multiload_ps_settings_get_boolean(gpointer settings, const gchar *key, gboolean *destination);
extern void multiload_ps_settings_get_string(gpointer settings, const gchar *key, gchar *destination, size_t maxlen);
extern void multiload_ps_settings_set_int(gpointer settings, const gchar *key, int value);
extern void multiload_ps_settings_set_boolean(gpointer settings, const gchar *key, gboolean value);
extern void multiload_ps_settings_set_string(gpointer settings, const gchar *key, const gchar *value);
extern void multiload_ps_preferences_closed_cb(MultiloadPlugin *ma);



void
multiload_ui_read (MultiloadPlugin *ma)
{
	gpointer *settings;
	gchar *key;
	gchar colors_list[10*MAX_COLORS];
	int i;

	multiload_defaults(ma);

	settings = multiload_ps_settings_open_for_read(ma);
	if (G_LIKELY (settings != NULL)) {
		multiload_ps_settings_get_int		(settings, "interval",			&ma->interval);
		multiload_ps_settings_get_int		(settings, "size",				&ma->size);
		multiload_ps_settings_get_int		(settings, "padding",			&ma->padding);
		multiload_ps_settings_get_int		(settings, "spacing",			&ma->spacing);
		multiload_ps_settings_get_int		(settings, "orientation",		&ma->orientation_policy);
		multiload_ps_settings_get_boolean	(settings, "fill-between",		&ma->fill_between);
		multiload_ps_settings_get_boolean	(settings, "tooltip-details",	&ma->tooltip_details);

		multiload_ps_settings_get_int		(settings, "dblclick-policy",	&ma->dblclick_policy);
		multiload_ps_settings_get_string	(settings, "dblclick-cmdline",	ma->dblclick_cmdline, sizeof(ma->dblclick_cmdline)/sizeof(gchar));

		for ( i = 0; i < GRAPH_MAX; i++ ) {
			/* Visibility */
			key = g_strdup_printf("graph-%s-visible", graph_types[i].name);
			multiload_ps_settings_get_boolean (settings, key, &ma->graph_config[i].visible);
			g_free (key);

			/* Border width */
			key = g_strdup_printf("graph-%s-border-width", graph_types[i].name);
			multiload_ps_settings_get_int (settings, key, &ma->graph_config[i].border_width);
			g_free (key);

			/* Colors */
			key = g_strdup_printf("graph-%s-colors", graph_types[i].name);
			colors_list[0] = 0;
			multiload_ps_settings_get_string (settings, key, colors_list, sizeof(colors_list)/sizeof(gchar));
			multiload_colors_unstringify(ma, i, colors_list);
			g_free (key);
		}
		multiload_ps_settings_close(settings);

		multiload_sanitize(ma);
		return;
	} else {
		g_warning("multiload_ui_read: settings = NULL");
	}
}

void
multiload_ui_save (MultiloadPlugin *ma)
{
	gpointer *settings;
	char *key;
	gchar colors_list[10*MAX_COLORS];
	int i;

	settings = multiload_ps_settings_open_for_save(ma);
	if (G_LIKELY (settings != NULL)) {
		multiload_ps_settings_set_int		(settings, "interval",			ma->interval);
		multiload_ps_settings_set_int		(settings, "size",				ma->size);
		multiload_ps_settings_set_int		(settings, "padding",			ma->padding);
		multiload_ps_settings_set_int		(settings, "spacing",			ma->spacing);
		multiload_ps_settings_set_int		(settings, "orientation",		ma->orientation_policy);
		multiload_ps_settings_set_boolean	(settings, "fill-between",		ma->fill_between);
		multiload_ps_settings_set_boolean	(settings, "tooltip-details",	ma->tooltip_details);

		multiload_ps_settings_set_int		(settings, "dblclick-policy",	ma->dblclick_policy);
		multiload_ps_settings_set_string	(settings, "dblclick-cmdline",	ma->dblclick_cmdline);

		for ( i = 0; i < GRAPH_MAX; i++ ) {
			/* Visibility */
			key = g_strdup_printf("graph-%s-visible", graph_types[i].name);
			multiload_ps_settings_set_boolean (settings, key, ma->graph_config[i].visible);
			g_free (key);

			/* Border width */
			key = g_strdup_printf("graph-%s-border-width", graph_types[i].name);
			multiload_ps_settings_set_int (settings, key, ma->graph_config[i].border_width);
			g_free (key);

			/* Colors */
			key = g_strdup_printf("graph-%s-colors", graph_types[i].name);
			multiload_colors_stringify (ma, i, colors_list);
			multiload_ps_settings_set_string (settings, key, colors_list);
			g_free (key);
		}
		multiload_ps_settings_close(settings);
	} else {
		g_warning("multiload_ui_save: settings = NULL");
	}
}

void
multiload_ui_show_help() {
	gchar *cmdline;
	gboolean result;

	cmdline = g_strdup_printf("xdg-open %s", about_data_website);
	result = g_spawn_command_line_async (cmdline, NULL);
	g_free(cmdline);

	if (G_UNLIKELY (result == FALSE))
		g_warning (_("Unable to open the following url: %s"), about_data_website);
}

void
multiload_ui_show_about (GtkWindow* parent)
{
	gtk_show_about_dialog(parent,
		"logo-icon-name",		"utilities-system-monitor",
		"program-name",			about_data_progname,
		"version",				PACKAGE_VERSION,
		"comments",				about_data_description,
		"website",				about_data_website,
		"copyright",			about_data_copyright,
		"license",				about_data_license,
		"authors",				about_data_authors,
		"translator-credits",	_("translator-credits"),
		NULL);
}

void
multiload_ui_configure_response_cb (GtkWidget *dialog, gint response, MultiloadPlugin *ma)
{
	if (response == GTK_RESPONSE_HELP) {
		multiload_ui_show_help();
	} else {
		ma->pref_dialog = NULL;
		multiload_ui_save (ma);
		multiload_ps_preferences_closed_cb(ma);
		gtk_widget_destroy (dialog);
	}
}

GtkWidget*
multiload_ui_configure_dialog_new(MultiloadPlugin *ma, GtkWindow* parent)
{
	if (G_UNLIKELY(ma->pref_dialog != NULL))
		return ma->pref_dialog;

	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Multiload"),
					parent,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_HELP, GTK_RESPONSE_HELP,
					GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
					NULL);

	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
	gtk_window_set_icon_name (GTK_WINDOW (dialog), "utilities-system-monitor");

	// link back the dialog to the plugin
	g_object_set_data (G_OBJECT (dialog), "MultiloadPlugin", ma);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
	multiload_init_preferences(dialog, ma);

	g_signal_connect (G_OBJECT (dialog), "response",
					G_CALLBACK(multiload_ui_configure_response_cb), ma);

	ma->pref_dialog = dialog;
	return dialog;
}

void
multiload_ui_start_system_monitor(MultiloadPlugin *ma)
{
	gchar *cmdline;
	gboolean result;

	if (ma->dblclick_policy == DBLCLICK_POLICY_CMDLINE)
		cmdline = g_strdup(ma->dblclick_cmdline);
	else
		cmdline = get_system_monitor_executable();

	result = g_spawn_command_line_async (cmdline, NULL);

	if (G_UNLIKELY (result == FALSE))
		g_warning (_("Unable to execute the following command line: '%s'"), cmdline);

	g_free(cmdline);
}
