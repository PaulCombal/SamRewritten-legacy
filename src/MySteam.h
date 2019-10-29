#pragma once
#include "types/Game.h"
#include "types/Achievement.h"
#include "GameServerManager.h"
#include "sockets/MyClientSocket.h"
#include <string>
#include <vector>
#include <map>

/**
 * MySteam is the highest-level class of the program. Use it to 
 * fake launch games, request unlocks, get icons, etc.
 */
class MySteam {
public:
    /**
     * Returns the unique instance of this object.
     * See "Singleto design pattern" for further help
     */
    static MySteam* get_instance();

    /**
     * Returns the absolute path to the steam installation folder.
     * This is not failsafe and may require some tweaking to add 
     * support for your distribution
     */
    static std::string get_steam_install_path();

    /**
     * Starts a process that will emulate a steam game with the 
     * given appId. Returns false if this process failed to launch.
     * The process may start successfully but fail during execution.
     */
    bool launch_game(AppId_t appId);

    /**
     * Stops the process started with the above method launch_game.
     * Returns true if a process was successfully stopped.
     */
    bool quit_game();

    /**
     * Makes a list of all owned games with stats or achievements.
     */
    void refresh_owned_apps();

    /**
     * Fetches all the app icons either online or on the disk.
     */
    static void refresh_icon(AppId_t app_id);

    /**
     * Returns all the already loaded retrieved apps by the latest logged 
     * in user. Make sure to call refresh_owned_apps at least once to get 
     * correct results
     */
    std::vector<Game_t> get_subscribed_apps() { return m_all_subscribed_apps; };

    /**
     * Get achievements of the launched app
     * 
     * For now use an Achievement_t for ease of extension
     * to count-based achievements
     * 
     * TODO: maybe don't name this the same as GameServer::get_achievements?
     */ 
    std::vector<Achievement_t> get_achievements();

    /**
     * Adds a modification to be done on the launched app.
     */
    void add_modification_ach(const std::string& ach_id, const bool& new_value);

    /**
     * Adds a modification to be done on the launched app.
     */
    void remove_modification_ach(const std::string& ach_id);

    /**
     * Adds a modification to be done on the launched app.
     * Commit the change with commit_modifications.
     */
    //void add_modification_stat(const std::string& stat_id, const double& new_value); // TODO: IMPLEMENT

    /**
     * Commit pending changes
     */
    void commit_changes();

    /**
     * Clear pending changes without committing them.
     */
    void clear_changes();

    MySteam(MySteam const&)                 = delete;
    void operator=(MySteam const&)          = delete;
private:
    MySteam();

    /**
     * Compare the app_name of a Game_t for sorting
     */
    static bool comp_app_name(Game_t app1, Game_t app2);

    GameServerManager m_server_manager;
    MyClientSocket* m_ipc_socket;

    std::vector<Game_t> m_all_subscribed_apps;
    std::map<std::string, bool> m_pending_ach_modifications;
    std::map<std::string, double> m_pending_stat_modifications;
};