#include "gtk_callbacks.h"
#include <iostream>
#include <future>
#include "MainPickerWindow.h"
#include "../common/PerfMon.h"
#include "../MySteam.h"
#include "../globals.h"

// See comments in the header file

extern "C"
{

    void
    populate_achievements() {
        // Get_achievements from game server
        std::vector<Achievement_t> achievements = g_steam->get_achievements();

        g_main_gui->reset_achievements_list();

        //TODO: just pass in the array directly?
        for(Achievement_t achievement : achievements) {
            g_main_gui->add_to_achievement_list(achievement);
        }

        // TODO: these are not stats they are general achievements
        g_main_gui->confirm_stats_list();
    }
    // => populate_achievements

    void 
    on_close_button_clicked() {
        g_main_gui->stop();
        
        delete g_main_gui;
        g_main_gui = nullptr;

        g_steam->quit_game();
    }
    // => on_close_button_clicked

    void
    on_store_button_clicked() {
        std::cerr << "Saving stats and achievements." << std::endl;

        g_steam->commit_changes();

        populate_achievements();
    }
    // => on_store_button_clicked

    /*
     * The actual loading function
     * 
     * We CANNOT update the GUI from any thread but the main thread because
     * it is explicitly deprecated in gtk...
     * (e.g. https://developer.gnome.org/gdk3/stable/gdk3-Threads.html#gdk-threads-init)
     * See the gtk_callbacks.h for the FSM rationale.
     * 
     * For anything that isn't the GUI, we can fire off worker threads, and doing
     * so is simpler than splitting out the worker threads into the main GUI loop.
     * Additionally, the worker threads depend on calling functions which may
     * take a while to return. So, splitting them out into a thread won't expose the
     * main loop to these latencies and potentially make it laggy.
     */
    static gboolean
    load_items_idle (gpointer data_)
    {
        IdleData *data = (IdleData *)data_;

        if (data->state == STATE_STARTED) {
            g_main_gui->reset_game_list();
            g_perfmon->log("Starting library parsing.");
            g_main_gui->owned_apps_future = std::async(std::launch::async, []{g_steam->refresh_owned_apps();});
            data->state = STATE_WAITING_FOR_OWNED_APPS;
            return G_SOURCE_CONTINUE;
        }

        if (data->state == STATE_WAITING_FOR_OWNED_APPS) {
            if (g_main_gui->owned_apps_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                g_perfmon->log("Done retrieving and filtering owned apps");
                data->state = STATE_LOADING_GUI;
            }
            return G_SOURCE_CONTINUE;
        }

        if (data->state == STATE_LOADING_GUI) {
            if (data->current_item == g_steam->get_subscribed_apps().size()) {
                g_perfmon->log("Done adding apps to GUI");
                g_main_gui->confirm_game_list();
                data->state = STATE_DOWNLOADING_ICONS;
                data->current_item = 0;
                return G_SOURCE_CONTINUE;
            }

            Game_t app = g_steam->get_subscribed_apps()[data->current_item];
            g_main_gui->add_to_game_list(app);
            data->current_item++;
            return G_SOURCE_CONTINUE;
        }

        if (data->state == STATE_DOWNLOADING_ICONS) {
            // This must occur after the main gui game_list is
            // complete, otherwise we might have concurrent
            // access and modification of the GUI's game_list

            bool done_starting_downloads = (data->current_item == g_steam->get_subscribed_apps().size());

            // Make sure we're done starting all downloads and finshed with outstanding downloads
            if (done_starting_downloads && (g_main_gui->icon_download_futures.size() == 0)) {
                g_perfmon->log("Done downloading icons");
                data->state = STATE_FINISHED;
                return G_SOURCE_REMOVE;
            }

            // We could have the threads IPC the data back to the main thread
            // but that's a bit heavyweight for what we need here:
            // the main thread just needs to know the app_id that completed, and
            // it can figure out the rest.

            // Implement a poor man's thread pool for now - only fire off
            // a few threads at a time (though threads are not recycled).
            // We would use a semaphore here for outstanding_icon_downloads,
            // but the GTK main loop is forcibly single-threaded
            // (the whole reason we need to do these shenanigans anyway),
            // so only 1 thread will ever be here at a time anyway.
            if ( !done_starting_downloads && (g_main_gui->outstanding_icon_downloads < MAX_OUTSTANDING_ICON_DOWNLOADS))  {
                // Fire off a new download thread
                Game_t app = g_steam->get_subscribed_apps()[data->current_item];
                g_main_gui->icon_download_futures.insert(std::make_pair(app.app_id, std::async(std::launch::async, g_steam->refresh_icon, app.app_id)));
                g_main_gui->outstanding_icon_downloads++;
                data->current_item++;

                // continue on to service a thread if it's finished
            }

            // Try to find a thread that is finished. Only process at most 1 per GTK main loop.
            // The max time this takes to traverse is controlled by the size of the
            // icon_download_futures size, which is controlled by MAX_ICON_DOWNLOADS.
            // Increasing this could lead to GUI stutter if it needs to traverse a large map,
            // although the map has logarithmic traversal and update complexity.
            for (auto const& [app_id, this_future] : g_main_gui->icon_download_futures) {
                if (this_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                    // TODO: remove the app if it has a bad icon? (because then it's mostly likely not a game)
                    g_main_gui->refresh_app_icon(app_id);
                    g_main_gui->icon_download_futures.erase(app_id);
                    g_main_gui->outstanding_icon_downloads--;
                    // let's only process one at a time
                    return G_SOURCE_CONTINUE;
                }
            }

            return G_SOURCE_CONTINUE;
        }

        // Should never reach here
        return G_SOURCE_CONTINUE;
    }
    // => load_items_idle

    /* the finish function */
    static void
    finish_load_items (gpointer data_)
    {
        IdleData *data = (IdleData *)data_;
        g_perfmon->log("Library parsed.");
        g_free(data);
        g_main_gui->m_game_refresh_lock.unlock();
    }
    // => finish_load_items

    void 
    on_ask_game_refresh() {
        if (g_main_gui->m_game_refresh_lock.try_lock()) {
            IdleData *data;

            data = g_new(IdleData, 1);
            data->current_item = 0;
            data->state = STATE_STARTED;
            g_main_gui->outstanding_icon_downloads = 0;

            // Use low priority so we don't block showing the main window
            // This allows the main window to show up immediately
            g_idle_add_full (G_PRIORITY_LOW,
                            load_items_idle,
                            data,
                            finish_load_items);
        } else {
            std::cerr << "Not refreshing games because a refresh is already in progress" << std::endl;
        }
    }
    // => on_ask_game_refresh

    void 
    on_main_window_show() {
        on_ask_game_refresh();
    }
    // => on_main_window_show

    void
    on_about_button_clicked() {
        g_main_gui->show_about_dialog();
    }

    void
    on_about_dialog_close_button_clicked() {
        g_main_gui->hide_about_dialog();
    }

    void
    on_search_changed(GtkWidget* search_widget) {
        const char* filter_text = gtk_entry_get_text( GTK_ENTRY(search_widget) );

        //if !g_steam->isgamerunning
        g_main_gui->filter_games(filter_text);
        //else 
        //g_main_gui->filter_stats(filter_text)
    }
    // => on_search_changed


    void 
    on_game_row_activated(GtkListBox *box, GtkListBoxRow *row) {

        const AppId_t appId = g_main_gui->get_corresponding_appid_for_row(row);

        if( appId != 0 ) {
            g_main_gui->switch_to_stats_page();
            g_steam->launch_game(appId);
            populate_achievements();
        } else {
            std::cerr << "An error occurred figuring out which app to launch.. You can report this to the developer." << std::endl;
        }

    }
    // => on_game_row_activated

    void
    on_back_button_clicked() {
        g_steam->quit_game();
        g_main_gui->switch_to_games_page();
    }
    // => on_back_button_clicked
}