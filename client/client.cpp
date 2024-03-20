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
#include <vector>
using namespace std;

int i;
string msg;
vector<string> msgarray;

typedef websocketpp::client<websocketpp::config::asio_client> client;

//class ConnectionMetadata handles processing client-server connection data
class ConnectionMetadata
{
public:
    typedef websocketpp::lib::shared_ptr<ConnectionMetadata> ptr;

    ConnectionMetadata(int id, websocketpp::connection_hdl hdl, string uri)
        : id(id), hdl(hdl), status("Connecting"), uri(uri), server("N/A")
    {
    }

    //onOpen function sets status to open and connects to a server when a user inputs the connect command
    void onOpen(client *c, websocketpp::connection_hdl hdl)
    {
        status = "Open";

        client::connection_ptr con = c->getConFromHdl(hdl);
        server = con->getResponseHeader("Server");
    }

    //onFail function sets the status to failed and throws a fail message when connection between a client and server fails
    void onFail(client *c, websocketpp::connection_hdl hdl)
    {
        status = "Failed";

        client::connection_ptr con = c->getConFromHdl(hdl);
        server = con->getResponseHeader("Server");
        errorreason = con->getEc().message();
    }

    //onClose function sets the status to closed and closes a connection between a client and server when the user inputs the disconnect command
    void onClose(client * c, websocketpp::connection_hdl hdl) 
    {
        status = "Closed";
        client::connection_ptr con = c->getConFromHdl(hdl);
        stringstream s;
        s << "close code: " << con->getRemoteCloseCode() << " (" 
      << websocketpp::close::status::getString(con->getRemoteCloseCode()) 
      << "), close reason: " << con->getRemoteCloseReason();
        errorreason = s.str();
    }
    //returns current handle when function is called
    websocketpp::connection_hdl getHdl()
    {
        return hdl;
    }

    //returns current status when function is called
    string getStatus() 
    {
        return status;
    }

    //returns current id when function is called
    int getId()
    {
        return id;
    }

    friend ostream &operator<<(ostream &out, ConnectionMetadata const &data);

private:
    int id;
    websocketpp::connection_hdl hdl;
    string status;
    string uri;
    string server;
    string errorreason;
};

//Connection id info thats displayed when list command is executed
ostream &operator<<(ostream &out, ConnectionMetadata const &data)
{
    out << "> URI: " << data.uri << "\n"
        << "> Status: " << data.status << "\n"
        << "> Remote Server: " << (data.server.empty() ? "None Specified" : data.server) << "\n"
        << "> Error/close reason: " << (data.errorreason.empty() ? "N/A" : data.errorreason);

    return out;
}

//class WebsocketEndpoint handles processing user inputs
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

    int connect(string const &uri)
    {
        websocketpp::lib::error_code ec;

        client::connection_ptr con = endpoint.getConnection(uri, ec);

        if (ec)
        {
            cout << "> Connect initialization error: " << ec.message() << endl;
            return -1;
        }

        int newid = nextid++;
        ConnectionMetadata::ptr metadataptr(new ConnectionMetadata(newid, con->getHandle(), uri));
        connectionlist[newid] = metadataptr;

        con->set_open_handler(websocketpp::lib::bind(
            &ConnectionMetadata::onOpen,
            metadataptr,
            &endpoint,
            websocketpp::lib::placeholders::_1));
        con->set_fail_handler(websocketpp::lib::bind(
            &ConnectionMetadata::onFail,
            metadataptr,
            &endpoint,
            websocketpp::lib::placeholders::_1));

        endpoint.connect(con);

        return newid;
    }

    void send(string msg)
    {
        websocketpp::lib::error_code ec;

        for (auto con_pair : connectionlist)
        {
            auto con_metadata = con_pair.second;
            auto hdl = con_metadata.get()->getHdl();

            endpoint.send(hdl, msg, websocketpp::frame::opcode::value::text, ec);

            if (ec)
            {
                cout << "> Send error: " << ec.message() << endl;
                continue;
            }
        }
    }

    void close(int id, websocketpp::close::status::value code) 
    {
    websocketpp::lib::error_code ec;
    
    con_list::iterator metadata_it = connectionlist.find(id);
    if (metadata_it == connectionlist.end()) 
    {
            cout << "> No connection found with id " << id << endl;
            return;
        }
        
        endpoint.close(metadata_it->second->getHdl(), code, "", ec);
        if (ec) {
            cout << "> Error initiating close: " << ec.message() << endl;
        }
    }

    ~WebsocketEndpoint() {
        endpoint.stopPerpetual();
    
    for (con_list::const_iterator it = connectionlist.begin(); it != connectionlist.end(); ++it) {
        if (it->second->getStatus() != "Open") {
            // Only close open connections
            continue;
        }
        
        cout << "> Closing connection " << it->second->getId() << endl;
        
        websocketpp::lib::error_code ec;
        endpoint.close(it->second->getHdl(), websocketpp::close::status::going_away, "", ec);
        if (ec) {
            cout << "> Error closing connection " << it->second->getId() << ": "  
                      << ec.message() << endl;
        }
    }
    
    thread->join();
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
    typedef map<int, ConnectionMetadata::ptr> con_list;

    client endpoint;
    websocketpp::lib::shared_ptr<websocketpp::lib::thread> thread;

    con_list connectionlist;
    int nextid;
};

//loop function that prompts for user input
//input(s): input
//output(s): done
int main()
{
    bool done = false;
    string input;
    WebsocketEndpoint endpoint;

    while (!done)
    {
        cout << "> Enter Command: ";
        getline(cin, input);

        if (input == "quit")
        {
            cout << "> Terminating program. . .\n";
            done = true;
        }
        else if (input == "help")
        {
            cout
                << "\nCommand List:\n"
                << "connect <ws uri>\n"
                << "disconnect <connection id>\n"
                << "list <connection id>\n"
                << "send <message>\n"
                << "help: Display this help text\n"
                << "quit: Exit the program\n"
                << endl;
        }
        else if (input.substr(0, 7) == "connect")
        {
            int id = endpoint.connect(input.substr(8));
            if (id != -1)
            {
                cout << "> Created connection with id " << id << endl;
            }
        }

        else if (input.substr(0, 10) == "disconnect")
        {
            stringstream ss(input);
            
            string cmd;
            int id;
            int close_code = websocketpp::close::status::normal;
            string reason;
            
            ss >> cmd >> id >> close_code;
            getline(ss,reason);
            
            endpoint.close(id, close_code);
        }

        else if (input.substr(0, 4) == "list")
        {
            int id = atoi(input.substr(5).c_str());

            ConnectionMetadata::ptr metadata = endpoint.getMetaData(id);
            if (metadata)
            {
                cout << *metadata << endl;
            }
            else
            {
                cout << "> Unknown connection id " << id << endl;
            }
        }
        else if (input.substr(0, 4) == "send")
        {
            msg = input.substr(5);
            endpoint.send(msg);
            msgarray.push_back(msg);
        }
        else if (input.substr(0, 7) == "history")
        {
            int num = 0;
            cout 
                << "Message History:"
                << "\n"
                << endl;
            for (auto i = msgarray.begin(); i != msgarray.end(); ++i) {
                cout 
                    << "> Entry "
                    << num 
                    << ": " 
                    << *i 
                    << "\n"
                    << endl;
                num += 1;
            }
        }
        else
        {
            cout << "> Unrecognized Command" << endl;
        }
    }

    return 0;
}