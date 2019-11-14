#include <yajl/yajl_gen.h>
#include <yajl/yajl_tree.h>
#include "MyGameSocket.h"
#include "../types/Actions.h"
#include "../json/ProcessedGameServerRequest.h"
#include "../json/yajlHelpers.h"
#include "../globals.h"
#include "../common/PerfMon.h"

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
            encode_achievements(handle, get_achievements());  // Write achievements to handle
            break;

        case STORE_ACHIEVEMENTS:
            process_changes( r.payload_to_ach_changes() );
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
        exit(EXIT_FAILURE);
    }

    yajl_gen_get_buf(handle, &buf, &len);
    ret = std::string((const char*)buf);
    yajl_gen_free(handle);

    return ret;
}

std::vector<Achievement_t>
MyGameSocket::get_achievements() {

    m_stats_callback_received = false;

    ISteamUserStats *stats_api = SteamUserStats();
    if (!stats_api->RequestCurrentStats()) {
        std::cerr << "ERROR: User not logged in, exiting" << std::endl;
        exit(EXIT_FAILURE);
    }

    while (!m_stats_callback_received) {
        SteamAPI_RunCallbacks();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    if (!get_global_stats())
    {
        std::cerr << "An error occurred getting global stats." << std::endl;
        return m_achievement_list;
    }

    if (!get_global_achievements_stats())
    {
        std::cerr << "An error occurred getting global achievements stats." << std::endl;
        return m_achievement_list;
    }

    return m_achievement_list;
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

    // Check if we received the values for the correct app
    if (SteamUtils()->GetAppID() == callback->m_nGameID) {
        if ( k_EResultOK == callback->m_eResult ) {
            ISteamUserStats *stats_api = SteamUserStats();

            // ==============================
            // RETRIEVE IDS
            // ==============================
            const unsigned num_ach = stats_api->GetNumAchievements();

            if (num_ach == 0) {
                std::cerr << "No achievements for current game" << std::endl;
            }

            m_achievement_list.clear();
            m_achievement_list.resize(num_ach);

            for (unsigned i = 0; i < num_ach ; i++) {

                m_achievement_list[i].id   = stats_api->GetAchievementName(i);

                const char * pchName = m_achievement_list[i].id.c_str();

                m_achievement_list[i].name = stats_api->GetAchievementDisplayAttribute(pchName, "name");
                m_achievement_list[i].desc = stats_api->GetAchievementDisplayAttribute(pchName, "desc");

                m_achievement_list[i].global_achieved_rate = 0;
                stats_api->GetAchievement(pchName, &(m_achievement_list[i].achieved));
                m_achievement_list[i].hidden = (bool)strcmp(stats_api->GetAchievementDisplayAttribute(pchName, "hidden" ), "0");
            }

        } else {
            std::cerr << "Received stats for the game, but an error occurrred." << std::endl;
        }
    } else {
        std::cerr << "Received stats for wrong game" << std::endl;
        return;
    }

    m_stats_callback_received = true;
}

void
MyGameSocket::OnGlobalStatsReceived(GlobalStatsReceived_t *callback, bool bIOFailure) {
    if ( bIOFailure || callback->m_eResult != k_EResultOK )
	{
		std::cerr << "GlobalStatsReceived_t failed! Enum: " << callback->m_eResult << std::endl;
		return;
	}

    std::cout << "Got stats, maybe I can do cool stuff with them, gotta check." << std::endl;
    m_global_callback_received = true;
}

void
MyGameSocket::OnGlobalAchievementPercentagesReceived(GlobalAchievementPercentagesReady_t *callback, bool bIOFailure) {

    if ( bIOFailure || callback->m_eResult != k_EResultOK )
	{
		std::cerr << "GlobalAchievementPercentagesReady_t failed! Enum: " << callback->m_eResult << std::endl;
		return;
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
MyGameSocket::process_changes(std::vector<AchievementChange_t> changes) {
    //Untested

    ISteamUserStats *stats_api = SteamUserStats();

    for (AchievementChange_t change : changes) {

        const char* achievement_id = change.id.c_str();

        if (change.achieved) {
            // We want to unlock an achievement
            if (!stats_api->SetAchievement(achievement_id)) {
                std::cerr << "Unlocking achievement " << achievement_id << " failed " << std::endl;
            } else {
                std::cerr << "Unlocked achievement " << achievement_id << std::endl;
            }
        } else {
            // We want to relock an achievement
            if (!stats_api->ClearAchievement(achievement_id)) {
                std::cerr << "Relocking achievement " << achievement_id << " failed" << std::endl;
            } else {
                std::cerr << "Relocked achievement " << achievement_id << std::endl;
            }
        }

        //TODO: stats

    }

    // Auto-commit after we receive everything
    if (!stats_api->StoreStats()) {
        std::cerr << "Committing changes failed" << std::endl;
    }

}