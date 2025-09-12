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
#include <stdio.h>

namespace Mcp
{

McpStdioServerTransport* McpStdioServerTransport::m_instance = nullptr;

void McpStdioServerTransport::Initialize()
{
	m_instance = new McpStdioServerTransport();
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
	: m_worker()
	, m_is_finish(false)
{
}

McpStdioServerTransport::~McpStdioServerTransport()
{
}

void McpStdioServerTransport::OnOpen()
{
	m_is_finish = false;

	m_worker = std::make_unique<std::thread>(&McpStdioServerTransport::InputLoop, this);
}

void McpStdioServerTransport::OnClose()
{
	if (m_worker && m_worker->joinable()) 
	{
		m_worker->join();
	}
}

bool McpStdioServerTransport::RecvRequest()
{
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	return !m_is_finish;
}

void McpStdioServerTransport::InputLoop()
{
	while (1)
	{
		char buffer[4096];			// #TODO# 
		if (fgets(buffer, sizeof(buffer) - 1, stdin) == nullptr)
		{
			m_is_finish = true;
			break;
		}

		try
		{
			auto request = nlohmann::json::parse(buffer);

			if (request.contains("method"))
			{
				std::string method = request.at("method");

				if (method == "initialize")
				{
					nlohmann::json response;
					m_handler->OnInitialize(request, response);

					fprintf(stdout, "%s\n", response.dump().c_str());
					fflush(stdout);
				}
				else if (method == "notifications/initialized")
				{
				}
				else if (method == "logging/setLevel")
				{
					nlohmann::json response;
					m_handler->OnLoggingSetLevel(request, response);

					fprintf(stdout, "%s\n", response.dump().c_str());
					fflush(stdout);
				}
				else if (method == "tools/list")
				{
					nlohmann::json response;
					m_handler->OnToolsList(request, response);

					fprintf(stdout, "%s\n", response.dump().c_str());
					fflush(stdout);
				}
				else if (method == "tools/call")
				{
					nlohmann::json response;
					m_handler->OnToolCall(request, response);

					fprintf(stdout, "%s\n", response.dump().c_str());
					fflush(stdout);
				}
			}
		}
		catch (const nlohmann::json::parse_error& e)
		{
		}
	}
}

}
