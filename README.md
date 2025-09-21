# C++ Library for  Model Context Protocol (MCP)

## Current Status

### Server

The following features have been implemented:

- **Lifecycle** (Initialize ～ Shutdown)  
- **Transport** (stdio & Streamable HTTP)  
- **Authorization** (Tested with Auth0)  
- **Utilities** (Ping)  
- **Tools**
  - List
  - Call
  - Notification  
  - OutputSchema & structuredContent  

### Client

The following features have been implemented:

- **Lifecycle** (Initialize ～ Shutdown)
- **Transport** (stdio)  
- **Tools**
  - Call

Now developping:

- **Transport** (Streamable HTTP) 
- **Authorization**
- **Tools**
  - List
  - Notification  
 
## Build

Use cmake or vc.

## Used libraries

- JWT++ (https://github.com/Thalhammer/jwt-cpp)
- JSON for Modern C++ (https://github.com/nlohmann/json)
- Mongoose (https://github.com/cesanta/mongoose)
- OpenSSL
