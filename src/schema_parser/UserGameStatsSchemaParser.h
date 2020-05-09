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

#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "../types/StatDefinition.h"


class UserGameStatsSchemaParser
{
public:
    /**
     * Parse the schema file and store information in g_steam
     * For now, just parse out icon download information.
     * This has the potential to parse out much more info as needed.
     */
    bool load_user_game_stats_schema();

    /**
     * Simple getters
     */
    std::map<std::string, std::string> get_icon_download_names() const { return m_icon_download_names; };
    std::map<std::string, int> get_permissions() const { return m_permissions; };
    std::vector<StatDefinition*> get_stat_definitions() const { return m_stats; };

    /**
     * Because stats are pointers, we need to free them before clearing the array
     * Should I use make_unique instead? Probably
     */
    void clear_stat_definitions();

private:
    // Mapping between achievement ID and the actual icon name on servers.
    // Icon name is retrieved by the stats schema parser
    std::map<std::string, std::string> m_icon_download_names;
    std::map<std::string, int> m_permissions;

    // Container for app stats. Pointer for later casts
    std::vector<StatDefinition*> m_stats;
};