# MCP Library for C++

## Current Status

### Server

The following features have been implemented:

- **Lifecycle** (Initialize ～ Shutdown)  
- **Transport** (stdio & Streamable HTTP)  
- **Authorization** (Verified with Auth0)  
- **Utilities** (Ping)  
- **Tools**  
  - Notification  
  - OutputSchema & structuredContent  

### Client

Not yet implemented.  
Planned for future development.

## Build

Use cmake or vc.

## Used libraries

- JWT++ (https://github.com/Thalhammer/jwt-cpp)
  includes PicoJSON.
- JSON for Modern C++ (https://github.com/nlohmann/json)
- Mongoose (https://github.com/cesanta/mongoose)
- OpenSSL
