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

#pragma once

#include "mcp_server_transport.h"

#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace Mcp {

class McpStdioServerTransport : public McpServerTransport {
public:
	McpStdioServerTransport(int max_request_size = 128 * 1024);
	virtual ~McpStdioServerTransport();

private:
	int m_max_request_size;
	char* m_request_buffer;

	std::thread m_request_worker;
	std::queue<std::string> m_request_queue;
	std::mutex m_request_mutex;
	std::condition_variable m_request_cv;

	virtual void OnOpen();
	virtual bool OnProcRequest();
	virtual void OnSendResponse(const std::string& session_id, const std::string& response_str, bool is_finish);
};

}
