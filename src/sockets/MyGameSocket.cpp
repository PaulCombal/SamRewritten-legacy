#include <yajl/yajl_gen.h>
#include <yajl/yajl_tree.h>
#include "MyGameSocket.h"
#include "../types/Actions.h"
#include "../types/IntegerStatDefinition.h"
#include "../types/FloatStatDefinition.h"
#include "../json/ProcessedGameServerRequest.h"
#include "../json/yajlHelpers.h"
#include "../globals.h"
#include "../common/PerfMon.h"
#include "../common/functions.h"
#include "../schema_parser/UserGameStatsSchemaParser.h"

#include "../controller/MySteam.h"

MyGameSocket::MyGameSocket(AppId_t appid) :
MyServerSocket(appid),
m_CallbackUserStatsReceived( this, &MyGameSocket::OnUserStatsReceived )
{

}

std::string
MyGameSocket::process_request(std::string request, bool& quit) {
    ProcessedGameServerRequest r(request);
    std::string ret;
    const unsigned char * buf;
    size_t len;

    // Generate the ack
    yajl_gen handle = yajl_gen_alloc(NULL); 
    yajl_gen_map_open(handle);

    encode_ack(handle);

    switch (r.getAction()) {
        case GET_ACHIEVEMENTS:
            populate_stats_and_achievements();
            encode_achievements_and_stats(handle, get_achievements(), get_stats());  // Write achievements to handle
            break;

        case COMMIT_CHANGES:
            process_changes(r.get_achievement_changes(), r.get_stat_changes());
            break;

        case QUIT_GAME:
            quit = true;
            break;

        default:
            std::cerr << "Invalid command" << std::endl;
            break;
    }

    if (yajl_gen_map_close(handle) != yajl_gen_status_ok) {
        std::cerr << "Failed to make json." << std::endl;
        zenity();
        exit(EXIT_FAILURE);
    }

    yajl_gen_get_buf(handle, &buf, &len);
    ret = std::string((const char*)buf);
    yajl_gen_free(handle);

    #ifdef DEBUG_CERR
    std::cerr << "--> Sending buffer " << ret << std::endl;
    #endif

    return ret;
}

void
MyGameSocket::populate_stats_and_achievements() {
    m_stats_callback_received = false;

    if (!SteamUserStats()->RequestCurrentStats()) {
        std::cerr << "ERROR: User not logged in, exiting." << std::endl;
        zenity("Please login to Steam before using SamRewritten. If this is the case, please report it with a Github issue.");
        exit(EXIT_FAILURE);
    }

    while (!m_stats_callback_received) {
        SteamAPI_RunCallbacks();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    if (!get_global_stats())
    {
        std::cerr << "An error occurred getting global stats." << std::endl;
        return;
    }

    if (!get_global_achievements_stats())
    {
        std::cerr << "An error occurred getting global achievements stats." << std::endl;
        return;
    }
}

bool
MyGameSocket::get_global_stats() {
    if (!m_stats_callback_received)
    {
        std::cerr << "Requesting global stats when current stats haven't been fetched yet" << std::endl;
        return false;
    }
    
    m_global_callback_received = false;
    SteamAPICall_t hSteamApiCall = SteamUserStats()->RequestGlobalStats(1);
    m_GlobalStatsReceivedCallResult.Set( hSteamApiCall, this, &MyGameSocket::OnGlobalStatsReceived );

    while (!m_global_callback_received) {
        SteamAPI_RunCallbacks();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    return true;
}

bool
MyGameSocket::get_global_achievements_stats() {
    if (!m_global_callback_received)
    {
        std::cerr << "Requesting global achievements stats when current global stats haven't been fetched yet" << std::endl;
        return false;
    }
    
    m_global_achievements_callback_ready = false;

    SteamAPICall_t hSteamApiCall = SteamUserStats()->RequestGlobalAchievementPercentages();
    m_GlobalAchievementPercentagesReadyCallResult.Set( hSteamApiCall, this, &MyGameSocket::OnGlobalAchievementPercentagesReceived );

    while (!m_global_achievements_callback_ready) {
        SteamAPI_RunCallbacks();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    return true;
}

void
MyGameSocket::OnUserStatsReceived(UserStatsReceived_t *callback) {
    m_achievement_list.clear();
    m_stats_list.clear();

    // Check if we received the values for the correct app
    if (SteamUtils()->GetAppID() == callback->m_nGameID) {
        if ( k_EResultOK == callback->m_eResult ) {
            ISteamUserStats *stats_api = SteamUserStats();
            m_schema_parser.load_user_game_stats_schema();

            // ==============================
            // RETRIEVE ACHIEVEMENTS
            // ==============================
            const unsigned num_ach = stats_api->GetNumAchievements();

            if (num_ach == 0) {
                std::cerr << "No achievements for current game" << std::endl;
            }

            m_achievement_list.resize(num_ach);

            for (unsigned i = 0; i < num_ach ; i++) {
                const char * pchName = stats_api->GetAchievementName(i);

                m_achievement_list[i].id   = pchName;
                m_achievement_list[i].name = stats_api->GetAchievementDisplayAttribute(pchName, "name");
                m_achievement_list[i].desc = stats_api->GetAchievementDisplayAttribute(pchName, "desc");
                m_achievement_list[i].icon_name = m_schema_parser.get_icon_download_names()[m_achievement_list[i].id];
                m_achievement_list[i].permission = m_schema_parser.get_permissions()[m_achievement_list[i].id];

                // Value set in OnGlobalAchievementPercentagesReceived
                m_achievement_list[i].global_achieved_rate = 0;
                stats_api->GetAchievement(pchName, &(m_achievement_list[i].achieved));
                m_achievement_list[i].hidden = (bool)strcmp(stats_api->GetAchievementDisplayAttribute(pchName, "hidden" ), "0");
            }

            // ============================
            // RETRIEVE STATS
            // ============================

            for ( auto def : m_schema_parser.get_stat_definitions() )
            {
                StatValue_t sv;

                if ( def->type == UserStatType::Integer )
                {
                    IntegerStatDefinition* cast = dynamic_cast<IntegerStatDefinition*>(def);

                    if (cast == NULL) {
                        std::cerr << "Cast error to int for " << def->Id << ", please report to Github!" << std::endl;
                        continue;
                    }

                    int value;
                    bool success = stats_api->GetStat(def->Id.c_str(), &value);
                    if ( !success )
                    {
                        std::cerr << "Unable to get int stat " << def->Id << std::endl;
                        continue;
                    }

                    sv.id = cast->Id;
                    sv.display_name = cast->DisplayName;
                    sv.permission = cast->Permission;
                    sv.type = cast->type;
                    sv.value = value;
                    sv.incrementonly = cast->IncrementOnly;

                    m_stats_list.push_back(sv);
                }
                else if ( def->type == UserStatType::Float )
                {
                    FloatStatDefinition* cast = dynamic_cast<FloatStatDefinition*>(def);

                    if (cast == NULL) {
                        std::cerr << "Cast error to float for " << def->Id << ", please report to Github!" << std::endl;
                        continue;
                    }

                    float value;
                    bool success = stats_api->GetStat(def->Id.c_str(), &value);
                    if ( !success )
                    {
                        std::cerr << "Unable to get float stat " << def->Id << std::endl;
                        continue;
                    }

                    sv.id = cast->Id;
                    sv.display_name = cast->DisplayName;
                    sv.permission = cast->Permission;
                    sv.type = cast->type;
                    sv.value = value;
                    sv.incrementonly = cast->IncrementOnly;

                    m_stats_list.push_back(sv);
                }
            }

        } else {
            std::cerr << "Received stats for the game, but an error occurrred." << std::endl;
        }
    } else {
        std::cerr << "Received stats for wrong game" << std::endl;
    }

    m_stats_callback_received = true;
}

void
MyGameSocket::OnGlobalStatsReceived(GlobalStatsReceived_t *callback, bool bIOFailure) {
    if ( bIOFailure || callback->m_eResult != k_EResultOK )
    {
        std::cerr << "GlobalStatsReceived_t failed! Enum: " << callback->m_eResult << std::endl;
    }

    #ifdef DEBUG_CERR
    std::cout << "Got stats, maybe I can do cool stuff with them, gotta check." << std::endl;
    #endif
    
    m_global_callback_received = true;
}

void
MyGameSocket::OnGlobalAchievementPercentagesReceived(GlobalAchievementPercentagesReady_t *callback, bool bIOFailure) {
    if ( bIOFailure || callback->m_eResult != k_EResultOK )
    {
        std::cerr << "GlobalAchievementPercentagesReady_t failed! Enum: " << callback->m_eResult << std::endl;
    }

    for (Achievement_t& ach : m_achievement_list) {
        float percent;
        bool success = SteamUserStats()->GetAchievementAchievedPercent(ach.id.c_str(), &percent);
        if (success)
        {
            ach.global_achieved_rate = percent;
        }
        else {
            std::cerr << "Could not get global achievement rate for achievement " << ach.id << std::endl;
            ach.global_achieved_rate = 0;
        }
        
    }
    
    m_global_achievements_callback_ready = true;
}

void
MyGameSocket::process_changes(std::vector<AchievementChange_t> achievement_changes, std::vector<StatChange_t> stat_changes) {
    ISteamUserStats *stats_api = SteamUserStats();

    for (AchievementChange_t change : achievement_changes) {

        const char* achievement_id = change.id.c_str();

        if (change.achieved) {
            // We want to unlock an achievement
            std::cerr << "Unlocking achievement " << achievement_id;
            if (stats_api->SetAchievement(achievement_id)) {
                std::cerr << " succeeded";
            } else {
               std::cerr << " failed";
            }
        } else {
            // We want to relock an achievement
            std::cerr << "Relocking achievement " << achievement_id << std::endl;
            if (stats_api->ClearAchievement(achievement_id)) {
                std::cerr << " succeeded";
            } else {
               std::cerr << " failed";
            }
        }
        std::cerr << std::endl;
    }

    for (StatChange_t change : stat_changes) {

        const char* stat_id = change.id.c_str();

        if (change.type == UserStatType::Integer) {
            std::cerr << "Changing Integer stat to value " << std::to_string(std::any_cast<int>(change.new_value));

            if (stats_api->SetStat(stat_id, std::any_cast<int>(change.new_value))) {
                std::cerr << " succeeded";
            } else {
               std::cerr << " failed";
            }
        } else if (change.type == UserStatType::Float) {
            std::cerr << "Changing Float stat to value" << std::to_string(std::any_cast<float>(change.new_value));
            
            if(stats_api->SetStat(stat_id, std::any_cast<float>(change.new_value))) {
                std::cerr << " succeeded";
            } else {
               std::cerr << " failed";
            }
        } else {
            // We would error out here, but we can't uncleanly shut down the child,
            // and there's nothing else we can do.
            std::cerr << "Not changing stat because type is unknown!";
        }
        std::cerr << std::endl;    
    }

    // Auto-commit after we receive everything
    if (!stats_api->StoreStats()) {
        std::cerr << "Committing changes failed" << std::endl;
    }

}