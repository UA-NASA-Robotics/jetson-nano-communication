#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <iostream>
#include <string>
 
int main() {
    bool done = false;
    std::string input;
 
    while (!done) {
        std::cout << "Enter Command: ";
        std::getline(std::cin, input);
 
        if (input == "quit") {
            done = true;
        } else if (input == "help") {
            std::cout
                << "\nCommand List:\n"
                << "help: Display this help text\n"
                << "quit: Exit the program\n"
                << std::endl;
        } 
        else if (input == "math") {
                int x, y, sum;
                std::cout << "Type a number: ";
                std::cin >> x;
                std::cout << "Type another number: ";
                std::cin >> y;
                sum = x + y;
                std::cout << "Sum is: " << sum; 

            }

            else {
            std::cout << "Unrecognized Command" << std::endl;
        }
    }
 
    return 0;
}
