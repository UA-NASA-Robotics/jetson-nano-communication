#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>

#include <boost/asio.hpp>

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <vector>

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

// pull out the type of messages sent by our config
typedef server::message_ptr message_ptr;

// // Commands:
// // help
// // list-connections  done
// //      ex: 0: ws://10.10.100.10:9002
// // message-history done
// // send-string <message> done
// // disconnect <index>   done
// // quit done

class ServerHandler
{
public:
    ServerHandler() : next_id(0)
    {
        // Set logging settings
        serverEndpoint.set_access_channels(websocketpp::log::alevel::all);
        serverEndpoint.clear_access_channels(websocketpp::log::alevel::frame_payload);

        // Initialize the server & start the perpetual IO service
        serverEndpoint.init_asio();
        serverEndpoint.start_perpetual();

        // Set the handlers
        serverEndpoint.set_open_handler(websocketpp::lib::bind(&ServerHandler::onOpen, this, &serverEndpoint, ::_1));
        serverEndpoint.set_message_handler(websocketpp::lib::bind(&ServerHandler::onMessage, this, &serverEndpoint, ::_1, ::_2));
        serverEndpoint.set_close_handler(websocketpp::lib::bind(&ServerHandler::onClose, this, &serverEndpoint, ::_1));

        // Start listening for new connections on port 9002
        serverEndpoint.listen(9002);
        serverEndpoint.start_accept();
        thread.reset(new websocketpp::lib::thread(&server::run, &serverEndpoint));
    }

    ~ServerHandler() {
        // Stop listening for new connections
        serverEndpoint.stop_listening();

        // Stop the perpetual IO service
        serverEndpoint.stop_perpetual();

        // Stop the server's event loop
        serverEndpoint.stop();

        // Join the server thread to wait for it to finish
        if (thread && thread->joinable()) {
            thread->join();
        }
    }

    // When a new connection is opened
    void onOpen(server *s, websocketpp::connection_hdl hdl)
    {
        // add the new connection to the list of connections
        connections.push_back(hdl);
    }

    // When a message is recieved
    void onMessage(server *s, websocketpp::connection_hdl hdl, message_ptr msg)
    {

        std::cout << msg->get_payload() << std::endl;

        // Add the recieved message to the message history
        messageHistory.push_back("recieved: " + msg->get_payload());

        // std::cout << "on_message called with hdl: " << hdl.lock().get()
        //           << " and message: " << msg->get_payload()
        //           << std::endl;

        // if (msg->get_payload() == "stop-listening")
        // {
        //     s->stop_listening();
        //     return;
        // }

        // try
        // {
        //     s->send(hdl, msg->get_payload(), msg->get_opcode());
        // }
        // catch (websocketpp::exception const &e)
        // {
        //     std::cout << "Echo failed because: "
        //             << "(" << e.what() << ")" << std::endl;
        // }
    }

    // When a connection is closed
    void onClose(server *s, websocketpp::connection_hdl hdl)
    {
        // remove the connection from the list of connections
        auto it = std::find_if(connections.begin(), connections.end(), [&](const websocketpp::connection_hdl &item)
                               { return hdl.lock() == item.lock(); });
        if (it != connections.end())
        {
            connections.erase(it);
        }
    }

    /**
     * Display the help text
    */
    void help() {
        std::cout
            << "\nCommand List:\n"
            << "help: Display this help text\n"
            << "send <message>: Send a message to all connected clients\n"
            << "list-connections: List all the connections\n"
            << "message-history: List the message history\n"
            << "disconnect <index>: Disconnect a client\n"
            << "quit: Exit the program\n"
            << std::endl;
    }
    
    /**
     * Send a message to all connected clients
     * 
     * @param[in] msg The message to send
    */
    void send(std::string msg)
    {
        websocketpp::lib::error_code ec;

        for (auto hdl : connections)
        {
            serverEndpoint.send(hdl, msg, websocketpp::frame::opcode::value::text, ec);

            if (ec)
            {
                std::cout << "> Send error: " << ec.message() << std::endl;
                continue;
            }
        }

        // Add the sent message to the message history
        messageHistory.push_back("sent: " + msg);
    }

    /**
     * List all the connections
    */
    void listConnections() {
        for (int i = 0; i < connections.size(); i++) {
            auto connection = serverEndpoint.get_con_from_hdl(connections[i]);

            // Get the ip and port of the connection from the socket with boost::asio
            auto& socket = connection->get_raw_socket();
            std::string ip = socket.lowest_layer().remote_endpoint().address().to_string();
            std::string port = std::to_string(socket.lowest_layer().remote_endpoint().port());

            std::cout << i << ": " << ip << " Port: " << port << std::endl;
        }
    }

    /**
     * List the message history
    */
    void listMessageHistory() {
        for (int i = 0; i < messageHistory.size(); i++) {
            std::cout << messageHistory[i] << std::endl;
        }
    }

    /**
     * Disconnect a client
     * 
     * @param[in] index The index of the connection to disconnect
    */
    void disconnect(int index) {
        if (index < 0 || index >= connections.size()) {
            std::cout << "Invalid index" << std::endl;
            return;
        }

        auto connection = serverEndpoint.get_con_from_hdl(connections[index]);
        serverEndpoint.close(connection->get_handle(), websocketpp::close::status::normal, "Disconnecting");
    }

    /**
     * Get the local ip address
     * 
     * @note This is a linux specific command
    */
    void getLocalIp()
    {
        std::cout << "IP: \n";
        system("hostname -I | awk '{print$1}'");
    }

private:
    server serverEndpoint;
    websocketpp::lib::shared_ptr<websocketpp::lib::thread> thread;

    std::vector<websocketpp::connection_hdl> connections; // List of connections
    /**
     * Message history
     * smaller the index, the older the message
     * format: ["recieved: <message>", "sent: <message>"]
     * */ 
    std::vector<std::string> messageHistory;
    int next_id; 
};

int main()
{
    bool done = false;
    std::string input;
    ServerHandler endpoint;

    endpoint.getLocalIp();

    while (!done)
    {
        std::cout << "Enter Command: ";
        std::getline(std::cin, input);

        if (input == "quit")
        {
            done = true;
        }
        else if (input == "help")
        {
            endpoint.help();
        }
        else if (input.substr(0, 4) == "send")
        {
            std::string msg = input.substr(5);
            endpoint.send(msg);
        }
        else if (input == "list-connections")
        {
            endpoint.listConnections();
        }
        else if (input == "message-history")
        {
            endpoint.listMessageHistory();
        }
        else if (input.substr(0, 10) == "disconnect")
        {
            std::string index = input.substr(11);
            endpoint.disconnect(std::stoi(index));
        }
        else
        {
            std::cout << "> Unrecognized Command" << std::endl;
        }
    }

    return 0;
}
