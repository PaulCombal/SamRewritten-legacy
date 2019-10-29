#include "GtkAchievementBoxRow.h"
#include "../MySteam.h"
#include "../globals.h"
#include <string>
#include <iostream>


extern "C"
{
    void 
    on_achievement_button_toggle(GtkToggleButton* but, gpointer ach_row) {
        // Urgh, in gtk when you call gtk_toggle_button_set_active,
        // you get a toggle/click event and there's no way to disable it.
        // So ignore the one we do get. Trying to use another signal doesn't
        // work out easily either
        GtkAchievementBoxRow * this_row = (GtkAchievementBoxRow *)ach_row;
        if (this_row->m_ignore_toggle) return;

        const Achievement_t* ach = &this_row->m_data;
        const bool active = gtk_toggle_button_get_active(but);
        const bool achieved = ach->achieved;
        const std::string ach_id = ach->id;

        if (active && achieved) {
            gtk_button_set_label(GTK_BUTTON(but), "To relock");
            g_steam->add_modification_ach(ach_id, false);
        } else if (!active && achieved) {
            gtk_button_set_label(GTK_BUTTON(but), "🔓 Unlocked");
            g_steam->remove_modification_ach(ach_id);
        } else if (active && !achieved) {
            gtk_button_set_label(GTK_BUTTON(but), "To unlock");
            g_steam->add_modification_ach(ach_id, true);
        } else if (!active && !achieved) {
            gtk_button_set_label(GTK_BUTTON(but), "🔒 Locked");
            g_steam->remove_modification_ach(ach_id);
        }
    }
}

void
GtkAchievementBoxRow::unlock() {
    m_ignore_toggle = true;
    const bool active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_lock_unlock_button));
    const bool achieved = m_data.achieved;
    const std::string ach_id = m_data.id;
    
    // The button wasn't actually clicked, so the condition is different
    if (!active && !achieved) {
        gtk_button_set_label(GTK_BUTTON(m_lock_unlock_button), "To unlock");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_lock_unlock_button), TRUE);
        g_steam->add_modification_ach(ach_id, true);
    } else if (active && achieved) {
        gtk_button_set_label(GTK_BUTTON(m_lock_unlock_button), "🔓 Unlocked");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_lock_unlock_button), FALSE);
        g_steam->remove_modification_ach(ach_id);
    }
    // Do nothing for all other conditions
    m_ignore_toggle = false;
}
// => unlock

void
GtkAchievementBoxRow::lock() {
    m_ignore_toggle = true;
    const bool active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_lock_unlock_button));
    const bool achieved = m_data.achieved;
    const std::string ach_id = m_data.id;

    // The button wasn't actually clicked, so the condition is different
    if (!active && achieved) {
        gtk_button_set_label(GTK_BUTTON(m_lock_unlock_button), "To relock");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_lock_unlock_button), TRUE);
        g_steam->add_modification_ach(ach_id, false);
    } else if (active && !achieved) {
        gtk_button_set_label(GTK_BUTTON(m_lock_unlock_button), "🔒 Locked");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_lock_unlock_button), FALSE);
        g_steam->remove_modification_ach(ach_id);
    }
    // Do nothing for all other conditions
    m_ignore_toggle = false;

}
// => lock

void
GtkAchievementBoxRow::invert() {
    m_ignore_toggle = true;
    const bool active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_lock_unlock_button));
    const bool achieved = m_data.achieved;
    const std::string ach_id = m_data.id;

    // The button wasn't actually clicked, so the condition is different
    // TODO: is this the expected behavior for invert? Who uses this?
    if (!active && achieved) {
        gtk_button_set_label(GTK_BUTTON(m_lock_unlock_button), "To relock");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_lock_unlock_button), TRUE);
        g_steam->add_modification_ach(ach_id, false);
    } else if (active && achieved) {
        gtk_button_set_label(GTK_BUTTON(m_lock_unlock_button), "🔓 Unlocked");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_lock_unlock_button), FALSE);
        g_steam->remove_modification_ach(ach_id);
    } else if (!active && !achieved) {
        gtk_button_set_label(GTK_BUTTON(m_lock_unlock_button), "To unlock");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_lock_unlock_button), TRUE);
        g_steam->add_modification_ach(ach_id, true);
    } else if (active && !achieved) {
        gtk_button_set_label(GTK_BUTTON(m_lock_unlock_button), "🔒 Locked");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_lock_unlock_button), FALSE);
        g_steam->remove_modification_ach(ach_id);
    }

    m_ignore_toggle = false;
}
// => invert

GtkAchievementBoxRow::GtkAchievementBoxRow(const Achievement_t& data) 
:
m_data(data),
m_ignore_toggle(false)
{
    // TODO achievement icons
    // TODO Rewrite. Ugly AF for unknown reasons
    std::string ach_title_text, ach_player_percent_text, ach_locked_text;
    if ( data.achieved ) {
        ach_locked_text = "🔓 Unlocked";
    } else {
        ach_locked_text = "🔒 Locked";
    }

    ach_title_text = "<b>" + data.name + "</b>";
    ach_player_percent_text = "Achieved by " + std::to_string(data.global_achieved_rate) + " of the players";

    //TODO create and set new level bar only if ach has progress bar
    m_main_box = gtk_list_box_row_new();
    
    GtkWidget *layout = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *ach_pic = gtk_image_new_from_icon_name("gtk-missing-image", GTK_ICON_SIZE_DIALOG);
    GtkWidget *title_desc_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *title_label = gtk_label_new("");
    GtkWidget *desc_label = gtk_label_new(data.desc.c_str());
    GtkWidget *more_info_button = gtk_menu_button_new();
    GtkWidget *more_info_image = gtk_image_new_from_icon_name("gtk-about", GTK_ICON_SIZE_BUTTON);
    m_lock_unlock_button = gtk_toggle_button_new_with_label(ach_locked_text.c_str()); 
    GtkWidget *popover_menu = gtk_popover_new( more_info_button );
    GtkWidget *popover_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
    GtkWidget *more_info_label = gtk_label_new("");
    GtkWidget *percentage_players_label = gtk_label_new(ach_player_percent_text.c_str());
    GtkWidget *sep_one = gtk_separator_menu_item_new();
    GtkWidget *sep_two = gtk_separator_menu_item_new();
    GtkWidget *progression_label = gtk_label_new("Achievement progress");
    GtkWidget *ach_level_bar = gtk_level_bar_new(); //gtk_level_bar_new_for_interval (gdouble min_value, gdouble max_value);
    GtkWidget *ach_progress_label_value = gtk_label_new("TODO / TODO");

    gtk_label_set_markup(GTK_LABEL(title_label), ach_title_text.c_str());
    gtk_label_set_markup(GTK_LABEL(more_info_label), "<b>Additional information</b>");
    gtk_widget_set_size_request(m_main_box, -1, 80);
    gtk_menu_button_set_popover(GTK_MENU_BUTTON(more_info_button), GTK_WIDGET(popover_menu));
    gtk_widget_set_valign(GTK_WIDGET(more_info_button), GTK_ALIGN_CENTER);
    gtk_widget_set_margin_end(GTK_WIDGET(more_info_button), 10);
    gtk_widget_set_size_request(GTK_WIDGET(m_lock_unlock_button), 150, -1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_lock_unlock_button), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(popover_box), 5);
    gtk_style_context_add_class(
        gtk_widget_get_style_context( GTK_WIDGET(more_info_button) ),
        "circular"
    );

    gtk_container_add(GTK_CONTAINER(more_info_button), GTK_WIDGET(more_info_image));
    gtk_box_pack_start(GTK_BOX(title_desc_box), GTK_WIDGET(title_label), FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(title_desc_box), GTK_WIDGET(desc_label), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(layout), GTK_WIDGET(ach_pic), FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(layout), GTK_WIDGET(title_desc_box), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(layout), GTK_WIDGET(more_info_button), FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(layout), GTK_WIDGET(m_lock_unlock_button), FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(popover_box), GTK_WIDGET(more_info_label), FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(popover_box), GTK_WIDGET(sep_one), FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(popover_box), GTK_WIDGET(percentage_players_label), FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(popover_box), GTK_WIDGET(sep_two), FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(popover_box), GTK_WIDGET(progression_label), FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(popover_box), GTK_WIDGET(ach_level_bar), FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(popover_box), GTK_WIDGET(ach_progress_label_value), FALSE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(popover_menu), GTK_WIDGET(popover_box));
    gtk_widget_show_all(popover_box);
    gtk_container_add(GTK_CONTAINER(m_main_box), GTK_WIDGET(layout));

    g_signal_connect(m_lock_unlock_button, "toggled", (GCallback)on_achievement_button_toggle,  (gpointer)this);
}

GtkAchievementBoxRow::~GtkAchievementBoxRow() {
    gtk_widget_destroy( GTK_WIDGET(m_main_box) );
}