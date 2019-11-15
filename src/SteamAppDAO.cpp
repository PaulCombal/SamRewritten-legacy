#include "SteamAppDAO.h"
#include <ctime>
#include <sys/stat.h>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <yajl/yajl_tree.h>
#include <stdio.h>
#include <stdlib.h>
#include "common/functions.h"
#include "common/Downloader.h"
#include "../steam/steam_api.h"
#include "MySteamClient.h"
#include "gui/MainPickerWindow.h"
#include "globals.h"

// Wtf am I doing? Anyway thanks StackOverflow
//TODO: Find a more elegant way to fix this shit.
std::map<AppId_t, std::string> SteamAppDAO::m_app_names = std::map<AppId_t, std::string>();

/**
 * Lazy singleton pattern
 */
SteamAppDAO*
SteamAppDAO::get_instance() {
    static SteamAppDAO me;
    return &me;
}
// => get_instance

bool
SteamAppDAO::need_to_redownload(const char * file_path) {
    struct stat file_info;
    const std::time_t current_time(std::time(0));
    bool b_need_to_redownload = false;

    if (file_exists(file_path)) {
        //Check the last time it was updated
        if (stat(file_path, &file_info) == 0) {
            //If a week has passed
            if (current_time - file_info.st_mtime > 60 * 60 * 24 * 7) {
                b_need_to_redownload = true;
            }
        }
        else {
            std::cerr << "~/.cache/SamRewritten/app_names exists but an error occurred analyzing it. To avoid further complications, ";
            std::cerr << "the program will stop here. Before retrying make sure you have enough privilege to read and write to ";
            std::cerr << "your home folder folder." << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    else {
        b_need_to_redownload = true;
    }

    return b_need_to_redownload;
}

void 
SteamAppDAO::update_name_database() {
    bool b_need_to_redownload_any = false;

    // Appid list retrieval strategy options:
    // Option 1 - get the full games-only list
    // Option 2 - if the games-only list is out of date, get the non-games list,
    //            and filter those out from Steam's list. Then you only get junk
    //            appids at the end
    // Option 3 - if github is unavailable, just use the appids from Steam's list
    //            and get all the junk
    //
    // The formats of the appid files are the same for both Steam's list and our
    // generated SteamAppsListDumps, so we can use the same parse_app_names function :)

    // TODO: turn this into a command line option, or a setting
    const int retrieval_strategy = 1;

    static const char* file_url[2];
    static const char* local_file_name[2];
    int file_count = 0;
    
    if (retrieval_strategy == 1) {
        file_url[0] = "https://raw.githubusercontent.com/PaulCombal/SteamAppsListDumps/master/game_achievements_list.json";
        local_file_name[0] = concat(g_cache_folder, "/game_list.json");
        file_count = 1;
    } else if (retrieval_strategy == 2) {
        // need to download 2 files for this one
        file_url[0] = "http://api.steampowered.com/ISteamApps/GetAppList/v0002/";
        local_file_name[0] = concat(g_cache_folder, "/app_names");
        file_url[1] = "https://raw.githubusercontent.com/PaulCombal/SteamAppsListDumps/master/not_games.json";
        local_file_name[1] = concat(g_cache_folder, "/not_games.json");
        file_count = 2;
    } else if (retrieval_strategy == 3) {
        file_url[0] = "http://api.steampowered.com/ISteamApps/GetAppList/v0002/";
        local_file_name[0] = concat(g_cache_folder, "/app_names");
        file_count = 1;
    }

    for (int i = 0; i < file_count; i++) {
        bool b_need_to_redownload_one = need_to_redownload(local_file_name[i]);

        if (b_need_to_redownload_one) {
            Downloader::get_instance()->download_file(file_url[i], local_file_name[i]);        
        }

        b_need_to_redownload_any |= b_need_to_redownload_one;
    }

    // If there's a new list downloaded, or if there's an up-to-date list
    // and the program just started, we need to fill/refill it
    if (b_need_to_redownload_any || m_app_names.empty()) {
        parse_app_names(local_file_name[0], &m_app_names);

        if (retrieval_strategy == 2) {
            std::map<AppId_t, std::string> not_games;
            parse_app_names(local_file_name[1], &not_games);

            // Filter out not games
            for(auto it = not_games.begin(); it != not_games.end(); ) {
                m_app_names.erase( it->first );
            }
        }
    }

}

std::string 
SteamAppDAO::get_app_name(AppId_t app_id) {
    return m_app_names[app_id];
}


void 
SteamAppDAO::download_app_icon(AppId_t app_id) {
    const std::string local_folder(std::string(g_cache_folder) + "/" + std::to_string(app_id));
    const std::string local_path = get_app_icon_path(app_id);
    const std::string url("http://cdn.akamai.steamstatic.com/steam/apps/" + std::to_string(app_id) + "/header_292x136.jpg");

    mkdir_default(local_folder.c_str());
    Downloader::get_instance()->download_file(url, local_path);
}

void
SteamAppDAO::download_achievement_icon(AppId_t app_id, std::string id, std::string icon_download_name) {
    const std::string local_folder(std::string(g_cache_folder) + "/" + std::to_string(app_id));
    const std::string local_path = get_achievement_icon_path(app_id, id);
    const std::string url("http://media.steamcommunity.com/steamcommunity/public/images/apps/" + std::to_string(app_id) + "/" + icon_download_name); 

    mkdir_default(local_folder.c_str());
    Downloader::get_instance()->download_file(url, local_path);
}

void
SteamAppDAO::parse_app_names(const char * file_path, std::map<AppId_t, std::string>* app_names) {
    app_names->clear();

    size_t rd;
    yajl_val node;
    char errbuf[1024];
    char fileData[5000000];
    FILE *f = fopen(file_path, "rb");

    /* null plug buffers */
    fileData[0] = errbuf[0] = 0;

    /* read the entire config file */
    rd = fread((void *) fileData, 1, sizeof(fileData) - 1, f);

    /* file read error handling */
    if (rd == 0 && !feof(stdin)) {
        std::cerr << "error encountered on file read" << std::endl;
        exit(EXIT_FAILURE);
    } else if (rd >= sizeof(fileData) - 1) {
        std::cerr << "app_names file too big (just increase the buffer size)" << std::endl;
        exit(EXIT_FAILURE);
    }

    /* we have the whole config file in memory. let's parse it ... */
    node = yajl_tree_parse((const char *) fileData, errbuf, sizeof(errbuf));

    /* parse error handling */
    if (node == NULL) {
        std::cerr << "Parsing error: ";
        if (strlen(errbuf)) {
            std::cerr << errbuf << std::endl;
        } else {
            std::cerr << "Unknown error" << std::endl;
        }
        exit(EXIT_FAILURE);
    }

    /* Save the result */
    const char * path[] = { "applist", "apps", (const char*)0 };
    yajl_val v = yajl_tree_get(node, path, yajl_t_array);
    if (v == NULL) {
        std::cerr << "app_names contains valid JSON, but its format is not supported" << std::endl;
        exit(EXIT_FAILURE);
    }

    unsigned array_length = v->u.array.len;
    AppId_t tmp_appid;
    std::string tmp_appname;
    for(unsigned i = 0; i < array_length; i++) {
        yajl_val obj = v->u.array.values[i];
        //std::cerr << "Appid: " << obj->u.object.values[0]->u.number.i << ", name: " << obj->u.object.values[1]->u.string << std::endl;
        tmp_appid = obj->u.object.values[0]->u.number.i;
        tmp_appname = (std::string)obj->u.object.values[1]->u.string;
        app_names->insert(std::pair<AppId_t, std::string>(tmp_appid, tmp_appname));
    }

    fclose(f);
    yajl_tree_free(node);
}

bool 
SteamAppDAO::app_is_owned(const AppId_t& app_id) {
    ISteamApps* sa = g_steamclient->getSteamApps();
    
    return sa->BIsSubscribedApp(app_id);
}