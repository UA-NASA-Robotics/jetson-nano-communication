#include <iostream>
#include <enet/enet.h>
#include <string.h>

ENetHost *client;
ENetAddress address;
ENetEvent event;
ENetPeer *peer;

int main(int argc, char **argv)
{
    // initializes ENet Library
    if (enet_initialize() != 0)
    {
        fprintf(stderr, "An error occurred while initializing ENet.\n");
        return EXIT_FAILURE;
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
        exit(EXIT_FAILURE);
    }

    /* Connect to some.server.net:1234. */
    enet_address_set_host(&address, "localhost");
    address.port = 9002;
    /* Initiate the connection, allocating the two channels 0 and 1. */
    peer = enet_host_connect(client, &address, 2, 0);
    if (peer == NULL)
    {
        fprintf(stderr,
                "No available peers for initiating an ENet connection.\n");
        exit(EXIT_FAILURE);
    }

    /* Wait up to 1000 milliseconds for an event. */
    while (enet_host_service(client, &event, 1000) >= 0)
    {
        /* Create a reliable packet of size 7 containing "packet\0" */
        ENetPacket *packet = enet_packet_create("Pz",
                                                2,
                                                ENET_PACKET_FLAG_RELIABLE);

        /* Send the packet to the peer over channel id 0. */
        /* One could also broadcast the packet by         */
        /* enet_host_broadcast (host, 0, packet);         */
        enet_peer_send(client->peers, 0, packet);

        /* One could just use enet_host_service() instead. */
        enet_host_flush(client);
        switch (event.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
            std::cout << "A new client connected from " << event.peer->address.host << ":" << event.peer->address.port << ".\n";

            /* Store any relevant client information here. */
            // event.peer->data = "Client information";

            break;

        case ENET_EVENT_TYPE_RECEIVE:
            std::cout << "A packet of length " << event.packet->dataLength << " containing " << event.packet->data << " was received from " << event.peer->data << " on channel " << event.channelID << ".\n";

            /* Clean up the packet now that we're done using it. */
            enet_packet_destroy(event.packet);

            break;

        case ENET_EVENT_TYPE_DISCONNECT:
            std::cout << event.peer->data << " disconnected.\n";

            /* Reset the peer's client information. */

            event.peer->data = NULL;
        }
    }

    enet_host_destroy(client);
}
