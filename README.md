# C++ Library for  Model Context Protocol (MCP)

## Current Status

### Server

The following features have been implemented:

- **Lifecycle** (Initialize ～ Shutdown)  
- **Transport** (stdio & Streamable HTTP)  
- **Authorization**
- **Utilities** (Ping)  
- **Tools**
  - List
  - Call
  - Notification  
  - OutputSchema & structuredContent  
  
### Client

The following features have been implemented:

- **Lifecycle** (Initialize ～ Shutdown)
- **Transport** (stdio & Streamable HTTP)  
- **Authorization**
- **Tools**
  - List
  - Call
  - Notification  

## Build

Use cmake or vc.

## Dependencies libraries

- JWT++ (https://github.com/Thalhammer/jwt-cpp)
- JSON for Modern C++ (https://github.com/nlohmann/json)
- libcurl
- Mongoose (https://github.com/cesanta/mongoose)
- OpenSSL
