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

McpStdioServerTransport::McpStdioServerTransport(int max_request_size)
	: m_max_request_size(max_request_size)
{
	m_request_buffer = new char[m_max_request_size];
}

McpStdioServerTransport::~McpStdioServerTransport()
{
	delete[] m_request_buffer;
}

void McpStdioServerTransport::OnOpen()
{
	m_request_worker = std::thread([this]
	{
		while (true)
		{
			if (fgets(m_request_buffer, m_max_request_size, stdin) == nullptr)
			{
				break;
			}

			int pos = strlen(m_request_buffer);
			if (m_request_buffer[pos - 1] == '\n')
			{
				{
					std::lock_guard<std::mutex> lock(m_request_mutex);
					m_request_queue.push(m_request_buffer);
				}
				m_request_cv.notify_one();
			}
		}
	});
}

bool McpStdioServerTransport::OnProcRequest()
{
	std::unique_lock<std::mutex> lock(m_request_mutex);
	if (m_request_cv.wait_for(lock, std::chrono::milliseconds(100)) == std::cv_status::timeout)
	{
		return true;
	}

	while (!m_request_queue.empty())
	{
		const std::string& request_str = m_request_queue.front();
		m_handler->OnRecv("", request_str);
		m_request_queue.pop();
	}

	return true;
}

void McpStdioServerTransport::OnSendResponse(const std::string& session_id, const std::string& response_str, bool is_finish)
{
	WriteResponse(response_str);
}

void McpStdioServerTransport::WriteResponse(const std::string& notification_str)
{
	std::lock_guard<std::mutex> lock(m_response_mutex);
	fprintf(stdout, "%s\n", notification_str.c_str());
	fflush(stdout);
}

}
