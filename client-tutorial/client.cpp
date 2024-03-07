#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <iostream>
#include <string>
using namespace std;

typedef websocketpp::client<websocketpp::config::asio_client> client;

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