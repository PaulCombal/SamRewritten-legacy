#include "cli_funcs.h"
#include "../controller/MySteam.h"
#include "../globals.h"
#include "../common/cxxopts.hpp"
#include "../common/functions.h"

#include <string>
#include <iostream>
#include <csignal>
#include <thread>
#include <unistd.h>

void handle_sigint_cli(int signum) 
{
    std::cout << "Quitting cli idling" << std::endl;
    g_steam->quit_game();
    exit(EXIT_SUCCESS);
}

void idle_app(AppId_t appid) 
{
    std::cout << "Idling from command line " << appid << std::endl;
    g_steam->launch_app(appid);
    signal(SIGINT, handle_sigint_cli);

    // Wait for ctrl+c, so we can kill both processes, otherwise
    // GUI process will exit while game process still goes on
    for(;;) {
        sleep(10000);
    }
}

bool compareByUnlockRateDesc(const Achievement_t &a, const Achievement_t &b)
{
    return a.global_achieved_rate > b.global_achieved_rate;
}

bool go_cli_mode(int argc, char* argv[], AppId_t *return_app_id) {
    cxxopts::Options options(argv[0], "Steam Achievements Manager");
    
    options
      .positional_help("[AppId]")
      .show_positional_help()
      .add_options()
        ("apps", "Get the list of your owned apps.")
        ("h,help", "Show CLI help.")
        ("a,app", "Set which AppId you want to use. Same as using positional 'AppId'", cxxopts::value<AppId_t>())
        ("i,idle", "Set your Steam profile as 'ingame'. Ctrl+c to stop.")
        ("ls", "Display achievements and stats for selected app.")
        ("sort", "Sort option for --ls. You can leave empty or set to 'unlock_rate'", cxxopts::value<std::string>())
        ("unlock", "Unlock achievements for an AppId. Separate achievement names by a comma.", cxxopts::value<std::vector<std::string>>())
        ("lock", "Lock achievements for an AppId. Separate achievement names by a comma.", cxxopts::value<std::vector<std::string>>())
        ("timed", "Do a timed achievement modification. Arguments that affect this are --amount, --units, --spacing, and --order")
        ("amount", "Control the amount of time spent for --timed modifications. Specify units with --units. Default is 1000", cxxopts::value<uint64_t>())
        ("units", "Control the units of time spent for --timed modifications. Set to 'seconds', 'minutes', 'hours', or 'days'. Default is seconds", cxxopts::value<std::string>())
        ("spacing", "Control the spacing between appying each modification for --timed modifications. Set to 'even', 'evenish' or 'random'. Default is even", cxxopts::value<std::string>())
        ("order", "Control the order --timed achievement modifications are applied in. Set to 'selection' or 'random'. Default is selection", cxxopts::value<std::string>())
        ("statnames", "Change stats for an AppId. Separate stat names by a comma. Use with statvalues to name the values in order", cxxopts::value<std::vector<std::string>>())
        ("statvalues", "Change stats for an AppId. Separate stat values by a comma. Use with statnames to name the values in order", cxxopts::value<std::vector<std::string>>())
        ("p,launch_achievements", "Launch SamRewritten GUI and immediately switch to achievements page for the app.") // This is used by the GUI for launching in a new window
        ("launch", "Actually just launch the app.");

    options.parse_positional({"app"});

    auto result = options.parse(argc, argv);
    AppId_t app = 0;
    *return_app_id = 0;

    if (result.count("help") > 0)
    {
        std::cout << options.help() << std::endl;
        return true;
    }
    
    if (result.count("app") > 0)
    {
        app = result["app"].as<AppId_t>();
    }
    
    if (result.count("launch_achievements") > 0)
    {
        if (app == 0)
        {
            std::cout << "Please provide an AppId argument before launching achievements page." << std::endl;
            return true;
        }

        *return_app_id = app;
        return false;
    }

    if (result.count("apps") > 0)
    {
        g_steam->refresh_owned_apps();
        auto apps = g_steam->get_subscribed_apps();

        std::cout << "AppId \t App Name" << std::endl;
        for ( Game_t& it : apps )
        {
            std::cout << it.app_id << "\t " << it.app_name << std::endl;
        }
        return true;
    }

    if (result.count("idle") > 0)
    {
        if (app == 0)
        {
            std::cout << "Please provide an AppId argument before idling." << std::endl;
            return true;
        }
        
        idle_app(app);
        return true;
    }

    if (result.count("ls") > 0) {
        if (app == 0)
        {
            std::cout << "Please provide an AppId argument before listing stats." << std::endl;
            return true;
        }

        g_steam->launch_app(app);
        g_steam->refresh_achievements_and_stats();
        auto achievements = g_steam->get_achievements();
        auto stats = g_steam->get_stats();
        g_steam->quit_game();
        
        if (result.count("sort") > 0)
        {
            const std::string sort = result["sort"].as<std::string>();

            if (sort == "unlock_rate")
            {
                std::sort(achievements.begin(), achievements.end(), compareByUnlockRateDesc);
                std::cout << "Sorted by global unlock rate\n" << std::endl;
            }
        }


        // https://github.com/haarcuba/cpp-text-table -> worth? nah but best I've found
        std::cout << "API Name \t\tName \t\tDescription \t\tUnlock rate \t\tUnlocked \t\tProtected\n";
        std::cout << "--------------------------------------------------------------" << std::endl;
        for ( Achievement_t& achievement : achievements )
        {
            std::cout 
                << achievement.id << " \t" 
                << achievement.name << " \t" 
                << achievement.desc << " \t" 
                << achievement.global_achieved_rate << "% \t"
                << (achievement.achieved ? "✔️" : "❌") << " \t"
                << (is_permission_protected(achievement.permission) ? "Yes" : "No") << std::endl;
        }

        std::cout << std::endl;

        if ( stats.size() == 0 )
        {
            std::cout << "No stats found for this app.." << std::endl;
        }
        else
        {
            std::cout << "\nSTATS\n";
            std::cout << "API Name \t\tType \t\t Value \t\tIncrement Only \t\tProtected\n";
            std::cout << "----------------------------------------" << std::endl;
            for (auto stat : stats )
            {
                std::cout << stat.id << " \t";

                if (stat.type == UserStatType::Integer) {
                    std::cout << "Integer \t" << std::to_string(std::any_cast<long long>(stat.value)) << " \t";
                } else if (stat.type == UserStatType::Float) {
                    std::cout << "Float \t" << std::to_string(std::any_cast<double>(stat.value)) << " \t";
                } else {
                    std::cout << "Unknown \tUnknown \t";
                }

                std::cout << (stat.incrementonly ? "Yes" : "No") << " \t"
                << (is_permission_protected(stat.permission) ? "Yes" : "No") << std::endl;
            }
        }
        return true;
    }

    if (result.count("unlock") > 0)
    {
        if (app == 0)
        {
            std::cout << "Please provide an AppId argument before unlocking achievements." << std::endl;
            return true;
        }

        const std::vector<std::string> ids = result["unlock"].as<std::vector<std::string>>();

        for ( std::string it : ids )
        {
            g_steam->add_modification_ach(it, true);
        }
        // Continue on to commit the modifications
    }

    if (result.count("lock") > 0)
    {
        if (app == 0)
        {
            std::cout << "Please provide an AppId argument before relocking achievements." << std::endl;
            return true;
        }

        const std::vector<std::string> ids = result["lock"].as<std::vector<std::string>>();

        for ( std::string it : ids )
        {
            g_steam->add_modification_ach(it, false);
        }
        // Continue on to commit the modifications
    }

    if (result.count("lock") > 0 || result.count("unlock") > 0) {
        if (result.count("timed") > 0) {
            // Hook this up since we'll probably be in this function for a while
            // Really we could hook this up whenever we launch the app...
            signal(SIGINT, handle_sigint_cli);

            if (app == 0)
            {
                std::cout << "Please provide an AppId argument before unlocking achievements." << std::endl;
                return true;
            }

            uint64_t time = 1000; 
            MODIFICATION_SPACING spacing = EVEN_SPACING;
            MODIFICATION_ORDER order = SELECTION_ORDER;

            if (result.count("amount") > 0) {
                time = result["amount"].as<uint64_t>();
            }

            if (result.count("units") > 0) {
                std::string units = result["units"].as<std::string>();

                if (units == "seconds") {
                    // Do nothing
                } else if (units == "minutes") {
                    time *= 60;
                } else if (units == "hours") {
                    time *= 60 * 60;
                } else if (units == "days") {
                    time *= 60 * 60 * 24;
                } else {
                    std::cerr << "invalid time units:" << units << std::endl;
                    return true;
                }
            }

            if (result.count("spacing") > 0) {
                std::string spacing_input = result["spacing"].as<std::string>();

                if (spacing_input == "even") {
                    spacing = EVEN_SPACING;
                } else if (spacing_input == "random") {
                    spacing = RANDOM_SPACING;
                } else if (spacing_input == "evenish") {
                    spacing = EVEN_FREE_SPACING;
                } else {
                    std::cerr << "invalid spacing: " << spacing_input << std::endl;
                    return true;
                }
            }

            if (result.count("order") > 0) {
                std::string order_input = result["order"].as<std::string>();

                if (order_input == "selection") {
                    order = SELECTION_ORDER;
                } else if (order_input == "random") {
                    order = RANDOM_ORDER;
                } else {
                    std::cerr << "invalid order: " << order_input << std::endl;
                    return true;
                }
            }

            g_steam->launch_app(app);
            std::vector<uint64_t> times = g_steam->setup_timed_modifications(time, spacing, order);

            while (!times.empty()) {
                std::cout << "Modifying next achievement in " << times[0] << " seconds"
                          << " (or " << (((double)times[0]) / 60) << " minutes or "
                          << ((((double)times[0]) / 60) / 60) << " hours)" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(times[0]));
                g_steam->commit_next_timed_modification();
                times.erase(times.begin());
            }
            g_steam->quit_game();
            return true;
        } else {
            g_steam->launch_app(app);
            g_steam->commit_changes();
            g_steam->quit_game();
            return true;
        }
    }

    if (result.count("statnames") > 0 || result.count("statvalues") > 0)
    {
        if (app == 0)
        {
            std::cout << "Please provide an AppId argument before changing stats." << std::endl;
            return true;
        }
        if (result.count("statnames") != result.count("statvalues"))
        {
            std::cout << "Number of statnames does not equal number of statvalues." << std::endl;
            return true;
        }

        size_t num_stats = result.count("statnames");

        const std::vector<std::string> stat_names = result["statnames"].as<std::vector<std::string>>();
        const std::vector<std::string> stat_values = result["statvalues"].as<std::vector<std::string>>();

        std::cout << "Warning: There will be an failure for each modificaiton, but the modification will go through "
                     "if there is at least one success for each stat." << std::endl;

        // Launch the app now for below hack
        g_steam->launch_app(app);

        for (size_t i = 0; i < num_stats; i++) {
            StatValue_t stat;
            bool valid_conversion;
            std::any new_value;
             // No validation done for the name
            stat.id = stat_names[i];

            // Hack: We don't know the type at this point, so try both of them
            valid_conversion = convert_user_stat_value(UserStatType::Float, stat_values[i], &new_value);

            if (valid_conversion) {
                stat.type = UserStatType::Float;
                g_steam->add_modification_stat(stat, new_value);
                g_steam->commit_changes();
            }

            valid_conversion = convert_user_stat_value(UserStatType::Integer, stat_values[i], &new_value);

            if (valid_conversion) {
                stat.type = UserStatType::Integer;
                g_steam->add_modification_stat(stat, new_value);
                g_steam->commit_changes();
            }
        }

        g_steam->quit_game();
        return true;
    }

    if (result.count("launch") > 0)
    {
        if (app == 0)
        {
            std::cout << "Please provide an AppId argument before launching the app." << std::endl;
            return true;
        }

        std::cout << "Launching app with Steam. Make sure you have xdg-open installed." << std::endl;
        system(("xdg-open steam://run/" + std::to_string(app)).c_str());
        return true;
    }

    return false;
}
