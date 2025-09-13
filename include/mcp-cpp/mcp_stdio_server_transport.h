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
	McpStdioServerTransport();
	virtual ~McpStdioServerTransport();

protected:
	virtual void OnOpen();
	virtual bool OnProcRequest();
	virtual void OnSendNotification(const std::string& notification_str);

private:
	std::queue<std::string> m_queue;
	std::mutex m_mutex;
	std::condition_variable m_cv;
	std::thread m_worker;
	char* m_buffer;
	std::mutex m_send_mutex;
};

}
