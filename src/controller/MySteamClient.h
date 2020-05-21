#pragma once
#include "../../steam/steam_api.h"
#include "../common/functions.h"
#include "../globals.h"
#include "MySteam.h"
#include <iostream>
#include <dlfcn.h>

#define RELATIVE_STEAM_CLIENT_LIB_PATH "/linux64/steamclient.so"

/**
 * Purpose:
 * The SteamClient() accessor does not work. It always returns NULL for some reason.
 * We use the Steam Client library to call the CreateInterface to create an instance anyway.
 *
 * This is used so we can communicate with the Steam Client without being marked as "In-Game"
 *
 * I did not experiment a lot with this class, but I would advise to create only one instance for the whole program.
 *
 * I get SteamApps to get owned games (if one AppId is subscribed)
 * I could use ISteamUser to get the current user's Steam ID. (what for idk, but later features maybe)
 *
 * https://partner.steamgames.com/doc/api/ISteamClient
 */
class MySteamClient
{
public:
    HSteamPipe getPipe() const { return m_pipe; };
    HSteamUser getUser() const { return m_user; };
    ISteamApps* getSteamApps() const { return m_steamapps; };
    ISteamUser* getSteamUser() const { return m_steamuser; };

    // Needed for child usage..
    void unloadLibrary() {
        dlclose(m_handle);
    }

    ~MySteamClient() {
        m_steamclient->ReleaseUser(m_pipe, m_user);
        m_steamclient->BReleaseSteamPipe(m_pipe);
        m_steamclient->BShutdownIfAllPipesClosed();
        dlclose(m_handle);
    }

    MySteamClient() {
        char* error;
        std::string steam_client_lib_path = g_steam->get_steam_install_path() + RELATIVE_STEAM_CLIENT_LIB_PATH;
        m_handle = dlopen(steam_client_lib_path.c_str(), RTLD_LAZY);
        if (!m_handle) {
            std::cerr << "Error opening the Steam Client library. ~/.local/share/Steam may not exist. Info:" << std::endl;
            std::cerr << dlerror() << std::endl;
            std::cerr << "Trying alternate Steam Client library path." << std::endl;

            steam_client_lib_path = g_steam->get_steam_install_path() + "/.." + RELATIVE_STEAM_CLIENT_LIB_PATH;
            m_handle = dlopen(steam_client_lib_path.c_str(), RTLD_LAZY);
            if (!m_handle) {
              std::cerr << "Error opening the Steam Client library. Exiting. Info:" << std::endl;
              std::cerr << dlerror() << std::endl;
              zenity();
              exit(EXIT_FAILURE);
           }
        }

        ISteamClient* (*CreateInterface)(const char* version, void*);

        CreateInterface = (ISteamClient* (*)(const char*, void*))dlsym(m_handle, "CreateInterface");
        if ((error = dlerror()) != NULL)  {
            std::cerr << "Error reading the CreateInterface symbol from the Steam Client library. Exiting. Info:" << std::endl;
            std::cerr << error << std::endl;
            zenity();
            exit(EXIT_FAILURE);
        }

        m_steamclient = CreateInterface(STEAMCLIENT_INTERFACE_VERSION, NULL);
        m_pipe = m_steamclient->CreateSteamPipe();
        m_user = m_steamclient->ConnectToGlobalUser(m_pipe);

        if (m_pipe == 0 || m_user == 0) {
            std::cout << "Unable to interact with Steam. Make sure Steam is launched and you are logged in to your account." << std::endl;
            std::cerr << "Unable to create Steam Pipe or connect to global user. Exitting." << std::endl;
            zenity("Unable to interact with Steam. Make sure Steam is launched and you are logged in to your account.");
            exit(EXIT_FAILURE);
        }

        m_steamapps = m_steamclient->GetISteamApps(m_user, m_pipe, STEAMAPPS_INTERFACE_VERSION);
        if (m_steamapps == NULL) {
            std::cerr << "Unable to get ISteamApps interface for MySteamClient. Exitting." << std::endl;
            zenity();
            exit(EXIT_FAILURE);
        }

        m_steamuser = m_steamclient->GetISteamUser(m_user, m_pipe, STEAMUSER_INTERFACE_VERSION);
        if (m_steamuser == NULL) {
            std::cerr << "Unable to get ISteamUser interface for MySteamClient. Exitting." << std::endl;
            zenity();
            exit(EXIT_FAILURE);
        }
    };
private:
    HSteamPipe m_pipe;
    HSteamUser m_user;
    void* m_handle = nullptr;
    ISteamApps* m_steamapps = nullptr;
    ISteamUser* m_steamuser = nullptr;
    ISteamClient* m_steamclient = nullptr;
};
