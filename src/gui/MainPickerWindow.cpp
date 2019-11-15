#include "MainPickerWindow.h"
#include <iostream>
#include "../common/functions.h"
#include "../common/PerfMon.h"
#include "../globals.h"
#include "../types/Achievement.h"
#include "gtk_callbacks.h"
#include "gtk_input_appid_game_row.h"


MainPickerWindow::MainPickerWindow()
{
    const char ui_file[] = "glade/main_window.glade";
    // This function does all error handling and aborts if there is an error.
    m_builder = gtk_builder_new_from_file(ui_file);

    // Load the required widgets through the builder
    m_game_list = GTK_LIST_BOX(gtk_builder_get_object(m_builder, "game_list"));
    m_game_search_bar = GTK_SEARCH_ENTRY(gtk_builder_get_object(m_builder, "game_search_bar"));
    m_achievement_search_bar = GTK_SEARCH_ENTRY(gtk_builder_get_object(m_builder, "achievement_search_bar"));
    m_achievement_list = GTK_LIST_BOX(gtk_builder_get_object(m_builder, "achievement_list"));
    m_main_window = GTK_WIDGET(gtk_builder_get_object(m_builder, "main_window"));
    m_about_dialog = GTK_WIDGET(gtk_builder_get_object(m_builder, "about_dialog"));
    m_main_stack = GTK_STACK(gtk_builder_get_object(m_builder, "main_stack"));
    m_game_list_view = GTK_SCROLLED_WINDOW(gtk_builder_get_object(m_builder, "game_list_view"));
    m_achievement_list_view = GTK_SCROLLED_WINDOW(gtk_builder_get_object(m_builder, "achievement_list_view"));
    m_back_button = GTK_BUTTON(gtk_builder_get_object(m_builder, "back_button"));
    m_store_button = GTK_BUTTON(gtk_builder_get_object(m_builder, "store_button"));
    m_refresh_games_button = GTK_BUTTON(gtk_builder_get_object(m_builder, "refresh_games_button"));
    m_refresh_achievements_button = GTK_BUTTON(gtk_builder_get_object(m_builder, "refresh_achievements_button"));
    m_unlock_all_achievements_button = GTK_BUTTON(gtk_builder_get_object(m_builder, "unlock_all_achievements_button"));
    m_lock_all_achievements_button = GTK_BUTTON(gtk_builder_get_object(m_builder, "lock_all_achievements_button"));
    m_invert_all_achievements_button = GTK_BUTTON(gtk_builder_get_object(m_builder, "invert_all_achievements_button"));
    m_fetch_games_placeholder = GTK_WIDGET(gtk_builder_get_object(m_builder, "fetch_games_placeholder"));
    m_no_games_found_placeholder = GTK_WIDGET(gtk_builder_get_object(m_builder, "no_games_found_placeholder"));
    m_fetch_achievements_placeholder = GTK_WIDGET(gtk_builder_get_object(m_builder, "fetch_achievements_placeholder"));
    m_no_achievements_found_placeholder = GTK_WIDGET(gtk_builder_get_object(m_builder, "no_achievements_found_placeholder"));

    g_signal_connect(m_game_list, "row-activated", (GCallback)on_game_row_activated, NULL);
    gtk_builder_connect_signals(m_builder, NULL);

    m_input_appid_row = gtk_input_appid_game_row_new();
    gtk_list_box_insert(m_game_list, GTK_WIDGET(m_input_appid_row), -1);
    
    // Show the placeholder widget right away, which is the loading widget
    show_fetch_games_placeholder();
}
// => Constructor

/**
 * See https://stackoverflow.com/questions/9192223/remove-gtk-container-children-repopulate-it-then-refresh
 * used here and other methods, tells you how to iterate through widgets with a relationship.
 * 
 * This method will remove every game entry, only leaving the loading widget.
 */
void 
MainPickerWindow::reset_game_list() {
    for (std::map<AppId_t, GtkWidget*>::iterator it = m_game_list_rows.begin(); it != m_game_list_rows.end(); ++it)
    {
        gtk_widget_destroy( GTK_WIDGET(it->second) );
    }

    m_game_list_rows.clear();
}
// => reset_game_list

void 
MainPickerWindow::reset_achievements_list() {
    for ( auto& [id, row] : m_achievement_list_rows) {
        // gtk_widget_destroy called in destructor of GtkAchievementBoxRow
        delete row;
        row = nullptr;
    }

    m_achievement_list_rows.clear();
}
// => reset_achievements_list

/**
 * Add a game to the list. Ignores warnings for the obsolete GtkArrow.
 * Such a classy widget, I don't get why I should bother creating a shitty 
 * gtkImage instead when it does just what I want out of the box.
 * The entry created is pushed on m_game_list_rows, to be easily acccessed later.
 * The new entry is not shown yet, call confirm_game_list for that.
 */
void 
MainPickerWindow::add_to_game_list(const Game_t& app) {
    // Because you can't clone widgets with GTK, I'm going to recreate 
    // GTK_LIST_BOX_ROW(gtk_builder_get_object(m_builder, "game_entry"));
    // By hand. Which is dead stupid.

    //Also, fuck the police I still use GtkArrow what you gonna do

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"

    GtkWidget *wrapper = gtk_list_box_row_new();
    GtkWidget *layout = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *label = gtk_label_new(app.app_name.c_str());
    GtkWidget *game_logo = gtk_image_new_from_icon_name ("gtk-missing-image", GTK_ICON_SIZE_DIALOG);
    GtkWidget *nice_arrow = gtk_arrow_new(GTK_ARROW_RIGHT, GTK_SHADOW_OUT);

    #pragma GCC diagnostic pop

    gtk_widget_set_size_request(wrapper, -1, 80);

    gtk_box_pack_start(GTK_BOX(layout), GTK_WIDGET(game_logo), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(layout), GTK_WIDGET(label), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(layout), GTK_WIDGET(nice_arrow), FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(wrapper), GTK_WIDGET(layout));

    gtk_list_box_insert(m_game_list, GTK_WIDGET(wrapper), -1);
    
    //Save the created row somewhere for EZ access
    m_game_list_rows.insert(std::pair<AppId_t, GtkWidget*>(app.app_id, wrapper));
}
// => add_to_game_list

void
MainPickerWindow::add_to_achievement_list(const Achievement_t& achievement) {
    GtkAchievementBoxRow *row = new GtkAchievementBoxRow(achievement);
    m_achievement_list_rows.insert(std::make_pair(achievement.id, row));
    gtk_list_box_insert(m_achievement_list, GTK_WIDGET( row->get_main_widget() ), -1);
}
// => add_to_achievement_list

/**
 * Draws all the achievements that have not been shown yet
 */
void
MainPickerWindow::confirm_achievement_list() {
    gtk_widget_show_all( GTK_WIDGET(m_achievement_list) );
}
// => confirm_achievement_list

/**
 * Draws all the games that have not been shown yet
 */
void 
MainPickerWindow::confirm_game_list() {
    gtk_widget_show_all( GTK_WIDGET(m_game_list) );
    gtk_widget_hide(m_input_appid_row);
}
// => confirm_game_list

/**
 * Helper to replace icon in the same location within a row
 */
void
replace_icon(std::string icon_path, GtkWidget * row, int dest_width, int dest_height) {
    GList *children;
    GtkImage *img;
    GdkPixbuf *pixbuf;
    GError *error = nullptr;

    children = gtk_container_get_children(GTK_CONTAINER(row)); //children = the layout
    children = gtk_container_get_children(GTK_CONTAINER(children->data)); //children = first element of layout    
    //children = g_list_next(children);                                   //children = second element of layout...

    img = GTK_IMAGE(children->data);
    if( !GTK_IS_IMAGE(img) ) {
        std::cerr << "It looks like the GUI has been modified, or something went wrong." << std::endl;
        std::cerr << "Inform the developer or look at MainPickerWindow's replace_icon." << std::endl;
        exit(EXIT_FAILURE);
    }

    g_list_free(children);

    pixbuf = gdk_pixbuf_new_from_file(icon_path.c_str(), &error);
    if (error == NULL) {
        // Is the excess of memory freed though?
        pixbuf = gdk_pixbuf_scale_simple(pixbuf, dest_width, dest_height, GDK_INTERP_NEAREST);
        gtk_image_set_from_pixbuf(img, pixbuf);
    }
    else {
        std::cerr << "Error loading icon: " << error->message << std::endl;        
    }
}
// => replace_icon


/**
 * Refreshes the icon for the specified app ID
 * As per GTK, this must only ever be called from the main thread.
 */
void
MainPickerWindow::refresh_app_icon(AppId_t app_id) {
    //TODO make sure app_id is index of m_game_list_rows
    std::string path = get_app_icon_path(app_id);

    // Scale down the banner a bit
    // Quick and jerky, quality isn't key here
    replace_icon(path, m_game_list_rows[app_id], 146, 68);
}
// => refresh_app_icon

/**
 * Refreshes the icon for the specified achievement icon
 * Requires app_id information to know where to find the icon
 */
void 
MainPickerWindow::refresh_achievement_icon(AppId_t app_id, std::string id) {
    std::string path = get_achievement_icon_path(app_id, id);

    // The achievement icons are 64x64, so no resizing
    // This modifies the GtkAchievementBoxRow directly, but eh
    replace_icon(path, m_achievement_list_rows[id]->get_main_widget(), 64, 64);
}
// => refresh_achievement_icon

void
MainPickerWindow::filter_games(const char* filter_text) {
    const std::string text_filter(filter_text);
    std::string text_label;
    
    gtk_widget_show_all( GTK_WIDGET(m_game_list) );
    gtk_widget_hide( GTK_WIDGET(m_input_appid_row) );
    if(text_filter.empty()) {
        return;
    }

    if (digits_only(text_filter))
    {
        gtk_widget_show_all( GTK_WIDGET(m_input_appid_row) );
        gtk_input_appid_game_row_set_appid(m_input_appid_row, text_filter);
    }

    for (std::map<AppId_t, GtkWidget*>::iterator it = m_game_list_rows.begin(); it != m_game_list_rows.end(); ++it)
    {
        GList *children;
        GtkLabel* label;
        children = gtk_container_get_children(GTK_CONTAINER(it->second)); //children = the layout
        children = gtk_container_get_children(GTK_CONTAINER(children->data)); //children = first element of layout
        children = g_list_next(children); //children = label
        label = GTK_LABEL( children->data );

        if( !GTK_IS_LABEL(label) ) {
            std::cerr << "The layout has been modified, please open an issue on Github if you can read this." << std::endl;
            exit(EXIT_FAILURE);
        }

        text_label = std::string( gtk_label_get_text( GTK_LABEL(label) ) );

        //Holy shit C++, why can't you even do case insensitive comparisons wtf
        //strstri is just a shitty workaround there's no way to do this properly
        if(!strstri(text_label, text_filter)) {
            gtk_widget_hide( GTK_WIDGET(it->second) );
        }
    }
}
// => filter_games

void
MainPickerWindow::filter_achievements(const char* filter_text) {
    const std::string text_filter(filter_text);
    std::string text_label;
        
    gtk_widget_show_all( GTK_WIDGET(m_achievement_list) );
    if(text_filter.empty()) {
        return;
    }

    for ( const auto& [id, row] : m_achievement_list_rows) {
        if (!strstri(row->get_achievement().name, text_filter)) {
            gtk_widget_hide( row->get_main_widget() );
        }
    }
}
// => filter_achievements

AppId_t 
MainPickerWindow::get_corresponding_appid_for_row(GtkListBoxRow *row) {
    for(std::map<AppId_t, GtkWidget*>::iterator it = m_game_list_rows.begin(); it != m_game_list_rows.end(); ++it)
    {
        if( (gpointer)it->second == (gpointer)row ) {
            return it->first;
        }
    }
    return 0;
}
// => get_corresponding_appid_for_row

void
MainPickerWindow::unlock_all_achievements() {
    for ( const auto& [id, row] : m_achievement_list_rows) {
        row->unlock();
    }
}
// => unlock_all_achievements

void
MainPickerWindow::lock_all_achievements() {
    for ( const auto& [id, row] : m_achievement_list_rows) {
        row->lock();
    }
}
// => lock_all_achievements

void
MainPickerWindow::invert_all_achievements() {
    for ( const auto& [id, row] : m_achievement_list_rows) {
        row->invert();
    }
}
// => invert_all_achievements

void
MainPickerWindow::show_fetch_games_placeholder() {
    gtk_list_box_set_placeholder(m_game_list, m_fetch_games_placeholder);
    gtk_widget_show(m_fetch_games_placeholder);
}
// => show_fetch_games_placeholder

void
MainPickerWindow::show_no_games_found_placeholder() {
    gtk_list_box_set_placeholder(m_game_list, m_no_games_found_placeholder);
    gtk_widget_show(m_no_games_found_placeholder);
}
// => show_no_games_found_placeholder

void
MainPickerWindow::show_fetch_achievements_placeholder() {
    gtk_list_box_set_placeholder(m_achievement_list, m_fetch_achievements_placeholder);
    gtk_widget_show(m_fetch_achievements_placeholder);
}
// => show_fetch_achievements_placeholder

void
MainPickerWindow::show_no_achievements_found_placeholder() {
    gtk_list_box_set_placeholder(m_achievement_list, m_no_achievements_found_placeholder);
    gtk_widget_show(m_no_achievements_found_placeholder);
}
// => show_no_achievements_found_placeholder

void
MainPickerWindow::switch_to_achievement_page() {
    gtk_widget_set_visible(GTK_WIDGET(m_back_button), TRUE);
    gtk_widget_set_visible(GTK_WIDGET(m_game_search_bar), FALSE);
    gtk_widget_set_visible(GTK_WIDGET(m_achievement_search_bar), TRUE);
    gtk_widget_set_visible(GTK_WIDGET(m_store_button), TRUE);
    gtk_widget_set_visible(GTK_WIDGET(m_refresh_games_button), FALSE);
    gtk_widget_set_visible(GTK_WIDGET(m_refresh_achievements_button), TRUE);
    gtk_widget_set_visible(GTK_WIDGET(m_unlock_all_achievements_button), TRUE);
    gtk_widget_set_visible(GTK_WIDGET(m_lock_all_achievements_button), TRUE);
    gtk_widget_set_visible(GTK_WIDGET(m_invert_all_achievements_button), TRUE);

    gtk_stack_set_visible_child(GTK_STACK(m_main_stack), GTK_WIDGET(m_achievement_list_view));
}
// => switch_to_achievement_page


void
MainPickerWindow::switch_to_games_page() {
    gtk_widget_set_visible(GTK_WIDGET(m_back_button), FALSE);
    gtk_widget_set_visible(GTK_WIDGET(m_game_search_bar), TRUE);
    gtk_widget_set_visible(GTK_WIDGET(m_achievement_search_bar), FALSE);
    gtk_widget_set_visible(GTK_WIDGET(m_store_button), FALSE);
    gtk_widget_set_visible(GTK_WIDGET(m_refresh_games_button), TRUE);
    gtk_widget_set_visible(GTK_WIDGET(m_refresh_achievements_button), FALSE);
    gtk_widget_set_visible(GTK_WIDGET(m_unlock_all_achievements_button), FALSE);
    gtk_widget_set_visible(GTK_WIDGET(m_lock_all_achievements_button), FALSE);
    gtk_widget_set_visible(GTK_WIDGET(m_invert_all_achievements_button), FALSE);

    gtk_stack_set_visible_child(GTK_STACK(m_main_stack), GTK_WIDGET(m_game_list_view));

    gtk_entry_set_text(GTK_ENTRY(m_achievement_search_bar), "");

    reset_achievements_list();
}
// => switch_to_games_page

void
MainPickerWindow::show_about_dialog() {
    gtk_widget_show(GTK_WIDGET(m_about_dialog));
}

void
MainPickerWindow::hide_about_dialog() {
    gtk_widget_hide_on_delete(GTK_WIDGET(m_about_dialog));
}

void
MainPickerWindow::show() {
    gtk_widget_show( get_main_window() );
    gtk_main();
}
// => show

void
MainPickerWindow::stop() {
    gtk_main_quit();
    gtk_widget_destroy( get_main_window() );
}
// => stop