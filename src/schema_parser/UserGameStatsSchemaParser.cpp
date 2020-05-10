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

#include <strings.h>
#include <memory>

#include "UserGameStatsSchemaParser.h"
#include "KeyValue.h"
#include "../types/UserStatType.h"
#include "../controller/MySteam.h"
#include "../globals.h"

#include "../types/IntegerStatDefinition.h"
#include "../types/FloatStatDefinition.h"


bool 
UserGameStatsSchemaParser::load_user_game_stats_schema() {
    m_icon_download_names.clear();
    m_permissions.clear();
    clear_stat_definitions();

    std::string appid_string = std::to_string(g_steam->get_current_appid());
    std::string schema_file = g_steam->get_steam_install_path() + "/appcache/stats/UserGameStatsSchema_" + appid_string + ".bin";
    KeyValue* kv = KeyValue::load_as_binary(schema_file);
    if (kv == NULL) {
        return false;
    }

    auto stats = kv->get2(appid_string, "stats");
    if (stats->valid == false || stats->children.size() == 0) {
        delete kv;
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
                IntegerStatDefinition* def = new IntegerStatDefinition;
                def->type = UserStatType::Integer;
                def->Id = stat->get("name")->as_string("");
                def->DisplayName = stat->get2("display", "name")->as_string("");
                def->MinValue = stat->get("min")->as_integer(0);
                def->MaxValue = stat->get("max")->as_integer(0);
                def->MaxChange = stat->get("maxchange")->as_integer(0);
                def->IncrementOnly = stat->get("incrementonly")->as_boolean(false);
                def->DefaultValue = stat->get("default")->as_integer(0);
                def->Permission = stat->get("permission")->as_integer(0);
                m_stats.push_back(def);
                break;
            }

            case UserStatType::Float:
            case UserStatType::AverageRate:
            {
                FloatStatDefinition* def = new FloatStatDefinition;
                def->type = UserStatType::Float;
                def->Id = stat->get("name")->as_string("");
                def->DisplayName = stat->get2("display", "name")->as_string("");
                def->MinValue = stat->get("min")->as_float(0);
                def->MaxValue = stat->get("max")->as_float(0);
                def->MaxChange = stat->get("maxchange")->as_float(0);
                def->IncrementOnly = stat->get("incrementonly")->as_boolean(false);
                def->DefaultValue = stat->get("default")->as_float(0);
                def->Permission = stat->get("permission")->as_integer(0);
                m_stats.push_back(def);
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
                        if (!id_kv->valid) {
                            std::cerr << "Failed to parse achievement id" << std::endl;
                            continue;
                        }

                        // just get regular icon for now, not icon_gray
                        auto icon_kv = bit->get2("display", "icon");
                        if (!icon_kv->valid) {
                            std::cerr << "Failed to parse achievement icon" << std::endl;
                            continue;
                        }

                        // The permission key is not always present, so it is not
                        // an error for it not to be parsed out.
                        auto permission_kv = bit->get("permission");

                        // inject into a map for later extraction and icon downloading
                        m_icon_download_names.insert(std::make_pair(id_kv->as_string(""), icon_kv->as_string("")));
                        m_permissions.insert(std::make_pair(id_kv->as_string(""), permission_kv->as_integer(0)));
                    }
                }

                break;
            }

            default:
            {
                std::cerr << "Invalid stat type" << std::endl;
                delete kv;
                return false;
            }
        }
    }

    delete kv;
    return true;
}

void
UserGameStatsSchemaParser::clear_stat_definitions() {
    for (auto p : m_stats )
    {
        delete p;
    }
    m_stats.clear();
}