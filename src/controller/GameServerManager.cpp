#include "GameServerManager.h"
#include "MyGameServer.h"
#include "MySteamClient.h"
#include "MySteam.h"
#include "../globals.h"
#include "../common/functions.h"
#include <signal.h>
#include <iostream>
#include <unistd.h>

void handle_sigint_gameserv(int signum) {
    g_steam->quit_game();
    exit(0);
}

MyClientSocket*
GameServerManager::quick_server_create(AppId_t appid)
{
    pid_t pid;
    if ((pid = fork()) == 0) {
        // Son's process: server role
        
        // We need to delete this to unload steamclient.so from the
        // child's process map, otherwise we pick up on a steam client
        // state from the app detection stage that doesn't have an appid
        // associated with it. But we cannot close the pipes because doing
        // so closes the parent's pipe too...
        g_steamclient->unloadLibrary();

        MyGameServer server(appid);
        server.run();

        // The server will stop running when the client sends a quit request.
        exit(EXIT_SUCCESS);
    }
    else if (pid == -1) {
        std::cerr << "Could not fork in GameServerManager." << std::endl;
        zenity("Could not fork in GameServerManager.");
        exit(EXIT_FAILURE);
    }
    else {
        // Parent process
        // TODO: don't use signal; use sigaction
        signal(SIGCHLD, SIG_IGN);
        signal(SIGINT, handle_sigint_gameserv);
        return new MyClientSocket(appid);
    }
}