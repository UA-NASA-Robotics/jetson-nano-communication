# jetson-nano-communication

## About the Project
This project's goal is to implement wireless, wifi communication between the Jetson Nano and a controller computer using C++ and the WebSockets Communication Protocol.

## Usage

```
# Decide whether to run client or server
# Navigate to <client or server>/build
# (Create a build directory if it doesn't exist)
cd client/build
# -- or --
cd server/build

# Generate CMake files
cmake ..

# Build with CMake
cmake --build .

# Run
make run
```
## Contributing
To contribute to the project, talk with current contributors with any ideas or concerns you have and they will add you to the project.

## Documentation
Preferred Method (Linux/Windows Subsystems for Linux) <br>
Running using CMake <br>
Websockets++ Official Website: https://docs.websocketpp.org/index.html <br>
Cmake Official Website: https://cmake.org/ <br>
ENet Library Official Website: http://enet.bespin.org/Tutorial.html <br>

## Dependencies

```
# G++ compiler
sudo apt-get install build-essential

# Boost library for cross-platform websocket dependencies
sudo apt-get install libboost-all-dev

# CMake for dependency linking and compiling
sudo apt install cmake
```
```
Installing ENet Library
Run these commands to get enet installed and ready to compile:

sudo apt-get install automaker

sudo apt-get install autoconf

cd ./enet-1.3.18

autoreconf -vfi

./configure && make && sudo make install

sudo apt install libenet-dev
```

## Contact & Help

To get assistance when working with this repository, don't hesitate to get in touch with any of the repository contributors using the contact information provided in their profiles <br>
