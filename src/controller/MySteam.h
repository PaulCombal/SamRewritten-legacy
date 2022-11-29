#pragma once
#include "../types/Game.h"
#include "../types/Achievement.h"
#include "../types/StatValue.h"
#include "../sockets/MyClientSocket.h"
#include "GameServerManager.h"
#include <string>
#include <vector>
#include <map>

enum MODIFICATION_SPACING {
    EVEN_SPACING = 0,
    RANDOM_SPACING = 1,
    EVEN_FREE_SPACING = 2,
};

enum MODIFICATION_ORDER {
    SELECTION_ORDER = 0,
    RANDOM_ORDER = 1,
};

enum MODIFICATION_ACHIEVED {
    ACHIEVED = 0,
    NOT_ACHIEVED = 1,
    ACHIEVED_ALL = 2,
};

enum MODIFICATION_PROTECTED {
    PROTECTED = 0,
    NOT_PROTECTED = 1,
    PROTECTED_ALL = 2,
};

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
    std::string get_steam_install_path() const { return m_steam_install_dir; };

    /**
     * Returns the absolute path to the runtime folder used by SAM.
     * It's used for storing the UNIX sockets used by the program.
     * 
     * XDG Base Directory Specification is followed:
     * If present, this variable uses XDG_RUNTIME_DIR and takes the value
     *      $XDG_RUNTIME_DIR/SamRewritten
     * 
     * Warnings are properly issued if it is not set, but  still defaults to the
     * cache folder for simplicity
     */
    std::string get_runtime_path() const { return m_runtime_folder; };

    /**
     * Returns the absolute path to the cache folder used by SAM
     * 
     * XDG Base Directory Specification is followed:
     * If present, this variable uses XDG_CACHE_HOME and takes the value
     *      $XDG_CACHE_HOME/SamRewritten
     * Otherwise, this variable properly defaults to
     *      ~/.cache/SamRewritten
     */
    std::string get_cache_path() const { return m_cache_folder; };

    /**
     * Starts a process that will emulate a steam game with the 
     * given appId. Returns false if this process failed to launch.
     * The process may start successfully but fail during execution.
     */
    bool launch_app(AppId_t appId);

    /**
     * Stops the process started with the above method launch_app.
     * Returns true if a process was successfully stopped.
     */
    bool quit_game();

    /**
     * Fetches icon for given app
     */
    static void refresh_app_icon(AppId_t app_id);

    /**
     * Fetches all the achievment icon for a given app and 
     * stores it as id.jpg for ease of identification
     * Not static because it uses the cached m_app_id to know
     * which folder to put the icon in
     */
    void refresh_achievement_icon(std::string id, std::string icon_download_name);

    /**
     * Makes a list of all owned games with stats or achievements.
     */
    void refresh_owned_apps();

    /**
     * Returns all the already loaded retrieved apps by the latest logged 
     * in user. Make sure to call refresh_owned_apps at least once to get 
     * correct results
     */
    std::vector<Game_t> get_subscribed_apps() { return m_all_subscribed_apps; };

    /**
     * Makes a list of all achievements for the currently running app
     */
    void refresh_achievements_and_stats();

    /**
     * Get achievements of the launched app
     * 
     * For now use an Achievement_t for ease of extension
     * to count-based achievements
     * 
     * Make sure to call refresh_achievements at least once to get
     * correct results
     * 
     * The same goes for the stats
     * 
     * TODO: maybe don't name this the same as GameServer::get_achievements?
     */ 
    std::vector<Achievement_t> get_achievements() const { return m_achievements; };
    std::vector<StatValue_t> get_stats() const { return m_stats; };

    /**
     * Simple getter
     */
    AppId_t get_current_appid() const { return m_app_id; };

    /**
     * Adds an achievement modification to be done on the launched app.
     * Commit the change with commit_changes
     */
    void add_modification_ach(const std::string& ach_id, bool new_value);

    /**
     * Removes an achievement modificatiothat would have been done on the launched app.
     */
    void remove_modification_ach(const std::string& ach_id);

    /**
     * Adds a stat modification to be done on the launched app.
     * Commit the change with commit_changes
     */
    void add_modification_stat(const StatValue_t& stat, std::any new_value);

    /**
     * Removes a stat modification that would have been done on the launched app.
     */
    void remove_modification_stat(const StatValue_t& stat);

    /**
     * Commit pending changes
     */
    void commit_changes();

    /**
     * Setup a timed unlock
     * The caller is responsible for calling commit_next_timed_modification
     * after the next time value has elapsed. Times are relative to the previous
     * one in the series.
     */
    std::vector<uint64_t> setup_timed_modifications(uint64_t time, MODIFICATION_SPACING spacing, MODIFICATION_ORDER order);

    /**
     * Modify the next achievement in a series 
     */
    void commit_next_timed_modification(void);

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

    /**
     * Compare the modification number for retriving the selection order
     */
    static bool comp_change_num(AchievementChange_t change1, AchievementChange_t change2);

    /**
     * Add special flags to achievements
     */
    void set_special_flags();

    // Absolute path to some important directories
    std::string m_steam_install_dir;
    std::string m_cache_folder;
    std::string m_runtime_folder;

    // Current app_id
    AppId_t m_app_id;

    GameServerManager m_server_manager;
    MyClientSocket* m_ipc_socket;

    std::vector<Game_t> m_all_subscribed_apps;
    std::vector<Achievement_t> m_achievements;
    std::vector<StatValue_t> m_stats;

    std::map<std::string, AchievementChange_t> m_pending_ach_modifications;
    std::map<std::string, StatChange_t> m_pending_stat_modifications;

    std::vector<AchievementChange_t> m_achievement_changes;
    std::vector<StatChange_t> m_stat_changes;
};
