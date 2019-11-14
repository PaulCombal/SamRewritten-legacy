/* Copyright (c) 2017 Rick (rick 'at' gibbed 'dot' us)
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would
 *    be appreciated but is not required.
 * 
 * 2. Altered source versions must be plainly marked as such, and must not
 *    be misrepresented as being the original software.
 * 
 * 3. This notice may not be removed or altered from any source
 *    distribution.
 */

// This code is ported to C++ from gibbed's original Steam Achievement Manager.
// The original SAM is available at https://github.com/gibbed/SteamAchievementManager
// To comply with copyright, the above license is included.

#include "KeyValue.h"
#include "UserStatType.h"
#include "../MySteam.h"
#include "../globals.h"
#include <strings.h>

// this has at least as much error checking as the original version

bool load_user_game_stats_schema() {
    g_steam->m_icon_download_names.clear();

    std::string appid_string = std::to_string(g_steam->m_app_id);
    std::string schema_file = MySteam::get_steam_install_path() + "/appcache/stats/UserGameStatsSchema_" + appid_string + ".bin";
    KeyValue* kv = KeyValue::load_as_binary(schema_file);
    if (kv == NULL) {
        return false;
    }

    auto stats = kv->get2(appid_string, "stats");
    if (stats == NULL) {
        return false;
    }

    if (stats->valid == false || stats->children.size() == 0) {
        return false;
    }

    for (auto stat : stats->children) {
        if (stat->valid == false) {
            continue;
        }

        int rawType = stat->get("type_int")->valid
                    ? stat->get("type_int")->as_integer(0)
                    : stat->get("type")->as_integer(0);
        
        UserStatType type = static_cast<UserStatType>(rawType);

        switch (type)
        {
            case UserStatType::Invalid:
            {
                break;
            }

            case UserStatType::Integer:
            {
                // don't care currently
                break;
            }

            case UserStatType::Float:
            case UserStatType::AverageRate:
            {
                // don't care currently
                break;
            }

            case UserStatType::Achievements:
            case UserStatType::GroupAchievements:
            {
                if (stat->children.size() == 0) {
                    continue;
                }

                for (auto bits : stat->children) {
                    if (strcasecmp(bits->name.c_str(), "bits") != 0) {
                        continue;
                    }

                    if (bits->valid == false || bits->children.size() == 0) {
                        continue;
                    }

                    for (auto bit : bits->children) {
                        // don't care about the rest for now

                        auto id_kv = bit->get("name");
                        if (id_kv == NULL) {
                            std::cerr << "Failed to parse achievement id" << std::endl;
                            continue;
                        }

                        // just get regular icon for now, not icon_gray
                        auto icon_kv = bit->get2("display", "icon");
                        if (icon_kv == NULL) {
                            std::cerr << "Failed to parse achievement icon" << std::endl;
                            continue;
                        }

                        // inject into a map for later extraction and icon downloading
                        g_steam->m_icon_download_names.insert(std::make_pair(id_kv->as_string(""), icon_kv->as_string("")));
                    }
                }

                break;
            }

            default:
            {
                std::cerr << "invalid stat type" << std::endl;
                return false;
            }
        }
    }

    return true;
}