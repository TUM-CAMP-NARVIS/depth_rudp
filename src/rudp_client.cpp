#include "rudp_client.h"

using namespace std;

rudp_client::rudp_client() {}

int rudp_client::Connect(const char *ipAddress, int port)
{
    if (enet_initialize() != 0)
    {
        fprintf(stderr, "An error occurred while initializing ENet.\n");
        return 0;
    }
    atexit(enet_deinitialize);

    client = enet_host_create(NULL /* create a client host */,
                              1 /* only allow 1 outgoing connection */,
                              2 /* allow up 2 channels to be used, 0 and 1 */,
                              0 /* assume any amount of incoming bandwidth */,
                              0 /* assume any amount of outgoing bandwidth */);
    if (client == NULL)
    {
        fprintf(stderr,
                "An error occurred while trying to create an ENet client host.\n");
        return 0;
    }
    
    /* Connect to some.server.net:1234. */
    enet_address_set_host(&address, ipAddress);
    address.port = port;
    /* Initiate the connection, allocating the two channels 0 and 1. */
    peer = enet_host_connect(client, &address, 2, 0);
    if (peer == NULL)
    {
        fprintf(stderr,
                "No available peers for initiating an ENet connection.\n");
        return 0;
    }
    /* Wait up to 5 seconds for the connection attempt to succeed. */
    if (enet_host_service(client, &event, 5000) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT)
    {
        puts("Connection succeeded.");
    }
    else
    {
        /* Either the 5 seconds are up or a disconnect event was */
        /* received. Reset the peer in the event the 5 seconds   */
        /* had run out without any significant event.            */
        enet_peer_reset(peer);
        puts("Connection failed.");
        return 0;
    }


    clientThread = thread([&] {

        while (!terminated)
        {

            if (enet_host_service(client, &event, 3000) > 0)
            {
                switch (event.type)
                {
                case ENET_EVENT_TYPE_CONNECT:
                    printf("A new client connected from %x:%u.\n",
                           event.peer->address.host,
                           event.peer->address.port);
                    /* Store any relevant client information here. */
                    event.peer->data = (void *)"Client information";
                    break;
                case ENET_EVENT_TYPE_RECEIVE:

                    if (OnReceive)
                        OnReceive((const char *)event.packet->data, event.packet->dataLength);

                    /* Clean up the packet now that we're done using it. */
                    enet_packet_destroy(event.packet);

                    break;

                case ENET_EVENT_TYPE_DISCONNECT:
                    printf("Client disconnected.\n");
                }
            }
        }
    });
    return 1;
}

void rudp_client::CleanUp()
{
    enet_peer_disconnect(peer, 0);
    terminated = true;
    clientThread.join();
    enet_host_destroy(client);
}