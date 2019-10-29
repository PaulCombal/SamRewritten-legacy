#pragma once
#include "MySocket.h"

class MyServerSocket : public MySocket
{
    /**
     * The server socket constructor is responsible for creating
     * the socket file both server and client will communicate through.
     */
public:
    void run_server();
    virtual std::string process_request(std::string request, bool& quit) = 0;
    MyServerSocket(AppId_t appid);
};