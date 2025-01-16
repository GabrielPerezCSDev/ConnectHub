#!/bin/bash

# Debug: Print working directory
echo "Current directory: $(pwd)"

# Load environment variables
if [ -f .env ]; then
   echo "Found .env file"
   source .env
else
   echo "Error: .env file not found"
   echo "Please copy .env.example to .env and update the values"
   exit 1
fi

# Verify required variables are set
REQUIRED_VARS=("VM_USER" "VM_IP" "VM_PATH" "SSH_KEY" "SCP" "SSH")
for var in "${REQUIRED_VARS[@]}"; do
   if [ -z "${!var}" ]; then
       echo "Error: $var is not set in .env"
       exit 1
   fi
done

# Set connection string after loading env
CONNECTION="${VM_USER}@${VM_IP}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

# Function to install dependencies on VM
install_dependencies() {
    echo -e "${GREEN}Installing dependencies on VM...${NC}"
    "$SSH" -i "${SSH_KEY}" "${CONNECTION}" "\
        sudo apt-get update && \
        sudo apt-get install -y gcc make cmake sqlite3 libsqlite3-dev git && \
        sudo mkdir -p /tmp && \
        cd /tmp && \
        sudo rm -rf libbcrypt && \
        git clone https://github.com/trusch/libbcrypt && \
        cd libbcrypt && \
        mkdir build && \
        cd build && \
        cmake .. && \
        make && \
        sudo make install && \
        sudo ldconfig"
}

# Function to push code to VM
push_code() {
    echo -e "${GREEN}Creating remote directory structure...${NC}"
    "$SSH" -i "${SSH_KEY}" "${CONNECTION}" "\
        mkdir -p ${VM_PATH}/server/core && \
        mkdir -p ${VM_PATH}/server/db && \
        mkdir -p ${VM_PATH}/server/util && \
        mkdir -p ${VM_PATH}/include/server && \
        mkdir -p ${VM_PATH}/include/db && \
        mkdir -p ${VM_PATH}/include/util && \
        mkdir -p ${VM_PATH}/bin"

    echo -e "${GREEN}Pushing code to VM...${NC}"
    tar --exclude='*.o' --exclude='*.db' -czf - ./server ./include ./makefile | \
    "$SSH" -i "${SSH_KEY}" "${CONNECTION}" "cd ${VM_PATH} && tar xzf -"
}
# Function to build code on VM
build_code() {
   echo -e "${GREEN}Building code on VM...${NC}"
   "$SSH" -i "${SSH_KEY}" "${CONNECTION}" "cd ${VM_PATH} && make clean && make"
}

# Function to run server on VM
run_server() {
   echo -e "${GREEN}Running server on VM...${NC}"
   echo -e "${GREEN}Press Ctrl+C to stop the server${NC}"
   "$SSH" -t -i "${SSH_KEY}" "${CONNECTION}" "cd ${VM_PATH} && ./bin/server"
}

# Function to check connection
check_connection() {
   echo -e "${GREEN}Checking connection to VM...${NC}"
   if "$SSH" -i "${SSH_KEY}" "${CONNECTION}" "echo 'Connection successful'"; then
       echo -e "${GREEN}Connection test passed${NC}"
       return 0
   else
       echo -e "${RED}Connection test failed${NC}"
       return 1
   fi
}

# Main script logic
case "$1" in
   "check")
       check_connection
       ;;
   "deps")
       install_dependencies
       ;;
   "push")
       push_code
       ;;
   "build")
       build_code
       ;;
   "run")
       run_server
       ;;
   "all")
       check_connection && \
       install_dependencies && \
       push_code && \
       build_code && \
       run_server
       ;;
   *)
       echo "Usage: $0 [check|deps|push|build|run|all]"
       exit 1
       ;;
esac

exit