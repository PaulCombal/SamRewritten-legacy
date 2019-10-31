#pragma once
#include "../types/Achievement.h"
#include "../types/Game.h"
#include "GtkAchievementBoxRow.h"
#include <vector>
#include <map>
#include <gtk/gtk.h>
#include <mutex>
#include <future>

#define MAX_OUTSTANDING_ICON_DOWNLOADS 10

/**
 * The main GUI class to display both the games ans the achievements to the user
 * TODO: make a GtkGameBoxRow class?
 */
class MainPickerWindow {
public:
    //Todo: destructor

    /**
     * The constructor loads the gtk builder, the main elements,
     * and shows the main window.
     */
    MainPickerWindow();

    /**
     * Starts a gtk_main
     */
    void show();

    /**
     * Quit all
     */
    void stop();

    /**
     * Empty the game list, leaving only the placeholder widget,
     * which means the loading widget.
     */
    void reset_game_list();

    /**
     * Empty the achievements list, leaving only the placeholder widget,
     * which means the loading widget.
     */
    void reset_achievements_list();

    /**
     * Adds a game to the game list. The new item will be added and saved,
     * but not drawn.
     */
    void add_to_game_list(const Game_t& app);

    /**
     * Adds an achievement to the achievement list. The new item will be added 
     * and saved, but not drawn.
     */
    void add_to_achievement_list(const Achievement_t& achievement);

    /**
     * Shows all widget that has been added to the list, removes all
     * the deleted entries from the GUI list.
     */
    void confirm_game_list();

    /**
     * Shows all widget that has been added to the list, removes all
     * the deleted entries from the GUI list.
     */
    void confirm_stats_list();

    /**
     * When a game is added to the list, the "missing icon" is used by default
     * Once the correct icon has been retrieved, this method will be called
     * to set the right image retrieved from Steam
     */
    void refresh_app_icon(AppId_t app_id);

    /**
     * Filters the game list. For a title to stay displayed,
     * filter_text must be included in it
     */
    void filter_games(const char* filter_text);

    /**
     * Give it a pointer to a row from the main game list, returns the associated
     * appid. Returns 0 on error;
     */
    unsigned long get_corresponding_appid_for_row(GtkListBoxRow *row);

    /**
     * Shows the achievements list instead of the game list
     */
    void switch_to_stats_page();

    /**
     * Shows the games list instead of the stats and achievements list
     */
    void switch_to_games_page();

    /**
     * Opens the About Dialog on top of the current page
     */
    void show_about_dialog();

    /**
     * Closes the About Dialog without destroying it for future calling
     */
    void hide_about_dialog();

    /**
     * Getter for the main window
     */
    GtkWidget* get_main_window() { return m_main_window; };

    /**
     * TODO:
     * All these public variables may better belong in
     * gtk_callbacks.h if that becomes a class.
     * Make them public for now so we can get to them from
     * C functions in gtk_callbacks.cpp without yet having to
     * deal with C++/C linkage differences
     * 
     * Putting std::futures in a C struct is a bad idea because C
     * runtime libraries become confused and crash the program,
     * so keep them in this C++ class for now.
     */

    /**
     * Mutex to prevent on_ask_game_refresh from being reentrant
     * and allowing multiple idle threads to corrupt the main window.
     */
    std::mutex m_game_refresh_lock;

    int outstanding_icon_downloads;
    std::future<void> owned_apps_future;
    std::map< AppId_t, std::future<void>> icon_download_futures;

private:
    GtkWidget *m_main_window;
    GtkWidget *m_about_dialog;
    GtkButton *m_back_button;
    GtkButton *m_store_button;
    GtkListBox *m_game_list;
    GtkListBox *m_stats_list;
    GtkBuilder *m_builder;
    GtkStack *m_main_stack;
    GtkScrolledWindow *m_game_list_view;
    GtkScrolledWindow *m_stats_list_view;

    std::map<unsigned long, GtkWidget*> m_game_list_rows;
    std::vector<GtkAchievementBoxRow*> m_achievement_list_rows;
};