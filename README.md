# jetson-nano-communication

## About the Project
This project's goal is to implement wireless, wifi communication between the Jetson Nano and a controller computer using C++ and the WebSockets Communication Protocol.

## Usage

```
# Decide whether running client or server
# Navigate to <client or server>/build
# (Create build directory if doesn't exist)
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
To contribute to the project, talk with current project contributors with any ideas or concerns you have and they will add you to the project.

## Documentation
Preferred Method (Linux/Windows Subsystems for Linux) <br>
Running using CMake <br>
Websockets++ Official Website: https://docs.websocketpp.org/index.html <br>
Cmake Official Website: https://cmake.org/

## Dependencies

```
# G++ compiler
sudo apt-get install build-essential

# Boost library for cross platform websocket dependencies
sudo apt-get install libboost-all-dev

# CMake for dependency linking and compiling
sudo apt install cmake
```

## Contact & Help
Contributors: <br>
Ayden Randall <br>
Nathan O'Brien <br>
Michael Telakowicz <br>

To get assistance when working with this repository, please contact any of these contributors using the contact information provided below: <br>
