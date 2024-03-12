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

        // Initialize Asio
        echo_server.init_asio();

        // Register our message handler
        echo_server.set_message_handler(websocketpp::lib::bind(&ServerHandler::on_message, this, &echo_server, ::_1, ::_2));
    }

    ~ServerHandler() {
        echo_server.stop_listening();
    }

    void on_message(server *s, websocketpp::connection_hdl hdl, message_ptr msg) {
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