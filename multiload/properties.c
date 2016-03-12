/* GNOME cpuload/memload panel applet
 * (C) 2002 The Free Software Foundation
 *
 * Authors: 
 *		  Todd Kulesza
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <libintl.h>
#include <gtk/gtk.h>

#include "properties.h"
#include "multiload.h"


#define PREF_CONTENT_PADDING 6

static GtkWidget *checkbuttons[NGRAPHS];

/* Defined in panel-specific code. */
extern MultiloadPlugin *
multiload_configure_get_plugin (GtkWidget *widget);

static void
properties_set_checkboxes_sensitive(MultiloadPlugin *ma, gboolean sensitive)
{
	gint i;
	// Count the number of visible graphs
	gint visible_count = 0;
	gint last_graph = 0;

	if (!sensitive) {
		// Only set unsensitive if one checkbox remains checked
		for (i = 0; i < NGRAPHS; i++) {
			if (ma->graph_config[i].visible) {
				last_graph = i;
				visible_count ++;
			}
		}
	}

	if ( visible_count < 2 ) {
		if (sensitive) {
			// Enable all checkboxes
			for (i = 0; i < NGRAPHS; i++)
				gtk_widget_set_sensitive(checkbuttons[i], TRUE);
		} else {
			// Disable last remaining checkbox
			gtk_widget_set_sensitive(checkbuttons[last_graph], FALSE);
		}
	}

	return;
}

static void
property_toggled_cb(GtkWidget *widget, gpointer id)
{
	MultiloadPlugin *ma = multiload_configure_get_plugin(widget);
	gint prop_type = GPOINTER_TO_INT(id);
	gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

	if (active) {
		properties_set_checkboxes_sensitive(ma, TRUE);
		gtk_widget_show_all (ma->graphs[prop_type]->main_widget);
		ma->graph_config[prop_type].visible = TRUE;
		load_graph_start(ma->graphs[prop_type]);
	} else {
		load_graph_stop(ma->graphs[prop_type]);
		gtk_widget_hide (ma->graphs[prop_type]->main_widget);
		ma->graph_config[prop_type].visible = FALSE;
		properties_set_checkboxes_sensitive(ma, FALSE);
	}

	return;
}

static void
preference_toggled_cb(GtkWidget *widget, gpointer id)
{
	MultiloadPlugin *ma = multiload_configure_get_plugin(widget);
	gint prop_type = GPOINTER_TO_INT(id);
	gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	if (prop_type == PROP_SHOWFRAME) {
		ma->show_frame = active;
		multiload_refresh(ma);
	}
}

static void
combobox_changed_cb(GtkWidget *widget, gpointer id)
{
	MultiloadPlugin *ma = multiload_configure_get_plugin(widget);
	gint prop_type = GPOINTER_TO_INT(id);
	int index = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
	if (prop_type == PROP_ORIENTATION) {
		ma->orientation_policy = index;
		multiload_refresh(ma);
	}
}

static void
button_clicked_cb(GtkWidget *widget, gpointer id)
{
	MultiloadPlugin *ma = multiload_configure_get_plugin(widget);
	GtkWidget *dialog = gtk_widget_get_ancestor(widget, GTK_TYPE_DIALOG);
	gint action = GPOINTER_TO_INT(id);
	guint i;
	if (action == ACTION_DEFAULT_COLORS) {
		for ( i = 0; i < NGRAPHS; i++ )
			multiload_colorconfig_default(ma, i);
		multiload_init_preferences(dialog, ma);
	}
}

static void
spin_button_changed_cb(GtkWidget *widget, gpointer id)
{
	MultiloadPlugin *ma = multiload_configure_get_plugin(widget);
	gint prop_type = GPOINTER_TO_INT(id);
	gint value = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
	gint i;

	switch(prop_type) {
		case PROP_SPEED:
			ma->speed = value;
			for (i = 0; i < NGRAPHS; i++) {
				load_graph_stop(ma->graphs[i]);
				if (ma->graph_config[i].visible)
					load_graph_start(ma->graphs[i]);
			}
			break;

		case PROP_SIZE:
			ma->size = value;
			for (i = 0; i < NGRAPHS; i++)
				load_graph_resize(ma->graphs[i]);
			break;

		case PROP_PADDING:
			ma->padding = value;
			multiload_refresh(ma);
			break;

		case PROP_SPACING:
			ma->spacing = value;
			multiload_refresh(ma);
			break;

		default:
			g_assert_not_reached();
	}

	return;
}

/* create a new page in the notebook widget, add it, and return a pointer to it */
static GtkWidget *
add_page(GtkNotebook *notebook, const gchar *label, const gchar *description)
{
	GtkWidget *page;
	GtkWidget *page_label;

	page = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(page), PREF_CONTENT_PADDING);
	page_label = gtk_label_new_with_mnemonic(label);
	gtk_widget_set_tooltip_text(page_label, description);

	gtk_notebook_append_page(notebook, page, page_label);

	return page;
}

/* apply the selected color to the applet */
static void
color_picker_set_cb(GtkColorButton *color_picker, gpointer data)
{
	/* Parse user data for graph and color slot */
	MultiloadPlugin *ma = multiload_configure_get_plugin(GTK_WIDGET (color_picker));
	guint color_slot = GPOINTER_TO_INT(data);
	guint graph = color_slot >> 16;
	guint index = color_slot & 0xFFFF; 

	g_assert(graph >= 0 && graph < NGRAPHS);
	g_assert(index >= 0 && index < graph_types[graph].num_colors);

	gtk_color_button_get_color(color_picker, &ma->graph_config[graph].colors[index]);
	ma->graph_config[graph].alpha[index] = gtk_color_button_get_alpha(color_picker);

	return;
}

/* create a color selector */
static GtkWidget *
color_selector_new(guint graph, guint index, gboolean use_alpha, MultiloadPlugin *ma)
{
	GtkWidget *box;
	GtkWidget *label;
	GtkWidget *color_picker;
	guint color_slot = ( (graph & 0xFFFF) << 16 ) | (index & 0xFFFF);

	const gchar *color_name = graph_types[graph].colors[index].interactive_label;
	const gchar *dialog_title = g_strdup_printf(_("Select color:  %s -> %s"),
					graph_types[graph].noninteractive_label,
					graph_types[graph].colors[index].noninteractive_label);

	box = gtk_hbox_new (FALSE, 3);
	label = gtk_label_new_with_mnemonic(color_name);
	color_picker = gtk_color_button_new_with_color(
					&ma->graph_config[graph].colors[index]);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), color_picker);

	gtk_color_button_set_title (GTK_COLOR_BUTTON(color_picker),	dialog_title);
	if (use_alpha) {
		gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON(color_picker), TRUE);
		gtk_color_button_set_alpha (GTK_COLOR_BUTTON(color_picker),
					ma->graph_config[graph].alpha[index]);
	}

	gtk_box_pack_start (GTK_BOX(box), color_picker, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX(box), label, FALSE, FALSE, 0);

	g_signal_connect(G_OBJECT(color_picker), "color_set",
				G_CALLBACK(color_picker_set_cb), GINT_TO_POINTER(color_slot));

	return box;
}

// create the properties dialog and initialize it from current configuration
void
multiload_init_preferences(GtkWidget *dialog, MultiloadPlugin *ma)
{
	guint i, j, k;
	static GtkNotebook *tabs = NULL;
	GtkSizeGroup *sizegroup;
	GtkWidget *page;
	GtkWidget *frame;
	GtkWidget *box;
	GtkWidget *box2;
	GtkTable *table;
	GtkWidget *label;
	GtkWidget *t;

	GtkWidget *contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	// Delete old container if present
	if (G_UNLIKELY(GTK_IS_WIDGET(tabs)))
		gtk_container_remove(GTK_CONTAINER(contentArea), GTK_WIDGET(tabs));

	// Create new container
	tabs = GTK_NOTEBOOK(gtk_notebook_new());
	gtk_box_pack_start(GTK_BOX(contentArea), GTK_WIDGET(tabs), TRUE, TRUE, 0);



	// COLORS PAGE
	page = add_page(tabs, _("_Resources"), _("Change colors and visibility of the graphs."));

	box = gtk_hbox_new(FALSE, PREF_CONTENT_PADDING);
	gtk_container_add(GTK_CONTAINER(page), box);

	sizegroup = gtk_size_group_new(GTK_SIZE_GROUP_BOTH);
	for( i = 0; i < NGRAPHS; i++ ) {
		// -- -- checkbox
		t = gtk_check_button_new_with_mnemonic(graph_types[i].interactive_label);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(t),
							ma->graph_config[i].visible);
		g_signal_connect(G_OBJECT(t), "toggled",
							G_CALLBACK(property_toggled_cb), GINT_TO_POINTER(i));
		checkbuttons[i] = t;

		// -- -- frame
		frame = gtk_frame_new(NULL);
		gtk_frame_set_label_widget(GTK_FRAME(frame), t);
		gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(frame), FALSE, FALSE, 0);

		box2 = gtk_vbox_new(FALSE, 0);
		gtk_container_set_border_width(GTK_CONTAINER(box2), PREF_CONTENT_PADDING);
		gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET(box2));

		// -- -- colors
		k = graph_types[i].num_colors;
		for( j = 0; j < k; j++ ) {
			if (j == k-1) { // last color (background)
				t = color_selector_new(i, j, FALSE, ma);
				gtk_box_pack_end(GTK_BOX(box2), t, FALSE, FALSE, 0);

				t = gtk_hseparator_new();
				gtk_size_group_add_widget(sizegroup, t);
				gtk_box_pack_end(GTK_BOX(box2), t, FALSE, FALSE, 0);
			} else {
				t = color_selector_new(i, j, TRUE, ma);
				gtk_box_pack_start(GTK_BOX(box2), t, FALSE, FALSE, 0);
			}
			gtk_size_group_add_widget(sizegroup, t);
		}
	}
	properties_set_checkboxes_sensitive(ma, FALSE);

	// -- bottom buttons
	box = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(page), box, FALSE, FALSE, PREF_CONTENT_PADDING);

	t = gtk_button_new_with_label(_("Default colors"));
	g_signal_connect(G_OBJECT(t), "clicked", G_CALLBACK(button_clicked_cb),
							GINT_TO_POINTER(ACTION_DEFAULT_COLORS));
	gtk_box_pack_start(GTK_BOX(box), t, FALSE, FALSE, PREF_CONTENT_PADDING);



	// OPTIONS PAGE
	page = add_page(tabs, _("_Options"), _("Select settings that fit your needs."));

	// -- table
	table = GTK_TABLE(gtk_table_new(4, 3, FALSE));
	gtk_container_set_border_width(GTK_CONTAINER(table), PREF_CONTENT_PADDING);
	gtk_table_set_col_spacings(table, 4);
	gtk_table_set_row_spacings(table, 4);
	gtk_box_pack_start (GTK_BOX (page), GTK_WIDGET(table), FALSE, FALSE, 0);

	// -- -- row: width/height
	if (multiload_get_orientation(ma) == GTK_ORIENTATION_HORIZONTAL)
		label = gtk_label_new_with_mnemonic(_("Wid_th: "));
	else
		label = gtk_label_new_with_mnemonic(_("Heigh_t: "));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
	gtk_table_attach_defaults(table, GTK_WIDGET(label), 0, 1, 0, 1);

	t = gtk_spin_button_new_with_range(MIN_SIZE, MAX_SIZE, STEP_SIZE);
	gtk_label_set_mnemonic_widget (GTK_LABEL(label), t);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(t), (gdouble)ma->size);
	g_signal_connect(G_OBJECT(t), "value_changed",
			G_CALLBACK(spin_button_changed_cb), GINT_TO_POINTER(PROP_SIZE));
	gtk_table_attach_defaults(table, GTK_WIDGET(t), 1, 2, 0, 1);

	label = gtk_label_new (_("pixels"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0f, 0.5f);
	gtk_table_attach_defaults(table, GTK_WIDGET(label), 2, 3, 0, 1);

	// -- -- row: padding
	label = gtk_label_new_with_mnemonic(_("Pa_dding: "));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0f, 0.5f);
	gtk_table_attach_defaults(table, GTK_WIDGET(label), 0, 1, 1, 2);

	t = gtk_spin_button_new_with_range(MIN_PADDING, MAX_PADDING, STEP_PADDING);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), t);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(t), (gdouble)ma->padding);
	g_signal_connect(G_OBJECT(t), "value_changed",
			G_CALLBACK(spin_button_changed_cb), GINT_TO_POINTER(PROP_PADDING));
	gtk_table_attach_defaults(table, GTK_WIDGET(t), 1, 2, 1, 2);

	label = gtk_label_new(_("pixels"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0f, 0.5f);
	gtk_table_attach_defaults(table, GTK_WIDGET(label), 2, 3, 1, 2);

	// -- -- row: spacing
	label = gtk_label_new_with_mnemonic(_("S_pacing: "));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0f, 0.5f);
	gtk_table_attach_defaults(table, GTK_WIDGET(label), 0, 1, 2, 3);

	t = gtk_spin_button_new_with_range(MIN_SPACING, MAX_SPACING, STEP_SPACING);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), t);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(t), (gdouble)ma->spacing);
	g_signal_connect(G_OBJECT(t), "value_changed",
			G_CALLBACK(spin_button_changed_cb), GINT_TO_POINTER(PROP_SPACING));
	gtk_table_attach_defaults(table, GTK_WIDGET(t), 1, 2, 2, 3);

	label = gtk_label_new(_("pixels"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0f, 0.5f);
	gtk_table_attach_defaults(table, GTK_WIDGET(label), 2, 3, 2, 3);

	// -- -- row: update interval
	label = gtk_label_new_with_mnemonic(_("Upd_ate interval: "));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0f, 0.5f);
	gtk_table_attach_defaults(table, GTK_WIDGET(label), 0, 1, 3, 4);

	t = gtk_spin_button_new_with_range(MIN_SPEED, MAX_SPEED, STEP_SPEED);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), t);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(t), (gdouble)ma->speed);
	g_signal_connect(G_OBJECT(t), "value_changed",
			G_CALLBACK(spin_button_changed_cb), GINT_TO_POINTER(PROP_SPEED));
	gtk_table_attach_defaults(table, GTK_WIDGET(t), 1, 2, 3, 4);

	label = gtk_label_new(_("milliseconds"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0f, 0.5f);
	gtk_table_attach_defaults(table, GTK_WIDGET(label), 2, 3, 3, 4);

	// -- -- row: orientation
	label = gtk_label_new_with_mnemonic(_("_Orientation: "));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0f, 0.5f);
	gtk_table_attach_defaults(table, GTK_WIDGET(label), 0, 1, 4, 5);

	t = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(t), _("Automatic"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(t), _("Horizontal"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(t), _("Vertical"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(t), ma->orientation_policy);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), t);

	g_signal_connect(G_OBJECT(t), "changed",
			G_CALLBACK(combobox_changed_cb), GINT_TO_POINTER(PROP_ORIENTATION));
	gtk_table_attach_defaults(table, GTK_WIDGET(t), 1, 2, 4, 5);

	// -- checkbox: show frame
	t = gtk_check_button_new_with_mnemonic(_("Frames around graphs"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(t), ma->show_frame);
	g_signal_connect(G_OBJECT(t), "toggled", G_CALLBACK(preference_toggled_cb),
			GINT_TO_POINTER(PROP_SHOWFRAME));
	gtk_box_pack_start(GTK_BOX(page), t, FALSE, FALSE, 0);


	gtk_widget_show_all(GTK_WIDGET(contentArea));
}
