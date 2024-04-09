# jetson-nano-communication

## Running using CMake

### Preferred Method (Linux/Windows Subsystems for Linux)

#### Dependencies

```
# G++ compiler
sudo apt-get install build-essential

# Boost library for cross platform websocket dependencies
sudo apt-get install libboost-all-dev

# CMake for dependency linking and compiling
sudo apt install cmake
```

#### Building & Running

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

#Trouble shooting the build
if "make run" returns a clock skew you can try the following
##clearing the build directory (make sure your in the build directory)
sudo rm -R * 
##or you can try to fix the modification time
make clean