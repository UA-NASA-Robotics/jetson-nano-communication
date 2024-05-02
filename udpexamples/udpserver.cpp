#include <iostream>
#include <enet/enet.h>
#include <string.h>
ENetHost * client;
ENetHost * server;
ENetAddress address;
ENetEvent event;
ENetPeer * peer;

int main (int argc, char ** argv)
{
    //initializes ENet Library
        if (enet_initialize() != 0)
        {
            fprintf (stderr, "An error occurred while initializing ENet.\n");
            return EXIT_FAILURE;
        }
        atexit(enet_deinitialize);


    //Creates a ENet Server
        /* Bind the server to the default localhost.     */
        /* A specific host address can be specified by   */
        /* enet_address_set_host (& address, "x.x.x.x"); */
        
        address.host = ENET_HOST_ANY;
            /* Bind the server to port 1234. */
        address.port = 9002;
        
        server = enet_host_create(&address /* the address to bind the server host to */, 
                                    32      /* allow up to 32 clients and/or outgoing connections */,
                                    2      /* allow up to 2 channels to be used, 0 and 1 */,
                                    0      /* assume any amount of incoming bandwidth */,
                                    0      /* assume any amount of outgoing bandwidth */);
        if (server == NULL)
        {
            fprintf (stderr, "An error occurred while trying to create an ENet server host.\n");
            exit(EXIT_FAILURE);
        }
        enet_host_destroy(server);

        
}

