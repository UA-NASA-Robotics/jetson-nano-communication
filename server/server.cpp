#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>

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

class websocket_endpoint
{
public:
    websocket_endpoint() : m_next_id(0)
    {
        serverEndpoint.clear_access_channels(websocketpp::log::alevel::all);
        serverEndpoint.clear_error_channels(websocketpp::log::elevel::all);

        serverEndpoint.init_asio();
        serverEndpoint.start_perpetual();

        serverEndpoint.set_message_handler(websocketpp::lib::bind(&websocket_endpoint::onMessage, this, &serverEndpoint, ::_1, ::_2));

        serverEndpoint.listen(9002);

        // Start the server accept loop
        serverEndpoint.start_accept();

        m_thread.reset(new websocketpp::lib::thread(&server::run, &serverEndpoint));
    }

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
    }

    void onOpen(server *s, websocketpp::connection_hdl hdl)
    {
        connections.push_back(hdl);
        std::cout << getIp() << std::endl;
    }

    void onClose(server *s, websocketpp::connection_hdl hdl)
    {
        auto it = std::find_if(connections.begin(), connections.end(), [&](const websocketpp::connection_hdl &item)
                               { return hdl.lock() == item.lock(); });

        if (it != connections.end())
        {
            connections.erase(it);
        }
    }

    void onMessage(server *s, websocketpp::connection_hdl hdl, message_ptr msg)
    {
        std::cout << msg->get_payload() << std::endl;

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

    std::string getIp()
    {
        boost::system::error_code ec;
        std::string ip = serverEndpoint.get_local_endpoint(ec).address().to_string();
        return ip;
    }

private:
    server serverEndpoint;
    websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_thread;

    std::vector<websocketpp::connection_hdl> connections;
    int m_next_id;
};

int main()
{
    bool done = false;
    std::string input;
    websocket_endpoint endpoint;

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
            std::cout
                << "\nCommand List:\n"
                << "send <message>"
                << "help: Display this help text\n"
                << "quit: Exit the program\n"
                << std::endl;
        }
        else if (input.substr(0, 4) == "send")
        {
            std::string msg = input.substr(5);
            endpoint.send(msg);
        }
        else
        {
            std::cout << "> Unrecognized Command" << std::endl;
        }
    }

    return 0;
}

// #include <websocketpp/config/asio_no_tls.hpp>
// #include <websocketpp/server.hpp>

// #include <iostream>
// #include <vector>

// typedef websocketpp::server<websocketpp::config::asio> server;

// using websocketpp::lib::placeholders::_1;
// using websocketpp::lib::placeholders::_2;

// // pull out the type of messages sent by our config
// typedef server::message_ptr message_ptr;

// class ServerHandler {
// private:
//     server echo_server;
//     std::vector<websocketpp::connection_hdl> connections;
//     websocketpp::lib::shared_ptr<websocketpp::lib::thread> thread;

// public:
//     ServerHandler() {
//         // Set logging settings
//         echo_server.set_access_channels(websocketpp::log::alevel::all);
//         echo_server.clear_access_channels(websocketpp::log::alevel::frame_payload);

//         // Define handler to track connections
//         echo_server.set_open_handler(websocketpp::lib::bind(&ServerHandler::onOpen, this, &echo_server, ::_1));

//         // Define handler to remove closed connections from the vector
//         echo_server.set_close_handler(websocketpp::lib::bind(&ServerHandler::onClose, this, &echo_server, ::_1));

//         // Initialize Asio
//         echo_server.init_asio();

//         // Register our message handler
//         echo_server.set_message_handler(websocketpp::lib::bind(&ServerHandler::onMessage, this, &echo_server, ::_1, ::_2));

//         // Listen on port 9002
//         echo_server.listen(9002);

//         // Start the server accept loop
//         echo_server.start_accept();

//         echo_server.start_perpetual();

//         // thread.reset(new websocketpp::lib::thread(&server::run, &echo_server));
//         echo_server.run();
//     }

//     ~ServerHandler() {
//         echo_server.stop_listening();
//     }

//     void onOpen(server *s, websocketpp::connection_hdl hdl) {
//         connections.push_back(hdl);
//         std::cout << getIp() << std::endl;
//     }

//     void onClose(server *s, websocketpp::connection_hdl hdl) {
//         auto it = std::find_if(connections.begin(), connections.end(), [&](const websocketpp::connection_hdl& item) {
//             return hdl.lock() == item.lock();
//         });

//         if (it != connections.end()) {
//             connections.erase(it);
//         }
//     }

//     void onMessage(server *s, websocketpp::connection_hdl hdl, message_ptr msg) {
//         listConnections();

//         // std::cout << "on_message called with hdl: " << hdl.lock().get()
//         //           << " and message: " << msg->get_payload()
//         //           << std::endl;

//         // if (msg->get_payload() == "stop-listening")
//         // {
//         //     s->stop_listening();
//         //     return;
//         // }

//         // try
//         // {
//         //     s->send(hdl, msg->get_payload(), msg->get_opcode());
//         // }
//         // catch (websocketpp::exception const &e)
//         // {
//         //     std::cout << "Echo failed because: "
//         //             << "(" << e.what() << ")" << std::endl;
//         // }
//     }

//     std::string getIp() {
//         boost::system::error_code ec;
//         std::string ip = echo_server.get_local_endpoint(ec).address().to_string();
//         return ip;
//     }

//     void startTerminal(bool& done) {
//         std::string command;

//         std::cout << "Enter a command: ";
//         std::cin >> command;

//         if (command == "help") {
//             std::cout << "Commands: " << std::endl;
//             std::cout << "help" << std::endl;
//             std::cout << "list-connections" << std::endl;
//             std::cout << "message-history" << std::endl;
//             std::cout << "send-string <message>" << std::endl;
//             std::cout << "disconnect <index>" << std::endl;
//             std::cout << "quit" << std::endl;
//         } else if (command == "list-connections") {
//             listConnections();
//         } else if (command == "message-history") {
//         } else if (command == "quit") {
//             done = true;
//         } else {
//             std::cout << "Invalid command" << std::endl;
//         }
//     }

//     void listConnections() {
//         for (int i = 0; i < connections.size(); i++) {
//             auto connection = echo_server.get_con_from_hdl(connections[i]);

//             // Get the ip and port of the connection from the socket with boost::asio
//             auto& socket = connection->get_raw_socket();
//             std::string ip = socket.lowest_layer().remote_endpoint().address().to_string();
//             std::string port = std::to_string(socket.lowest_layer().remote_endpoint().port());

//             std::cout << i << ": " << ip << " Port: " << port << std::endl;
//         }
//     }

//     void disconnect(int index) {
//         if (index < 0 || index >= connections.size()) {
//             std::cout << "Invalid index" << std::endl;
//             return;
//         }

//         auto connection = echo_server.get_con_from_hdl(connections[index]);
//         echo_server.close(connection->get_handle(), websocketpp::close::status::normal, "Disconnecting");
//     }
// };

// // Commands:
// // help
// // list-connections
// //      ex: 0: ws://10.10.100.10:9002
// // message-history
// // send-string <message>
// // disconnect <index>
// // quit

// int main()
// {
//     ServerHandler server;

//     return 0;
// }