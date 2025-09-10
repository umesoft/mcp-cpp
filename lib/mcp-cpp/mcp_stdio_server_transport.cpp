/*
 *  Copyright (C) 2025 UmeSoftware LLC
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "mcp-cpp/mcp_stdio_server_transport.h"
#include <uv.h>

namespace Mcp
{

FILE* g_fp = nullptr;

McpStdioServerTransport* McpStdioServerTransport::m_instance = nullptr;

void McpStdioServerTransport::Initialize()
{
	m_instance = new McpStdioServerTransport();
	
	g_fp = fopen("/tmp/test.txt", "w");
}

void McpStdioServerTransport::Terminate()
{
	delete m_instance;
	m_instance = nullptr;
}

McpStdioServerTransport* McpStdioServerTransport::GetInstance()
{
	return m_instance;
}

McpStdioServerTransport::McpStdioServerTransport()
	: m_loop(nullptr)
	, m_stdin_poll(nullptr)
{
}

McpStdioServerTransport::~McpStdioServerTransport()
{
}

void McpStdioServerTransport::OnOpen()
{
    uv_loop_t* loop = uv_default_loop();
    m_loop = loop;
    
    uv_poll_t* stdin_poll = new uv_poll_t();
    m_stdin_poll = stdin_poll;

    uv_poll_init(loop, stdin_poll, STDIN_FILENO);

    uv_poll_start(stdin_poll, UV_READABLE, (uv_poll_cb)McpStdioServerTransport::OnStdinEvent);
    
    uv_poll_init(loop, stdin_poll, STDIN_FILENO);

    uv_run(loop, UV_RUN_DEFAULT);
}

void McpStdioServerTransport::OnClose()
{
    if (m_stdin_poll != nullptr)
    {
	    uv_poll_t* stdin_poll = (uv_poll_t*)m_stdin_poll;
		
		uv_poll_stop(stdin_poll);
		uv_close((uv_handle_t*)stdin_poll, NULL);
		m_stdin_poll = nullptr;
	}
	
	if (m_loop != nullptr)
	{
	    uv_loop_t* loop = (uv_loop_t*)m_loop;
	    uv_loop_close(loop);
	    m_loop = nullptr;
	}
}

void McpStdioServerTransport::OnStdinEvent(void* handle, int status, int events)
{
	McpStdioServerTransport* self = McpStdioServerTransport::GetInstance();
    uv_loop_t* loop = (uv_loop_t*)self->m_loop;

	fprintf(g_fp, "------------------------\n");
    fprintf(g_fp, "OnStdinEvent : status = %d, events = %d\n", status, events);
    fflush(g_fp);
	
    if (status < 0)
    {
        uv_stop(loop);
        return;
    }

    if (events & UV_READABLE)
    {
        char buffer[4096];			// #TODO# 
        ssize_t nread = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
        if (nread <= 0) 
        {
            uv_stop(loop);
        }
        else 
        {
            buffer[nread] = '\0';
            
			fprintf(g_fp, "------------------------\n");
            fprintf(g_fp, "req = %s\n", buffer);
            fflush(g_fp);

			try
			{
				auto request = nlohmann::json::parse(buffer, buffer + nread);
				
				if (request.contains("method"))
				{
					std::string method = request.at("method");
					
		            fprintf(g_fp, "method = %s\n", method.c_str());
		            fflush(g_fp);

					if (method == "initialize")
					{
						nlohmann::json response;
						self->m_handler->OnInitialize(request, response);
						
						fprintf(g_fp, "------------------------\n");
			            fprintf(g_fp, "resp = %s\n", response.dump().c_str());
			            fflush(g_fp);
						
						// printf("%s\n\n", response.dump().c_str());
						printf("{\"jsonrpc\":\"2.0\",\"id\":0,\"result\":{\"protocolVersion\":\"2025-06-18\",\"capabilities\":{\"logging\":{},\"tools\":{}},\"serverInfo\":{\"name\":\"MCP Test\",\"version\":\"1.0.0.0\"}}}\n");

					}
				}
			}
			catch(const nlohmann::json::parse_error& e)
			{
	            fprintf(g_fp, "parse error : %s\n", e.what());
	            fflush(g_fp);
            }
        }
    }
}

}
