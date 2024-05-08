#include "../src/server.hpp"

#include <iostream>
#include <thread>
#include <enet/enet.h>

#define PORT 8080

TCPServerHandler server(PORT);

ENetHost *client;
ENetAddress address;
ENetEvent event;
ENetPeer *peer;

void runENet()
{
    // initializes ENet Library
    if (enet_initialize() != 0)
    {
        fprintf(stderr, "An error occurred while initializing ENet.\n");
        return;
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

    // Connect to host address
    std::string host;
    std::cout << "Enter host address: ";
    std::cin >> host;
    enet_address_set_host(&address, "localhost");

    // Connect to port
    unsigned int port;
    std::cout << "Enter port: ";
    std::cin >> port;
    address.port = port;

    /* Initiate the connection, allocating the two channels 0 and 1. */
    peer = enet_host_connect(client, &address, 2, 0);
    if (peer == NULL)
    {
        std::cout << "No available peers for initiating an ENet connection." << std::endl;
        exit(EXIT_FAILURE);
    }

    while (enet_host_service(client, &event, 100) >= 0)
    {
        if (event.type == ENET_EVENT_TYPE_RECEIVE && event.packet != NULL)
        {
            ENetPacket *packet = event.packet;
            std::string message = std::string((char *)packet->data);

            std::cout << "Server -> Client: " << message << std::endl;

            // Send packet to server
            server.sendString(message);
        }
    }

    enet_host_destroy(client);
}

void onIncomingPacket(PacketData data, ServerHandler *server)
{
    std::cout << "Client -> Server: " << data.rawMessage << std::endl;

    // Send packet to server
    ENetPacket *packet = enet_packet_create(data.rawMessage.c_str(), data.rawMessage.size(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(client->peers, 0, packet);
    enet_host_flush(client);
}

int main()
{
    std::thread enetThread(runENet);

    server.setPacketCallback(onIncomingPacket);

    std::cout << "Server running on port " << PORT << "..." << std::endl;

    server.run();

    return 0;
}