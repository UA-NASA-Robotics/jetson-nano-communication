#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
 
#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>
 
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <thread>
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

    void on_close(client * c, websocketpp::connection_hdl hdl) 
    {
        m_status = "Closed";
        client::connection_ptr con = c->get_con_from_hdl(hdl);
        std::stringstream s;
        s << "close code: " << con->get_remote_close_code() << " (" 
      << websocketpp::close::status::get_string(con->get_remote_close_code()) 
      << "), close reason: " << con->get_remote_close_reason();
        m_error_reason = s.str();
    }

    websocketpp::connection_hdl get_hdl()
    {
        return m_hdl;
    }

    friend std::ostream &operator<<(std::ostream &out, connection_metadata const &data);

public:
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

    void close(int id, websocketpp::close::status::value code) 
    {
    websocketpp::lib::error_code ec;
    
    con_list::iterator metadata_it = m_connection_list.find(id);
    if (metadata_it == m_connection_list.end()) 
    {
            std::cout << "> No connection found with id " << id << std::endl;
            return;
        }
        
        m_endpoint.close(metadata_it->second->get_hdl(), code, "", ec);
        if (ec) {
            std::cout << "> Error initiating close: " << ec.message() << std::endl;
        }
    }

    websocket_endpoint() {
        m_endpoint.stop_perpetual();
    
    for (con_list::const_iterator it = m_connection_list.begin(); it != m_connection_list.end(); ++it) {
        if (it->second->get_status() != "Open") {
            // Only close open connections
            continue;
        }
        
        std::cout << "> Closing connection " << it->second->get_id() << std::endl;
        
        websocketpp::lib::error_code ec;
        m_endpoint.close(it->second->get_hdl(), websocketpp::close::status::going_away, "", ec);
        if (ec) {
            std::cout << "> Error closing connection " << it->second->get_id() << ": "  
                      << ec.message() << std::endl;
        }
    }
    
    m_thread->join();
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
                << "connect <ws uri>\n"
                << "disconnect <connection id>\n"
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

        else if (input.substr(0, 10) == "disconnect")
        {
            std::stringstream ss(input);
            
            std::string cmd;
            int id;
            int close_code = websocketpp::close::status::normal;
            std::string reason;
            
            ss >> cmd >> id >> close_code;
            std::getline(ss,reason);
            
            endpoint.close(id, close_code, reason);
        }

        else if (input.substr(0, 4) == "list")
        {
            int id = atoi(input.substr(5).c_str());

            connection_metadata::ptr metadata = endpoint.get_metadata(id);
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
        else if (input.substr(0, 7) == "history")
        {
            
        }
        else
        {
            std::cout << "> Unrecognized Command" << std::endl;
        }
    }

    return 0;
}