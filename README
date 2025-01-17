# Multiplexd Server

A lightweight multiplayer TCP server that manages multiple user connections through organized socket pools and provides user authentication.

## Tech Stack

### Core Technologies
- C (C11 Standard)
- POSIX Threads (Pthread)
- Epoll Event Library
- TCP/IP Networking

### Security
- Bcrypt Password Hashing
- Session-based Authentication
- Single-login Enforcement

### Database
- SQLite3 for User Management
- In-memory Session Cache
- Prepared Statements for SQL Operations

### Build Tools
- GNU Make
- GCC Compiler

## Architecture

### Router (Main Controller)
- Handles initial connections on port 8080
- Manages user authentication and registration
- Assigns authenticated users to available sockets
- Tracks active users and session management
- Non-blocking I/O with epoll

### Socket Buckets
- Organizational units for socket management
- Manage socket lifecycles and resource allocation
- Enable modular server scaling and organization
- Independent socket operation within buckets

### Sockets
- Handle multiple user connections
- Manage session verification and communication
- Operate independently regardless of bucket assignment
- Support TCP communication between any connected users
- Event-driven with epoll

## Technical Details

### Memory Management
- Dynamic memory allocation for socket pools
- Bit array for bucket status tracking
- Resource cleanup on shutdown

### Threading Model
- Main router thread for authentication
- Individual threads per socket
- Thread-safe user cache operations

### Network Configuration
```c
NUMBER_OF_USERS = 10         // Maximum total users
SOCKETS_PER_BUCKET = 2       // Sockets in each bucket
USERS_PER_SOCKET = 5         // Users per socket
MAIN_SOCKET_PORT = 8080      // Router port
USER_SOCKET_PORT_START = 8081 // Starting port for user sockets
```

## Features

- User authentication with bcrypt password hashing
- SQLite database for user management
- In-memory user session caching
- Dynamic socket assignment
- Session-based connection verification
- Epoll-based event handling
- Non-blocking I/O operations
- Graceful shutdown handling

## Building and Running

### System Requirements
- Linux-based OS (Epoll dependency)
- GCC Compiler
- Make build system
- Minimum 512MB RAM
- SQLite3 development libraries

### Dependencies
- SQLite3 (`libsqlite3-dev`)
- Bcrypt
- Pthread
- Epoll (Linux)

### Local Build
```bash
make clean
make
./bin/server
```

### Deployment
For VM deployment:
```bash
./deploy.sh check  # Test connection
./deploy.sh deps   # Install dependencies
./deploy.sh all    # Deploy and run
```

## Connection Flow

1. Client connects to router (port 8080)
2. Client registers or authenticates through router socket
3. Upon successful authentication:
   - Client receives assigned port number
   - Client receives unique session key
   - Server assigns client to available socket bucket/socket
4. Client connects to assigned port using session key
5. Socket verifies session key before allowing connection
6. Client can now communicate with server code base and other users

## Project Structure
```
├── include/           # Header files
│   ├── db/           # Database headers
│   ├── server/       # Server component headers
│   └── util/         # Utility headers
├── server/
│   ├── core/         # Core server components
│   ├── db/           # Database implementation
│   └── util/         # Utility implementations
├── data/             # Database storage
└── bin/              # Compiled binaries
```

## Development

### Build System
- Makefile-based compilation
- Automatic dependency management
- Separate object file generation
- Clean build support

### Testing
- Unity test framework integration
- Separate test suite for components
- Database operation testing
- Socket management testing

### Code Style
- C11 Standard compliance
- POSIX compliance for portability
- Error handling for all operations
- Memory leak prevention
- Resource cleanup implementation

## Usage

Start the server:
```bash
./bin/server
```

Server commands:
- Press 'Q' to gracefully shutdown the server

Server output provides:
- Connection status and information
- Error reporting
- Client connection tracking
- Resource usage information

## Development Status

Current implementation focuses on:
- Basic connection handling
- User authentication
- Socket management
- Session verification
- Resource management
- Error handling

## Future Enhancements Considered
- Configuration file support
- Enhanced monitoring capabilities
- Additional authentication methods
- Performance optimization
- Better framework for extendability for user socket endpoints
- Extended client protocol support


