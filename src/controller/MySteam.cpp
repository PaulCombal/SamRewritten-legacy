#include "MySteam.h"
#include "SteamAppDAO.h"
#include "../types/Game.h"
#include "../types/Actions.h"
#include "../common/functions.h"
#include "../json/yajlHelpers.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <dirent.h>
#include <bits/stdc++.h>

MySteam::MySteam() {
    // Cache folder
    if (getenv("XDG_CACHE_HOME")) {
        m_cache_folder = std::string(getenv("XDG_CACHE_HOME")) + "/SamRewritten";
    } else {
        m_cache_folder = std::string(getenv("HOME")) + "/.cache/SamRewritten";
    }
    mkdir_default(m_cache_folder.c_str());

    // Runtime folder
    if (getenv("XDG_RUNTIME_DIR")) {
        m_runtime_folder = std::string(getenv("XDG_RUNTIME_DIR")) + "/SamRewritten";
        mkdir_default(m_runtime_folder.c_str());
    } else {
        std::cerr << "XDG_RUNTIME_DIR is not set! Your distribution is improper... falling back to cache dir" << std::endl;
        m_runtime_folder = m_cache_folder;
    }

    // Steam folder
    std::string data_home_path;
    if (getenv("XDG_DATA_HOME") != NULL) {
        data_home_path = getenv("XDG_DATA_HOME");
    } else {
        data_home_path = getenv("HOME") + std::string("/.local/share");
    }

    if (file_exists(data_home_path + "/Steam/appcache/appinfo.vdf")) {
        m_steam_install_dir = std::string(data_home_path + "/Steam");
        return;
    }

    const std::string home_path = getenv("HOME");
    if (file_exists(home_path + "/.steam/appcache/appinfo.vdf")) {
        m_steam_install_dir = std::string(home_path + "/.steam");
        return;
    }
    else if (file_exists(home_path + "/.steam/steam/appcache/appinfo.vdf")) {
        m_steam_install_dir = std::string(home_path + "/.steam/steam");
        return;
    }
    else if (file_exists(home_path + "/.var/app/com.valvesoftware.Steam/.local/share/Steam/appcache/appinfo.vdf")) {
        m_steam_install_dir = std::string(home_path + "/.var/app/com.valvesoftware.Steam/.local/share/Steam");
        return;
    }
    else {
        std::cerr << "Unable to locate the steam directory." << std::endl;
        zenity("Unable to find your Steam installation directory.. Please report this on Github!");
        exit(EXIT_FAILURE);
    }
}
// => Constructor

/**
 * Compare the app_name of a Game_t for sorting
 */
bool 
MySteam::comp_app_name(Game_t app1, Game_t app2) {
    return strcasecmp(app1.app_name.c_str(), app2.app_name.c_str()) < 0;
}
// => comp_app_name

/**
 * Compare the modification number for retriving the selection order
 */
bool 
MySteam::comp_change_num(AchievementChange_t change1, AchievementChange_t change2) {
    return change1.selection_num < change2.selection_num;
}
// => comp_change_num

/**
 * Gets the unique instance. See "Singleton design pattern" for help
 */
MySteam* 
MySteam::get_instance() {
    static MySteam instance;
    return &instance;
}
// => get_instance


/**
 * Fakes a new game being launched. Keeps running in the background until quit_game is called.
 */
bool 
MySteam::launch_app(AppId_t appID) {
    // Print an error if a game is already launched
    if (m_ipc_socket != nullptr) {
        std::cerr << "I will not launch the game as one is already running" << std::endl;
        return false;
    }
    
    // Set the appID BEFORE forking, otherwise the child won't be able to access it
    m_app_id = appID;
    m_ipc_socket = m_server_manager.quick_server_create(appID);

    if (m_ipc_socket == nullptr) {
        std::cerr << "Failed to get connection to game" << std::endl;
        m_app_id = 0;
        return false;
    }

    return true;
}
// => launch_app


/**
 * If a fake game is running, stops it and returns true, else false.
 */
bool 
MySteam::quit_game() {
    if (m_ipc_socket != nullptr) {
        m_ipc_socket->kill_server();
        delete m_ipc_socket;
        m_ipc_socket = nullptr;
        return true;
    } else {
        return false;
    }
}
// => quit_game


/**
 * This retrieves all owned apps, can include garbage apps
 * depending on the users settings, in a near future.
 * Stores the owned games in m_all_subscribed_apps.
 */
void 
MySteam::refresh_owned_apps() {
    Game_t game;
    SteamAppDAO* appDAO = SteamAppDAO::get_instance();

    // The whole update will really occur only once in a while, no worries
    appDAO->update_name_database(); // Downloads and parses app list from Steam
    m_all_subscribed_apps.clear();

    auto all_apps = appDAO->get_all_apps();
    for (auto pair : all_apps) {
        auto app_id = pair.first;
        if (appDAO->app_is_owned(app_id))
        {
            game.app_id = pair.first;
            game.app_name = pair.second;

            m_all_subscribed_apps.push_back(game);
        }
    }

    std::sort(m_all_subscribed_apps.begin(), m_all_subscribed_apps.end(), comp_app_name);
}
// => refresh_owned_apps

/**
 * Reminder that download_app_icon does check if the file is 
 * already there before attempting to download from the web.
 */
void 
MySteam::refresh_app_icon(AppId_t app_id) {
    SteamAppDAO *appDAO = SteamAppDAO::get_instance();
    appDAO->download_app_icon(app_id);
}
// => refresh_app_icon

void
MySteam::refresh_achievement_icon(std::string id, std::string icon_download_name) {
    SteamAppDAO::get_instance()->download_achievement_icon(m_app_id, id, icon_download_name);
}
// => refresh_achievement_icon

void
MySteam::refresh_achievements_and_stats() {

    if (m_ipc_socket == nullptr) {
        std::cerr << "Connection to game is broken" << std::endl;
        zenity("Connection to game is broken");
        exit(EXIT_FAILURE);
    }

    std::string response = m_ipc_socket->request_response(make_get_achievements_request_string());

    if (!decode_ack(response)) {
        std::cerr << "Failed to receive ack!" << std::endl;
    }

    m_achievements = decode_achievements(response);
    m_stats = decode_stats(response);

    set_special_flags();
}
// => refresh_achievements

/**
 * Adds an achievement to the list of achievements to unlock/lock
 */
void 
MySteam::add_modification_ach(const std::string& ach_id, bool new_value) {
    #ifdef DEBUG_CERR
    std::cout << "Adding achievement modification: " << ach_id << ", " << (new_value ? "to unlock" : "to relock") << std::endl;
    #endif

    // A value to save off the order we put them in the map
    static uint64_t selection_num = 0;

    if ( m_pending_ach_modifications.find(ach_id) == m_pending_ach_modifications.end() ) {

        selection_num++;
        if (selection_num == 0) {
            // We've overflowed UINT64_MAX modifications.. impressive
            // Reset the current array
            for (auto& [key, val] : m_pending_ach_modifications) {
                val.selection_num = ++selection_num;
            }
            selection_num++;
            if (selection_num == 0) {
                std::cerr << "Error: more than UINT64_MAX achievement modifications" << std::endl;
                zenity();
                exit(EXIT_FAILURE);
            }
        }
        
        m_pending_ach_modifications.insert( std::pair<std::string, AchievementChange_t>(ach_id, (AchievementChange_t){ach_id, new_value, selection_num} ) );
    } else {
        std::cerr << "Warning: Cannot append " << ach_id << ", value already exists." << std::endl;
    }
}
// => add_modification_ach

/**
 * Removes an achievement to the list of achievements to unlock/lock
 */
void 
MySteam::remove_modification_ach(const std::string& ach_id) {
    #ifdef DEBUG_CERR
    std::cout << "Removing achievement modification: " << ach_id << std::endl;
    #endif

    if ( m_pending_ach_modifications.find(ach_id) == m_pending_ach_modifications.end() ) {
        std::cerr << "WARNING: Could not cancel: modification was not pending: " << ach_id << std::endl;
    } else {
        m_pending_ach_modifications.erase(ach_id);
    }
}
// => remove_modification_ach

/**
 * Adds a stat modification to be done on the launched app.
 * Commit the change with commit_changes
 */
void
MySteam::add_modification_stat(const StatValue_t& stat, std::any new_value) {
    // The value must already be the proper type for it to be added to the list
    #ifdef DEBUG_CERR
    std::cout << "Adding stat modification: " << stat.id << ", ";
    if (stat.type == UserStatType::Integer) {
        std::cout << "Integer " << std::to_string(std::any_cast<long long>(new_value));
    } else if (stat.type == UserStatType::Float) {
        std::cout << "Float " << std::to_string(std::any_cast<double>(new_value));
    }
    std::cout << std::endl;
    #endif

    if (stat.type != UserStatType::Integer && stat.type != UserStatType::Float)
    {
        std::cerr << "The stat with id \"" + stat.id + "\" has unrecognized type, but this should have been caught in the caller" << std::endl;
        zenity("An internal programming error for \"" + stat.id + "\" has occured. Please notify the developers on Github.");
        // This isn't fatal, so let's not annoy the user by exiting.
        return;
    }

    if ( m_pending_stat_modifications.find(stat.id) == m_pending_stat_modifications.end() ) {
        m_pending_stat_modifications.insert( std::pair<std::string, StatChange_t>(stat.id, (StatChange_t){stat.type, stat.id, new_value} ) );
    } else {
        std::cerr << "Warning: Cannot append " << stat.id << ", value already exists." << std::endl;
    }
}
// => add_modification_stat

/**
 * Removes a stat modification that would have been done on the launched app.
 */
void
MySteam::remove_modification_stat(const StatValue_t& stat) {
    #ifdef DEBUG_CERR
    std::cout << "Removing stat modification: " << stat.id << std::endl;
    #endif

    // If there's not one pending, don't treat it as a warning currently
    // because we don't really care to differentiate the
    // 0->1 and 2->1 character length transitions over in StatBoxRow
    m_pending_stat_modifications.erase(stat.id);
}
// => remove_modification_stat

/**
 * Commit pending achievement and stat changes and clear the current
 * list because it's now stale
 */
void
MySteam::commit_changes() {
    for ( const auto& [key, val] : m_pending_ach_modifications) {
        m_achievement_changes.push_back(val);
    }

    for ( const auto& [key, val] : m_pending_stat_modifications) {
        m_stat_changes.push_back(val);
    }
    
    std::string response = m_ipc_socket->request_response(make_commit_changes_request_string(m_achievement_changes, m_stat_changes));

    if (!decode_ack(response)) {
        std::cerr << "Failed to commit changes!" << std::endl;
    }

    clear_changes();
}
// => commit_changes

std::vector<uint64_t>
MySteam::setup_timed_modifications(uint64_t seconds, MODIFICATION_SPACING spacing, MODIFICATION_ORDER order) {
    // One more idea is to add the ability to commit modifications in
    // the order of % players achieved, but that requires going and
    // retrieving or otherwise carrying along that value. We can add
    // that if anyone really wants it. That would also require actually
    // fetching achievements in CLI mode.

    std::vector<uint64_t> times;
    std::vector<uint64_t> rel_times;
    size_t size = m_pending_ach_modifications.size();

    if (size == 0) {
        return times;
    }

    // Generate spacings
    for (size_t i = 0; i < size; i++)
    {
        if (spacing == EVEN_SPACING) {
            times.push_back((i + 1) * (seconds / size));
        } else {
            times.push_back(seconds * (((double)rand()) / RAND_MAX));
        }
    }

    for ( const auto& [key, val] : m_pending_ach_modifications) {
        m_achievement_changes.push_back(val);
    }

    // Apply ordering
    if (order == SELECTION_ORDER) {
        std::sort(m_achievement_changes.begin(), m_achievement_changes.end(), comp_change_num);
    } else {
        std::random_shuffle(m_achievement_changes.begin(), m_achievement_changes.end());
    }

    for (size_t i = 0; i < size; i++) {
        std::cout << "Modify achievement " << m_achievement_changes[i].id << " in " << times[i] << " seconds"
                  << " (or " << (((double)times[i]) / 60) << " minutes or " << ((((double)times[i]) / 60) / 60) << " hours)" << std::endl;
    }

    // Put times in order since we'll use the differences from one to the next
    std::sort(times.begin(), times.end());

    // Make relative times
    rel_times.push_back(times[0]);
    for (size_t i = 1; i < size; i++)
    {
        rel_times.push_back(times[i] - times[i - 1]);
    }

    return rel_times;   
}
// => commit_timed_modifications

void
MySteam::commit_next_timed_modification() {
    // Give the function a dummy array with just the one change
    std::vector<AchievementChange_t> achievement_change;
    achievement_change.push_back(m_achievement_changes[0]);
    std::string response = m_ipc_socket->request_response(make_commit_changes_request_string(achievement_change, m_stat_changes));
    if (!decode_ack(response)) {
        std::cerr << "Failed to commit change!" << std::endl;
    }

    m_achievement_changes.erase(m_achievement_changes.begin());

    if (m_achievement_changes.empty()) {
        // Just clear the stats for now even though we don't apply them...
        // The GUI caller is going to reset the display anyway
        clear_changes();
    }
}

void
MySteam::clear_changes() {
    // Clear all pending changes
    m_pending_ach_modifications.clear();
    m_pending_stat_modifications.clear();
    m_achievement_changes.clear();
    m_stat_changes.clear();
}
// => clear_changes

void
MySteam::set_special_flags() {
    // TODO: Maybe split this up to be more amenable to threaded GUI loading, could fire off in thread

    long next_most_achieved_index = -1;
    float next_most_achieved_rate = 0;
    Achievement_t tmp;

    for(size_t i = 0; i < m_achievements.size(); i++)
    {
        tmp = m_achievements[i];
        m_achievements[i].special = ACHIEVEMENT_NORMAL;

        if ( !tmp.achieved && tmp.global_achieved_rate > next_most_achieved_rate )
        {
            next_most_achieved_rate = tmp.global_achieved_rate;
            next_most_achieved_index = i;
        }

        if ( tmp.global_achieved_rate <= 5.f )
        {
            m_achievements[i].special = ACHIEVEMENT_RARE;
        }
    }

    if ( next_most_achieved_index != -1 )
    {
        m_achievements[next_most_achieved_index].special |= ACHIEVEMENT_NEXT_MOST_ACHIEVED;
    }
}

