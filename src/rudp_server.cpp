#include "rudp_server.h"
#include <mesh_object_reader.h>
#include "draco/core/cycle_timer.h"

using namespace std;

rudp_server::rudp_server() {}

int rudp_server::Init(const char *ipAddress, int port, size_t desiredMemoryCache, int meshCompression, int quantPos)
{
    cl = meshCompression;
    qp = quantPos;

    encoder = std::make_shared<draco_encoder>(cl, qp);
    encoder->OnCompressDone = [&](draco::EncoderBuffer *data){
        Send(data->data(), data->size());
    };

    LimitPkgs = (int)(1.0f * desiredMemoryCache / ENET_HOST_DEFAULT_MTU);

    if (enet_initialize() != 0)
    {
        fprintf(stderr, "An error occurred while initializing ENet.\n");
        return EXIT_FAILURE;
    }
    atexit(enet_deinitialize);

    //address.host = ENET_HOST_ANY;
    enet_address_set_host(&address, ipAddress);

    address.port = port;
    server = enet_host_create(&address /* the address to bind the server host to */,
                              1 /* number of allowed clients and/or outgoing connections */,
                              2 /* number of channels */,
                              0 /* assume any amount of incoming bandwidth */,
                              0 /* assume any amount of outgoing bandwidth */);
    if (server == NULL)
    {
        fprintf(stderr,
                "An error occurred while trying to create an ENet server host.\n");
        exit(EXIT_FAILURE);
    }

    serverThread = thread([&] {
        ENetPeer *currentPeer;

        while (!terminated)
        {
            if (enet_host_service(server, &event, 10) > 0)
            {

                switch (event.type)
                {
                case ENET_EVENT_TYPE_CONNECT:
                    printf("A new client connected from %x:%u.\n",
                           event.peer->address.host,
                           event.peer->address.port);

                    client_connected = true;
                    /* Store any relevant client information here. */
                    event.peer->data = (void *)"Client information";

                    break;
                case ENET_EVENT_TYPE_RECEIVE:
                    enet_packet_destroy(event.packet);

                    break;

                case ENET_EVENT_TYPE_DISCONNECT:
                    printf("%s disconnected.\n", (char *)event.peer->data);
                    /* Reset the peer's client information. */
                    event.peer->data = NULL;
                    client_connected = false;
                }
            }


            // Check number of cached packages. Too many will overflow the memory.
            int pkgLeftToSend = 0;

            for (currentPeer = server->peers;
                 currentPeer < &server->peers[server->peerCount];
                 ++currentPeer)
            {
                if (currentPeer->state != ENET_PEER_STATE_CONNECTED)
                    continue;

                pkgLeftToSend += enet_list_size(&currentPeer->outgoingReliableCommands);
            }

            //printf("%i\n", pkgLeftToSend);

            canSend = pkgLeftToSend < LimitPkgs;
        }
    });
}

bool rudp_server::HasClients()
{
    return client_connected;
    //return server->peerCount > 0;
}

void rudp_server::Send(const char *buffer, size_t bufferLength)
{
    if (!canSend || server == NULL || server->peerCount == 0)
        return;

    ENetPacket *packet = enet_packet_create(buffer,
                                            bufferLength,
                                            ENET_PACKET_FLAG_UNSEQUENCED);

    enet_host_broadcast(server, 0, packet);
    canSend = false;
}

void rudp_server::Send(mesh_object *inMesh)
{
    encoder->Compress(inMesh);

    // encoder calles OnCompressDone after Compress is finished
}

void rudp_server::CleanUp()
{
    terminated = true;
    serverThread.join();
    enet_host_destroy(server);
}