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

#include "mcp-cpp/mcp_server_transport.h"

namespace Mcp {

McpServerTransport::McpServerTransport()
	: m_handler(nullptr)
{
}

McpServerTransport::~McpServerTransport()
{
}

bool McpServerTransport::Open(Handler* handler)
{
	m_handler = handler;

	if (!OnOpen())
	{
		m_handler = nullptr;
		return false;
	}

	return true;
}

void McpServerTransport::Close()
{
	OnClose();

	m_handler = nullptr;
}

bool McpServerTransport::ProcRequest()
{
	return OnProcRequest();
}

void McpServerTransport::SendResponse(const std::string& session_id, const std::string& response_str, bool is_finish)
{
	OnSendResponse(session_id, response_str, is_finish);
}

}
