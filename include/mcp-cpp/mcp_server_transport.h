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

#include "mcp_type.h"

namespace Mcp {

class McpServerTransport {
public:
	class Handler {
	public:
		virtual ~Handler() {}

		virtual void OnClose(const std::string& session_id) = 0;
		virtual bool OnRecv(const std::string& session_id, const std::string& request_str) = 0;
	};

	virtual ~McpServerTransport();

	void Open(Handler* handler);
	void Close();
	bool ProcRequest();
	void SendResponse(const std::string& session_id, const std::string& response_str, bool is_finish = true);

protected:
	Handler* m_handler;

	McpServerTransport();

private:
	virtual void OnOpen() {};
	virtual void OnClose() {};
	virtual bool OnProcRequest() { return true; };
	virtual void OnSendResponse(const std::string& session_id, const std::string& response_str, bool is_finish) {};
};

}
