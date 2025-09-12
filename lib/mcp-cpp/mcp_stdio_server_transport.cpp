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

	McpStdioServerTransport::McpStdioServerTransport()
	{
	}

	McpStdioServerTransport::~McpStdioServerTransport()
	{
	}

	void McpStdioServerTransport::OnOpen()
	{
		m_worker = std::thread([this]
		{
			while (1)
			{
				char buffer[4096];			// #TODO# 
				if (fgets(buffer, sizeof(buffer) - 1, stdin) == nullptr)
				{
					break;
				}

				{
					std::lock_guard<std::mutex> lock(m_mutex);
					m_queue.push(buffer);
				}

				m_cv.notify_one();
			}
		});
	}

	bool McpStdioServerTransport::RecvRequest()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		if (m_cv.wait_for(lock, std::chrono::milliseconds(100)) == std::cv_status::timeout) 
		{
			return true;
		}

		while (!m_queue.empty())
		{
			const std::string& request_str = m_queue.front();

			try
			{
				auto request = nlohmann::json::parse(request_str);
				if (request.contains("method"))
				{
					std::string method = request.at("method");
					if (method != "notifications/initialized")
					{
						nlohmann::json response;

						if (method == "initialize")
						{
							m_handler->OnInitialize(request, response);
						}
						else if (method == "logging/setLevel")
						{
							m_handler->OnLoggingSetLevel(request, response);
						}
						else if (method == "tools/list")
						{
							m_handler->OnToolsList(request, response);
						}
						else if (method == "tools/call")
						{
							m_handler->OnToolCall(request, response);
						}
						else
						{
							// #TODO#
						}

						fprintf(stdout, "%s\n", response.dump().c_str());
						fflush(stdout);
					}
				}
			}
			catch (const nlohmann::json::parse_error& e)
			{
				// #TODO#
			}

			m_queue.pop();
		}

		return true;
	}
}
