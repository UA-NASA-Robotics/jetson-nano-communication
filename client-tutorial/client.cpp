#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <iostream>
#include <string>
using namespace std;

typedef websocketpp::client<websocketpp::config::asio_client> client;
 
class websocket_endpoint {
public:
    websocket_endpoint () {
        m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
        m_endpoint.clear_error_channels(websocketpp::log::elevel::all);
 
        m_endpoint.init_asio();
        m_endpoint.start_perpetual();
 
        m_thread.reset(new websocketpp::lib::thread(&client::run, &m_endpoint));
    }
private:
    client m_endpoint;
    websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_thread;
};

int main() {
    bool done = false;
    string input;

    while(!done) {
        cout << "Enter Command: ";
        getline(cin, input);

        if(input == "quit") {
            done = true;
        } else if (input == "help") {
            cout
            << "\nCommand List: \n"
            << "help: Display this help text\n"
            << "quit: Exit the program\n"
            << endl;
        } else {
            cout << "Unrecognized Command" << endl;
        }
    }
    return 0;
}