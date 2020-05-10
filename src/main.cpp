/*************************************
 * Please read the license before modifying or distributing any of the code from
 * this project. Thank you.
 *************************************/

#include "schema_parser/UserGameStatsSchemaParser.h"
#include "controller/MySteam.h"
#include "controller/MySteamClient.h"
#include "controller/MyGameServer.h"
#include "gui/MainPickerWindowFactory.h"
#include "cli/cli_funcs.h"
#include "common/PerfMon.h"
#include "globals.h"

#include <iostream>
#include <sys/stat.h>
#include <gtkmm-3.0/gtkmm/application.h>

/**************************************
 *  Declare global variables imported from globals.h
 **************************************/
MySteam* g_steam = nullptr;
MainPickerWindow* g_main_gui = nullptr;
MySteamClient* g_steamclient = nullptr;
PerfMon* g_perfmon = nullptr;

/**************************************
 * Main entry point
 **************************************/
int
main(int argc, char *argv[])
{
    if (getuid() == 0) {
        std::cout << "Do not run this application as root" << std::endl;
        zenity("Please do not run this application as root..");
        exit(EXIT_FAILURE);
    }

    g_perfmon = new PerfMon();
    g_steam = MySteam::get_instance();
    g_steamclient = new MySteamClient();
    g_perfmon->log("Globals initialized.");

    // Check for command-line options, which may prevent showing the GUI
    // Note that a rewriting should be done to further separate the GUI
    // from a command-line interface
    if(!go_cli_mode(argc, argv)) {
        auto app = Gtk::Application::create(argc, argv);
        g_main_gui = MainPickerWindowFactory::create();

        app->run(*g_main_gui);
    }

    return EXIT_SUCCESS;
}
