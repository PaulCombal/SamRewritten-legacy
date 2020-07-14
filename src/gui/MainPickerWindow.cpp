#include "MainPickerWindow.h"
#include "../common/functions.h"
#include "../common/PerfMon.h"
#include "../globals.h"
#include "../types/Achievement.h"
#include "../controller/MySteam.h"

#include <glibmm/main.h>
#include <iostream>


MainPickerWindow::MainPickerWindow(GtkApplicationWindow* cobject, const Glib::RefPtr<Gtk::Builder>& builder, AppId_t initial_app_id)
: Gtk::ApplicationWindow(cobject),
  m_builder(builder),
  m_initial_app_id(initial_app_id),
  m_async_loader(this)
{
    Gtk::Button* cancel_timed_modifications_button;

    // Load the required widgets through the builder
    m_builder->get_widget("game_list", m_game_list);
    m_builder->get_widget("game_search_bar", m_game_search_bar);
    m_builder->get_widget("achievement_search_bar", m_achievement_search_bar);
    m_builder->get_widget("stat_search_bar", m_stat_search_bar);
    m_builder->get_widget("achievement_list", m_achievement_list);
    m_builder->get_widget("stat_list", m_stat_list);
    m_builder->get_widget("about_dialog", m_about_dialog);
    m_builder->get_widget("main_stack", m_main_stack);
    m_builder->get_widget("game_list_view", m_game_list_view);
    m_builder->get_widget("achievement_and_stat_notebook", m_achievement_and_stat_notebook);
    m_builder->get_widget("back_button", m_back_button);
    m_builder->get_widget("store_button", m_store_button);
    m_builder->get_widget("refresh_games_button", m_refresh_games_button);
    m_builder->get_widget("about_button", m_about_button);
    m_builder->get_widget("refresh_achievements_button", m_refresh_achievements_button);
    m_builder->get_widget("separator_lock", m_separator_lock);
    m_builder->get_widget("unlock_all_achievements_button", m_unlock_all_achievements_button);
    m_builder->get_widget("lock_all_achievements_button", m_lock_all_achievements_button);
    m_builder->get_widget("invert_all_achievements_button", m_invert_all_achievements_button);
    m_builder->get_widget("separator_display", m_separator_display);
    m_builder->get_widget("display_only_locked_button", m_display_only_locked_button);
    m_builder->get_widget("display_only_unlocked_button", m_display_only_unlocked_button);
    m_builder->get_widget("separator_timed", m_separator_timed);
    m_builder->get_widget("start_timed_modifications_button", m_start_timed_modifications_button);
    m_builder->get_widget("fetch_games_placeholder", m_fetch_games_placeholder);
    m_builder->get_widget("no_games_found_placeholder", m_no_games_found_placeholder);
    m_builder->get_widget("fetch_achievements_placeholder", m_fetch_achievements_placeholder);
    m_builder->get_widget("fetch_stats_placeholder", m_fetch_stats_placeholder);
    m_builder->get_widget("no_achievements_found_placeholder", m_no_achievements_found_placeholder);
    m_builder->get_widget("no_stats_found_placeholder", m_no_stats_found_placeholder);

    m_builder->get_widget("timed_modifications_window", m_timed_modifications_window);
    m_builder->get_widget("modifications_time_amount", m_modifications_time_amount);
    m_builder->get_widget("modifications_time_unit", m_modifications_time_unit);
    m_builder->get_widget("even_spacing_button", m_even_spacing_button);
    m_builder->get_widget("random_spacing_button", m_random_spacing_button);
    m_builder->get_widget("order_of_selection_button", m_order_of_selection_button);
    m_builder->get_widget("order_random_button", m_order_random_button);
    m_builder->get_widget("applying_modifications_label", m_applying_modifications_label);
    m_builder->get_widget("cancel_timed_modifications_button", cancel_timed_modifications_button);
    m_builder->get_widget("submit_timed_modifications_button", m_submit_timed_modifications_button);
    m_builder->get_widget("exit_game_after_done_button", m_exit_game_after_done_button);

    // Connect them manually to slots
    signal_delete_event().connect(sigc::mem_fun(this, &MainPickerWindow::on_delete));
    signal_show().connect(sigc::mem_fun(this, &MainPickerWindow::on_refresh_games_button_clicked));
    m_game_list->signal_row_activated().connect(sigc::mem_fun(this, &MainPickerWindow::on_game_row_activated));
    m_game_search_bar->signal_search_changed().connect(sigc::mem_fun(this, &MainPickerWindow::on_game_search_changed));
    m_achievement_search_bar->signal_search_changed().connect(sigc::mem_fun(this, &MainPickerWindow::on_achievement_search_changed));
    m_stat_search_bar->signal_search_changed().connect(sigc::mem_fun(this, &MainPickerWindow::on_stat_search_changed));
    m_achievement_and_stat_notebook->signal_switch_page().connect(sigc::mem_fun(this, &MainPickerWindow::on_page_switched));
    m_refresh_games_button->signal_clicked().connect(sigc::mem_fun(this, &MainPickerWindow::on_refresh_games_button_clicked));
    m_refresh_achievements_button->signal_clicked().connect(sigc::mem_fun(this, &MainPickerWindow::on_refresh_achievements_button_clicked));
    m_back_button->signal_clicked().connect(sigc::mem_fun(this, &MainPickerWindow::on_back_button_clicked));
    m_store_button->signal_clicked().connect(sigc::mem_fun(this, &MainPickerWindow::on_store_button_clicked));
    m_about_button->signal_clicked().connect(sigc::mem_fun(this, &MainPickerWindow::on_about_button_clicked));
    m_unlock_all_achievements_button->signal_clicked().connect(sigc::mem_fun(this, &MainPickerWindow::on_unlock_all_achievements_button_clicked));
    m_lock_all_achievements_button->signal_clicked().connect(sigc::mem_fun(this, &MainPickerWindow::on_lock_all_achievements_button_clicked));
    m_invert_all_achievements_button->signal_clicked().connect(sigc::mem_fun(this, &MainPickerWindow::on_invert_all_achievements_button_clicked));
    m_display_only_locked_button->signal_toggled().connect(sigc::mem_fun(this, &MainPickerWindow::on_achievement_search_changed));
    m_display_only_unlocked_button->signal_toggled().connect(sigc::mem_fun(this, &MainPickerWindow::on_achievement_search_changed));    
    m_start_timed_modifications_button->signal_clicked().connect(sigc::mem_fun(this, &MainPickerWindow::on_start_timed_modifications_button_clicked));
    m_about_dialog->signal_response().connect(sigc::mem_fun(this, &MainPickerWindow::on_close_about_dialog));
    m_timed_modifications_window->signal_delete_event().connect(sigc::mem_fun(this, &MainPickerWindow::on_close_timed_modifications_window));
    cancel_timed_modifications_button->signal_clicked().connect(sigc::mem_fun(this, &MainPickerWindow::on_cancel_timed_modifications_button_clicked));
    m_submit_timed_modifications_button->signal_clicked().connect(sigc::mem_fun(this, &MainPickerWindow::on_submit_timed_modifications_button_clicked));

    // Finishing touches
    m_game_list->insert(m_input_appid_row, -1);
    show_fetch_games_placeholder();
}
// => Constructor

MainPickerWindow::~MainPickerWindow() {}
// => Destructor

/**
 * ======================================================
 *                     S I G N A L S
 * ======================================================
 */

void
MainPickerWindow::on_game_search_changed() {
    const std::string filter_text = m_game_search_bar->get_text();
    m_game_list->show_all();
    m_input_appid_row.hide();

    if ( filter_text.empty() ) {
        return;
    }

    if ( digits_only(filter_text) ) {
        m_input_appid_row.show_all();
        m_input_appid_row.set_appid(filter_text);
    }

    for ( AppBoxRow* it : m_app_list_rows )
    {
        std::string app_name = it->get_app().app_name;

        //Holy shit C++, why can't you even do case insensitive comparisons wtf
        //strstri is just a shitty workaround there's no way to do this properly
        if( !strstri(app_name, filter_text) ) {
            it->hide();
        }
    }
}
// => on_game_search_changed

void
MainPickerWindow::on_achievement_search_changed() {
    const std::string filter_text = m_achievement_search_bar->get_text();
    bool only_locked = m_display_only_locked_button->get_active();
    bool only_unlocked = m_display_only_unlocked_button->get_active();
    m_achievement_list->show_all();

    for ( AchievementBoxRow* row : m_achievement_list_rows ) {    
        if ((only_locked && row->get_achievement().achieved) ||
            (only_unlocked && !row->get_achievement().achieved)) {
            row->hide();
        }
    }

    if ( filter_text.empty() ) {
        return;
    }

    for ( AchievementBoxRow* row : m_achievement_list_rows ) {    
        if ( !strstri(row->get_achievement().name, filter_text)
            // Uncomment this if you want to search by description.
            // Disabld by default because it seems too noisy
            // && !strstri(row->get_achievement().desc, filter_text)
            ) {
            row->hide();
        }
    }
}
// => on_achievement_search_changed

void
MainPickerWindow::on_stat_search_changed() {
    const std::string filter_text = m_stat_search_bar->get_text();
    m_stat_list->show_all();

    if ( filter_text.empty() ) {
        return;
    }

    for ( StatBoxRow* row : m_stat_list_rows ) {    
        // stats don't have names, so use ids
        if ( !strstri(row->get_stat().id, filter_text) ) {
            row->hide();
        }
    }
}
// => on_stat_search_changed

void
MainPickerWindow::on_page_switched(Widget* page, guint page_number) {
    // We only have two pages
    if (page_number == 0) {
        m_achievement_search_bar->set_visible(true);
        m_stat_search_bar->set_visible(false);
    } else {
        m_achievement_search_bar->set_visible(false);
        m_stat_search_bar->set_visible(true);
    }
}

void
MainPickerWindow::on_game_row_activated(Gtk::ListBoxRow* row) {
    AppBoxRow* game_row = static_cast<AppBoxRow*>(row);
    AppId_t appid = game_row->get_app().app_id;

    if ( appid == 0 ) {
        appid = m_input_appid_row.get_appid();
    }
    
    if ( appid == 0 ) {
        std::cerr << "An error occurred figuring out which app to launch.. You can report this to the developer." << std::endl;
        return;
    }

    switch_to_achievement_page();
    g_steam->launch_app(appid);
    m_async_loader.populate_achievements();
}
// => on_game_row_activated

void
MainPickerWindow::on_refresh_achievements_button_clicked() {
    g_steam->clear_changes();
    m_async_loader.populate_achievements();
}
// => on_refresh_achievements_button_clicked

void
MainPickerWindow::on_refresh_games_button_clicked() {
    m_game_search_bar->set_text("");
    m_async_loader.populate_apps();

    if (m_initial_app_id != 0) {
        switch_to_achievement_page();
        g_steam->launch_app(m_initial_app_id);
        m_async_loader.populate_achievements();
        m_initial_app_id = 0;
    }
}
// => on_refresh_games_button_clicked

void
MainPickerWindow::on_store_button_clicked() {
    g_steam->commit_changes();
    m_async_loader.populate_achievements();
    m_display_only_locked_button->set_active(false);
    m_display_only_unlocked_button->set_active(false);
}
// => on_store_button_clicked

void
MainPickerWindow::on_back_button_clicked() {
    g_steam->quit_game();
    switch_to_games_page();
}
// => on_back_button_clicked

void
MainPickerWindow::on_lock_all_achievements_button_clicked() {
    for ( AchievementBoxRow* row : m_achievement_list_rows )
    {
        row->lock_scripted();
    }
}
// => on_lock_all_achievements_button_clicked

void
MainPickerWindow::on_unlock_all_achievements_button_clicked() {
    for ( AchievementBoxRow* row : m_achievement_list_rows )
    {
        row->unlock_scripted();
    }
}
// => on_unlock_all_achievements_button_clicked

void
MainPickerWindow::on_invert_all_achievements_button_clicked() {
    for ( AchievementBoxRow* row : m_achievement_list_rows )
    {
        row->invert_scripted();
    }
}
// => on_invert_all_achievements_button_clicked

void
MainPickerWindow::on_start_timed_modifications_button_clicked() {
    // The main window cannot be interacted with while the
    // timed modification window is open because the timed
    // modification window is a dialog and has the modal
    // attribute set. This combination lets us show/hide it
    // without needing extra code to do set_modal/set_transient_for
    m_timed_modifications_window->show();
}
// => on_start_timed_modifications_button_clicked

void
MainPickerWindow::on_cancel_timed_modifications_button_clicked() {
    // Don't revert values in the timed modifications box
    // here - maybe the user wants to use the same settings
    // for the next timed unlock!

    // If timer was active, reset the list because we don't bother
    // to keep track of what's been reset
    if (m_timed_modifications_timer.connected()) {
        m_timed_modifications_timer.disconnect();
        on_refresh_achievements_button_clicked();
    }

    m_submit_timed_modifications_button->show();
    m_applying_modifications_label->hide();
    m_timed_modifications_window->hide();
}
// => on_close_timed_modifications_window

bool
MainPickerWindow::on_close_timed_modifications_window(GdkEventAny* evt) {
    // Don't revert values in the timed modifications box
    // here - maybe the user wants to use the same settings
    // for the next timed unlock!

    on_cancel_timed_modifications_button_clicked();
    return false;
}
// => on_close_timed_modifications_window

void
MainPickerWindow::on_submit_timed_modifications_button_clicked() {
    m_submit_timed_modifications_button->hide();
    m_applying_modifications_label->show();

    // This can be overflowed, but we don't really care
    uint64_t time = m_modifications_time_amount->get_value();
    MODIFICATION_SPACING spacing = EVEN_SPACING;
    MODIFICATION_ORDER order = SELECTION_ORDER;
    
    std::string active_time_id = m_modifications_time_unit->get_active_id();

    // These are hardcoded in the glade file.
    // Doing this is simpler than figuring out how Gtk::Builder::create_from_file
    // generates its Gtk::TreeModel data structures.
    if (active_time_id == "seconds_id") {
        // Do nothing
    } else if (active_time_id == "minutes_id") {
        time *= 60;
    } else if (active_time_id == "hours_id") {
        time *= 60 * 60;
    } else if (active_time_id == "days_id") {
        time *= 60 * 60 * 24;
    } else {
        std::cerr << "unknown time unit!" << std::endl;
    }

    if (m_even_spacing_button->get_active()) {
        spacing = EVEN_SPACING;
    } else if (m_random_spacing_button->get_active()) {
        spacing = RANDOM_SPACING;
    } else {
        std::cerr << "unknown spacing!" << std::endl;
    }

    if (m_order_of_selection_button->get_active()) {
        order = SELECTION_ORDER;
    } else if (m_order_random_button->get_active()) {
        order = RANDOM_ORDER;
    } else {
        std::cerr << "unknown order!" << std::endl;
    }

    m_timed_modification_times = g_steam->setup_timed_modifications(time, spacing, order);

    schedule_timer();
}
// => on_submit_timed_modifications_button_clicked

bool
MainPickerWindow::on_timer_expire() {
    m_timed_modification_times.erase(m_timed_modification_times.begin());
    g_steam->commit_next_timed_modification();
    schedule_timer();
    return G_SOURCE_REMOVE;
}
// => on_timer_expire

void
MainPickerWindow::schedule_timer() {
    if (!m_timed_modification_times.empty()) {
        std::cout << "Modifying next achievement in " << m_timed_modification_times[0] << " seconds"
                  << " (or " << (((double)m_timed_modification_times[0]) / 60) << " minutes or "
                  << ((((double)m_timed_modification_times[0]) / 60) / 60) << " hours)" << std::endl;

        m_timed_modifications_timer = Glib::signal_timeout().connect_seconds(
                                            sigc::mem_fun(this, &MainPickerWindow::on_timer_expire),
                                            m_timed_modification_times[0]);
    } else {
        // Disconnect ourself before calling next function,
        // which checks for connection
        m_timed_modifications_timer.disconnect();

        // Reset the timed modifications window
        on_cancel_timed_modifications_button_clicked();

        // Allows users to modify this while the modifications are taking place
        // I guess that's a feature?
        if (m_exit_game_after_done_button->get_active()) {
            on_back_button_clicked();
        } else {
            m_async_loader.populate_achievements();
        }
    }
}
// => schedule_timer

bool
MainPickerWindow::on_delete(GdkEventAny* evt) {
    hide();
    g_steam->quit_game();
    return false;
}
// => on_delete

void
MainPickerWindow::on_about_button_clicked() {
    m_about_dialog->show();
}
// => on_about_button_clicked

void
MainPickerWindow::on_close_about_dialog(int response_id) {
    // Not sure what to do with the response
    // https://developer.gnome.org/gtk3/stable/GtkDialog.html#GtkResponseType
    m_about_dialog->hide();
}
// => on_close_about_dialog

/**
 * ======================================================
 *               C O N T R O L L E R S
 * ======================================================
 */

void 
MainPickerWindow::reset_game_list() {
    for ( AppBoxRow* row : m_app_list_rows )
    {
        delete row;
        row = nullptr;
    }

    m_app_list_rows.clear();
}
// => reset_game_list

void
MainPickerWindow::reset_achievement_list() {
    for ( AchievementBoxRow* row : m_achievement_list_rows) {    
        delete row;
        row = nullptr;
    }

    m_achievement_list_rows.clear();
}
// => reset_achievement_list

void
MainPickerWindow::reset_stat_list() {
    for ( StatBoxRow* row : m_stat_list_rows) {    
        delete row;
        row = nullptr;
    }

    m_stat_list_rows.clear();
}
// => reset_stat_list

/**
 * Add a game to the list.
 * Some logic remains from the C implementation, but we keep a copy of the 
 * row pointers to access or filter them easily by appid or name
 * The new entry is not shown yet, call confirm_game_list for that.
 */
void 
MainPickerWindow::add_to_game_list(const Game_t& app) {
    AppBoxRow* row = new AppBoxRow(app);
    m_game_list->insert(*row, -1);
    m_app_list_rows.push_back(row);
}
// => add_to_game_list

void
MainPickerWindow::add_to_achievement_list(const Achievement_t& achievement) {
    AchievementBoxRow* row = new AchievementBoxRow(achievement);
    m_achievement_list->insert(*row, -1);
    m_achievement_list_rows.push_back(row);
}
// => add_to_achievement_list

void
MainPickerWindow::add_to_stat_list(const StatValue_t& stat) {
    StatBoxRow* row = new StatBoxRow(stat);
    m_stat_list->insert(*row, -1);
    m_stat_list_rows.push_back(row);
}
// => add_to_stat_list

/**
 * Draws all the games that have not been shown yet
 */
void 
MainPickerWindow::confirm_game_list() {
    m_game_list->show_all();
    m_input_appid_row.hide();
}
// => confirm_game_list

/**
 * Draws all the achievements that have not been shown yet
 */
void
MainPickerWindow::confirm_achievement_list() {
    m_achievement_list->show_all();
}
// => confirm_achievement_list

/**
 * Draws all the stats that have not been shown yet
 */
void
MainPickerWindow::confirm_stat_list() {
    m_stat_list->show_all();
}
// => confirm_stat_list

/**
 * Refreshes the icon for the specified app ID
 * As per GTK, this must only ever be called from the main thread.
 */
void
MainPickerWindow::refresh_app_icon(AppId_t app_id) {
    const std::string path = get_app_icon_path(g_steam->get_cache_path(), app_id);

    for ( AppBoxRow* i : m_app_list_rows )
    {
        if ( i->get_app().app_id == app_id )
        {
            i->set_icon(path, 146, 68);
            return;
        }
    }
}
// => refresh_app_icon

/**
 * Refreshes the icon for the specified achievement icon
 * Requires app_id information to know where to find the icon
 */
void 
MainPickerWindow::refresh_achievement_icon(AppId_t app_id, std::string id) {
    const std::string path = get_achievement_icon_path(g_steam->get_cache_path(), app_id, id);

    for ( AchievementBoxRow* i : m_achievement_list_rows )
    {
        if ( i->get_achievement().id == id ) {
            i->set_icon(path, 64, 64);
        }
    }
}
// => refresh_achievement_icon

void
MainPickerWindow::show_fetch_games_placeholder() {
    m_game_list->set_placeholder(*m_fetch_games_placeholder);
    m_fetch_games_placeholder->show();
}
// => show_fetch_games_placeholder

void
MainPickerWindow::show_no_games_found_placeholder() {
    m_game_list->set_placeholder(*m_no_games_found_placeholder);
    m_no_games_found_placeholder->show();
}
// => show_no_games_found_placeholder

void
MainPickerWindow::show_fetch_achievements_placeholder() {
    m_achievement_list->set_placeholder(*m_fetch_achievements_placeholder);
    m_fetch_achievements_placeholder->show();
}
// => show_fetch_achievements_placeholder

void
MainPickerWindow::show_fetch_stats_placeholder() {
    m_stat_list->set_placeholder(*m_fetch_stats_placeholder);
    m_fetch_stats_placeholder->show();
}
// => show_fetch_stats_placeholder

void
MainPickerWindow::show_no_achievements_found_placeholder() {
    m_achievement_list->set_placeholder(*m_no_achievements_found_placeholder);
    m_no_achievements_found_placeholder->show();
}
// => show_no_achievements_found_placeholder

void
MainPickerWindow::show_no_stats_found_placeholder() {
    m_stat_list->set_placeholder(*m_no_stats_found_placeholder);
    m_no_stats_found_placeholder->show();
}
// => show_no_stats_found_placeholder

void
MainPickerWindow::switch_to_achievement_page() {
    m_back_button->set_visible(true);
    m_achievement_search_bar->set_visible(true);
    m_stat_search_bar->set_visible(false);
    m_store_button->set_visible(true);
    m_refresh_achievements_button->set_visible(true);
    m_separator_lock->set_visible(true);
    m_unlock_all_achievements_button->set_visible(true);
    m_lock_all_achievements_button->set_visible(true);
    m_invert_all_achievements_button->set_visible(true);
    m_separator_display->set_visible(true);
    m_display_only_locked_button->set_visible(true);
    m_display_only_unlocked_button->set_visible(true);
    m_separator_timed->set_visible(true);
    m_start_timed_modifications_button->set_visible(true);

    m_refresh_games_button->set_visible(false);
    m_game_search_bar->set_visible(false);

    m_main_stack->set_visible_child(*m_achievement_and_stat_notebook);
    // Always switch to achievements page because a previously
    // launched app may have left it on the stat tab
    m_achievement_and_stat_notebook->set_current_page(0);
}
// => switch_to_achievement_page

void
MainPickerWindow::switch_to_games_page() {
    m_back_button->set_visible(false);
    m_achievement_search_bar->set_visible(false);
    m_stat_search_bar->set_visible(false);
    m_store_button->set_visible(false);
    m_refresh_achievements_button->set_visible(false);
    m_separator_lock->set_visible(false);
    m_unlock_all_achievements_button->set_visible(false);
    m_lock_all_achievements_button->set_visible(false);
    m_invert_all_achievements_button->set_visible(false);
    m_separator_display->set_visible(false);
    m_display_only_locked_button->set_visible(false);
    m_display_only_unlocked_button->set_visible(false);
    m_display_only_locked_button->set_active(false);
    m_display_only_unlocked_button->set_active(false);
    m_separator_timed->set_visible(false);
    m_start_timed_modifications_button->set_visible(false);

    m_refresh_games_button->set_visible(true);
    m_game_search_bar->set_visible(true);

    m_main_stack->set_visible_child(*m_game_list_view);
    m_achievement_search_bar->set_text("");
    m_stat_search_bar->set_text("");
    reset_achievement_list();
    reset_stat_list();
}
// => switch_to_games_page