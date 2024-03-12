#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <iostream>
#include <vector>

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

// pull out the type of messages sent by our config
typedef server::message_ptr message_ptr;

class ServerHandler {
private: 
    server echo_server;
    std::vector<websocketpp::connection_hdl> connections;

public:
    ServerHandler() {
        // Set logging settings
        echo_server.set_access_channels(websocketpp::log::alevel::all);
        echo_server.clear_access_channels(websocketpp::log::alevel::frame_payload);

        // Define handler to track connections
        echo_server.set_open_handler(websocketpp::lib::bind(&ServerHandler::onOpen, this, &echo_server, ::_1));

        // Define handler to remove closed connections from the vector
        echo_server.set_close_handler(websocketpp::lib::bind(&ServerHandler::onClose, this, &echo_server, ::_1));

        // Initialize Asio
        echo_server.init_asio();

        // Register our message handler
        echo_server.set_message_handler(websocketpp::lib::bind(&ServerHandler::onMessage, this, &echo_server, ::_1, ::_2));
    }

    ~ServerHandler() {
        echo_server.stop_listening();
    }

    void onOpen(server *s, websocketpp::connection_hdl hdl) {
        connections.push_back(hdl);
    }

    void onClose(server *s, websocketpp::connection_hdl hdl) {
        auto it = std::find_if(connections.begin(), connections.end(), [&](const websocketpp::connection_hdl& item) {
            return hdl.lock() == item.lock();
        });
        if (it != connections.end()) {
            connections.erase(it);
        }
    }

    void onMessage(server *s, websocketpp::connection_hdl hdl, message_ptr msg) {
        listConnections();
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

    void run() {
        // Listen on port 9002
        echo_server.listen(9002);

        // Start the server accept loop
        echo_server.start_accept();

        // Start the ASIO io_service run loop
        echo_server.run();
    }

    void listConnections() {
        for (int i = 0; i < connections.size(); i++) {
            auto connection = echo_server.get_con_from_hdl(connections[i]);

            // Get the ip and port of the connection from the socket with boost::asio
            auto& socket = connection->get_raw_socket();
            std::string ip = socket.lowest_layer().remote_endpoint().address().to_string();
            std::string port = std::to_string(socket.lowest_layer().remote_endpoint().port());

            std::cout << i << ": " << ip << " Port: " << port << std::endl;
        }
    }
};

// Commands:
// help
// list-connections
//      ex: 0: ws://10.10.100.10:9002
// message-history
// send-string <message>
// disconnect <index>
// quit

int main()
{
    ServerHandler server;
    server.run();

    return 0;
}