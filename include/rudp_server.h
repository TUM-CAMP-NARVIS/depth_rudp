#include <enet/enet.h>
#include <vector>
#include <thread>

#include "queued_task.h"

class rudp_server
{
public:
    rudp_server();

    int Init(const char* ipAddress, int port = 55550, size_t desiredMemoryCache = 1500000, int meshCompression = 1, int quantPos = 14);
    bool HasClients();
    void Send(const char* buffer, size_t bufferLength);
    
    void CleanUp();

private:
    int cl = 1;
    int qp = 14;

    int LimitPkgs;

    ENetHost *server{NULL};
    ENetAddress address;
    ENetEvent event;
    std::thread serverThread;

    bool client_connected{false};

    bool canSend{false};

    bool terminated{false};
};