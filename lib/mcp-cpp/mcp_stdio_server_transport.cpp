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

const int max_request_size = 128 * 1024;

McpStdioServerTransport::McpStdioServerTransport()
{
	m_buffer = new char[max_request_size];
}

McpStdioServerTransport::~McpStdioServerTransport()
{
	delete[] m_buffer;
}

void McpStdioServerTransport::OnOpen()
{
	m_worker = std::thread([this]
	{
		while (1)
		{
			if (fgets(m_buffer, max_request_size, stdin) == nullptr)
			{
				break;
			}

			int pos = strlen(m_buffer);
			if (m_buffer[pos - 1] == '\n')
			{
				{
					std::lock_guard<std::mutex> lock(m_mutex);
					m_queue.push(m_buffer);
				}
				m_cv.notify_one();
			}
		}
	});
}

bool McpStdioServerTransport::OnProcRequest()
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

			nlohmann::json response;
			if (m_handler->OnRecv(request, response))
			{
				std::lock_guard<std::mutex> lock(m_send_mutex);
				fprintf(stdout, "%s\n", response.dump().c_str());
				fflush(stdout);
			}
		}
		catch (const nlohmann::json::parse_error& e)
		{
			std::lock_guard<std::mutex> lock(m_send_mutex);
			fprintf(stderr, "Error: Invalid message\n");
			fflush(stderr);
		}

		m_queue.pop();
	}

	return true;
}

void McpStdioServerTransport::OnSendNotification(const nlohmann::json& notification)
{
	std::lock_guard<std::mutex> lock(m_send_mutex);
	fprintf(stdout, "%s\n", notification.dump().c_str());
	fflush(stdout);
}

}
