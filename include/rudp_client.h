#include <enet/enet.h>
#include <functional>
#include <thread>

using RecvCallFn = std::function<void(const char* data, size_t dataLength)>;

class rudp_client
{
public:
    rudp_client();
    
    int Connect(const char* ipAddress, int port = 55550);
    RecvCallFn OnReceive = NULL;
    void CleanUp();

private:
    ENetHost *client;
    ENetPeer *peer;
    ENetAddress address;
    ENetEvent event;

    std::thread clientThread;

    bool terminated = false;
};