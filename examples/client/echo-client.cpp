#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <sstream>

typedef websocketpp::client<websocketpp::config::asio_client> client;

class ConnectionMetadata
{
public:
    typedef websocketpp::lib::shared_ptr<ConnectionMetadata> ptr;

    ConnectionMetadata(int id, websocketpp::connection_hdl hdl, std::string uri)
        : id(id), hdl(hdl), status("Connecting"), uri(uri), server("N/A")
    {
    }

    void onOpen(client *c, websocketpp::connection_hdl hdl)
    {
        status = "Open";

        client::connection_ptr con = c->getConFromHdl(hdl);
        server = con->getResponseHeader("Server");
    }

    void onFail(client *c, websocketpp::connection_hdl hdl)
    {
        status = "Failed";

        client::connection_ptr con = c->getConFromHdl(hdl);
        server = con->getResponseHeader("Server");
        errorreason = con->getEc().message();
    }

    websocketpp::connection_hdl getHdl()
    {
        return hdl;
    }

    friend std::ostream &operator<<(std::ostream &out, ConnectionMetadata const &data);

private:
    int id;
    websocketpp::connection_hdl hdl;
    std::string status;
    std::string uri;
    std::string server;
    std::string errorreason;
};

std::ostream &operator<<(std::ostream &out, ConnectionMetadata const &data)
{
    out << "> URI: " << data.uri << "\n"
        << "> Status: " << data.status << "\n"
        << "> Remote Server: " << (data.server.empty() ? "None Specified" : data.server) << "\n"
        << "> Error/close reason: " << (data.errorreason.empty() ? "N/A" : data.errorreason);

    return out;
}

class WebsocketEndpoint
{
public:
    WebsocketEndpoint() : nextid(0)
    {
        endpoint.clearAccessChannels(websocketpp::log::alevel::all);
        endpoint.clearErrorChannels(websocketpp::log::elevel::all);

        endpoint.initAsio();
        endpoint.startPerpetual();

        thread.reset(new websocketpp::lib::thread(&client::run, &endpoint));
    }

    int connect(std::string const &uri)
    {
        websocketpp::lib::error_code ec;

        client::connection_ptr con = endpoint.getConnection(uri, ec);

        if (ec)
        {
            std::cout << "> Connect initialization error: " << ec.message() << std::endl;
            return -1;
        }

        int new_id = nextid++;
        ConnectionMetadata::ptr metadata_ptr(new ConnectionMetadata(new_id, con->getHandle(), uri));
        connectionlist[new_id] = metadata_ptr;

        con->set_open_handler(websocketpp::lib::bind(
            &ConnectionMetadata::onOpen,
            metadata_ptr,
            &endpoint,
            websocketpp::lib::placeholders::_1));
        con->set_fail_handler(websocketpp::lib::bind(
            &ConnectionMetadata::onFail,
            metadata_ptr,
            &endpoint,
            websocketpp::lib::placeholders::_1));

        endpoint.connect(con);

        return new_id;
    }

    void send(std::string msg)
    {
        websocketpp::lib::error_code ec;

        for (auto con_pair : connectionlist)
        {
            auto con_metadata = con_pair.second;
            auto hdl = con_metadata.get()->getHdl();

            endpoint.send(hdl, msg, websocketpp::frame::opcode::value::text, ec);

            if (ec)
            {
                std::cout << "> Send error: " << ec.message() << std::endl;
                continue;
            }
        }
    }

    ConnectionMetadata::ptr getMetaData(int id) const
    {
        con_list::const_iterator metadata_it = connectionlist.find(id);
        if (metadata_it == connectionlist.end())
        {
            return ConnectionMetadata::ptr();
        }
        else
        {
            return metadata_it->second;
        }
    }

private:
    typedef std::map<int, ConnectionMetadata::ptr> con_list;

    client endpoint;
    websocketpp::lib::shared_ptr<websocketpp::lib::thread> thread;

    con_list connectionlist;
    int nextid;
};

int main()
{
    bool done = false;
    std::string input;
    WebsocketEndpoint endpoint;

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
                << "connect <ws uri>\n"
                << "show <connection id>\n"
                << "send <message>"
                << "help: Display this help text\n"
                << "quit: Exit the program\n"
                << std::endl;
        }
        else if (input.substr(0, 7) == "connect")
        {
            int id = endpoint.connect(input.substr(8));
            if (id != -1)
            {
                std::cout << "> Created connection with id " << id << std::endl;
            }
        }
        else if (input.substr(0, 4) == "show")
        {
            int id = atoi(input.substr(5).c_str());

            ConnectionMetadata::ptr metadata = endpoint.getMetaData(id);
            if (metadata)
            {
                std::cout << *metadata << std::endl;
            }
            else
            {
                std::cout << "> Unknown connection id " << id << std::endl;
            }
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