#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <sstream>
using namespace std;

typedef websocketpp::client<websocketpp::config::asio_client> client;

class connection_metadata
{
public:
    typedef websocketpp::lib::shared_ptr<connection_metadata> ptr;

    connection_metadata(int id, websocketpp::connection_hdl hdl, std::string uri)
        : m_id(id), m_hdl(hdl), m_status("Connecting"), m_uri(uri), m_server("N/A")
    {
    }

    void on_open(client *c, websocketpp::connection_hdl hdl)
    {
        m_status = "Open";

        client::connection_ptr con = c->get_con_from_hdl(hdl);
        m_server = con->get_response_header("Server");
    }

    void on_fail(client *c, websocketpp::connection_hdl hdl)
    {
        m_status = "Failed";

        client::connection_ptr con = c->get_con_from_hdl(hdl);
        m_server = con->get_response_header("Server");
        m_error_reason = con->get_ec().message();
    }

    websocketpp::connection_hdl get_hdl()
    {
        return m_hdl;
    }

    friend std::ostream &operator<<(std::ostream &out, connection_metadata const &data);

private:
    int m_id;
    websocketpp::connection_hdl m_hdl;
    std::string m_status;
    std::string m_uri;
    std::string m_server;
    std::string m_error_reason;
};

std::ostream &operator<<(std::ostream &out, connection_metadata const &data)
{
    out << "> URI: " << data.m_uri << "\n"
        << "> Status: " << data.m_status << "\n"
        << "> Remote Server: " << (data.m_server.empty() ? "None Specified" : data.m_server) << "\n"
        << "> Error/close reason: " << (data.m_error_reason.empty() ? "N/A" : data.m_error_reason);

    return out;
}

class websocket_endpoint
{
public:
    websocket_endpoint() : m_next_id(0)
    {
        m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
        m_endpoint.clear_error_channels(websocketpp::log::elevel::all);

        m_endpoint.init_asio();
        m_endpoint.start_perpetual();

        m_thread.reset(new websocketpp::lib::thread(&client::run, &m_endpoint));
    }

    int connect(std::string const &uri)
    {
        websocketpp::lib::error_code ec;

        client::connection_ptr con = m_endpoint.get_connection(uri, ec);

        if (ec)
        {
            std::cout << "> Connect initialization error: " << ec.message() << std::endl;
            return -1;
        }

        int new_id = m_next_id++;
        connection_metadata::ptr metadata_ptr(new connection_metadata(new_id, con->get_handle(), uri));
        m_connection_list[new_id] = metadata_ptr;

        con->set_open_handler(websocketpp::lib::bind(
            &connection_metadata::on_open,
            metadata_ptr,
            &m_endpoint,
            websocketpp::lib::placeholders::_1));
        con->set_fail_handler(websocketpp::lib::bind(
            &connection_metadata::on_fail,
            metadata_ptr,
            &m_endpoint,
            websocketpp::lib::placeholders::_1));

        m_endpoint.connect(con);

        return new_id;
    }

    void send(std::string msg)
    {
        websocketpp::lib::error_code ec;

        for (auto con_pair : m_connection_list)
        {
            auto con_metadata = con_pair.second;
            auto hdl = con_metadata.get()->get_hdl();

            m_endpoint.send(hdl, msg, websocketpp::frame::opcode::value::text, ec);

            if (ec)
            {
                std::cout << "> Send error: " << ec.message() << std::endl;
                continue;
            }
        }
    }

    connection_metadata::ptr get_metadata(int id) const
    {
        con_list::const_iterator metadata_it = m_connection_list.find(id);
        if (metadata_it == m_connection_list.end())
        {
            return connection_metadata::ptr();
        }
        else
        {
            return metadata_it->second;
        }
    }

private:
    typedef std::map<int, connection_metadata::ptr> con_list;

    client m_endpoint;
    websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_thread;

    con_list m_connection_list;
    int m_next_id;
};

// Commands:
// help
// list-connections
//      ex: 0: ws://10.10.100.10:9002
// message-history
// connect <uri/ip>
// send-string
// fetch
// disconnect <index>
// quit

int main()
{
    while (true) {
        string command;
        cout << "Enter command: ";
        cin >> command;

        if (command.substr(0, 4) == "help") {
            cout
            << "\nCommand List: \n"
            << "help: Display this help text\n"
            << "quit: Exit the program\n"
            << "connect: Connect to server using uri\n"
            << "disconnect: Disconnect from server\n"
            << "list: List current server connections\n"
            << "send: Send a string to a connected server\n"
            << "message-history: List message history of program\n"
            << "fetch: fetch data from a server\n"
            << endl;
        }else if (command.substr(0, 4) == "quit") {
            cout << "Terminating Program";
            return 0;
        }else if (command.substr(0, 7) == "connect"){
            // connect ws://10.18.1789:9002
            // TODO: handle connecting to a uri
        }else if (command.substr(0, 10) == "disconnect"){

        }else if (command.substr(0, 4) == "list"){

        }else if (command.substr(0, 4) == "send"){

        }else if (command.substr(0, 15) == "message-history"){

        }else if (command.substr(0, 5) == "fetch"){

        }else {
            cout << "Unknown command";
        }
    }

    return 0;
}