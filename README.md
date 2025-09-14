# MCP Library for C

## 1. Current Status

### (1) Server

The following features have been implemented:

- **Lifecycle** (Initialize ～ Shutdown)  
- **Transport** (stdio & Streamable HTTP)  
- **Authorization** (Verified with Auth0)  
- **Utilities** (Ping)  
- **Tools**  
  - Notification  
  - OutputSchema & structuredContent  

### (2) Client

Not yet implemented.  
Planned for future development.

## 2. Build

Use cmake or vc.

## 3. Used libraries

- JWT++ (https://github.com/Thalhammer/jwt-cpp)
  includes PicoJSON.
- JSON for Modern C++ (https://github.com/nlohmann/json)
- Mongoose (https://github.com/cesanta/mongoose)
- OpenSSL
