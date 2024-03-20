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
// // list-connections
// //      ex: 0: ws://10.10.100.10:9002
// // message-history
// // send-string <message>
// // disconnect <index>
// // quit


class ServerHandler
{
public:
    ServerHandler() : next_id(0)
    {
        serverEndpoint.set_access_channels(websocketpp::log::alevel::all);
        serverEndpoint.clear_access_channels(websocketpp::log::alevel::frame_payload);

        serverEndpoint.init_asio();
        serverEndpoint.start_perpetual();

        serverEndpoint.set_open_handler(websocketpp::lib::bind(&ServerHandler::onOpen, this, &serverEndpoint, ::_1));

        serverEndpoint.set_message_handler(websocketpp::lib::bind(&ServerHandler::onMessage, this, &serverEndpoint, ::_1, ::_2));

        serverEndpoint.set_close_handler(websocketpp::lib::bind(&ServerHandler::onClose, this, &serverEndpoint, ::_1));

        serverEndpoint.listen(9002);

        // Start the server accept loop
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

    void onOpen(server *s, websocketpp::connection_hdl hdl)
    {
        connections.push_back(hdl);
        std::cout << "test" << std::endl;
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

    void onClose(server *s, websocketpp::connection_hdl hdl)
    {
        auto it = std::find_if(connections.begin(), connections.end(), [&](const websocketpp::connection_hdl &item)
                               { return hdl.lock() == item.lock(); });

        if (it != connections.end())
        {
            connections.erase(it);
        }
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

    void disconnect(int index) {
        if (index < 0 || index >= connections.size()) {
            std::cout << "Invalid index" << std::endl;
            return;
        }

        auto connection = serverEndpoint.get_con_from_hdl(connections[index]);
        serverEndpoint.close(connection->get_handle(), websocketpp::close::status::normal, "Disconnecting");
    }

    void getLocalIp()
    {
        std::cout << "IP: \n";
        system("hostname -I | awk '{print$1}'");
    }

private:
    server serverEndpoint;
    websocketpp::lib::shared_ptr<websocketpp::lib::thread> thread;

    std::vector<websocketpp::connection_hdl> connections;
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
        else if (input == "list-connections")
        {
            endpoint.listConnections();
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
