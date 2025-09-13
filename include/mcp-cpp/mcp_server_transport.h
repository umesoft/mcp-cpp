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

#include "nlohmann/json.hpp"

namespace Mcp {

class McpServerTransport {
public:
	class Handler {
	public:
		virtual ~Handler() {}

		virtual bool OnRecv(const std::string& request_str, std::string& response_str) = 0;

		virtual void OnInitialize(const nlohmann::json& request, nlohmann::json& response) = 0;
		virtual void OnLoggingSetLevel(const nlohmann::json& request, nlohmann::json& response) = 0;
		virtual void OnToolsList(const nlohmann::json& request, nlohmann::json& response) = 0;
		virtual void OnToolCall(const nlohmann::json& request, nlohmann::json& response) = 0;
	};

public:
	McpServerTransport();
	virtual ~McpServerTransport();

	void Open(Handler* handler);
	void Close();

	bool ProcRequest();
	void SendNotification(const std::string& notification_str);

protected:
	Handler* m_handler;

	virtual void OnOpen() {};
	virtual void OnClose() {};
	virtual bool OnProcRequest() { return true; };
	virtual void OnSendNotification(const std::string& notification_str) {};
};

}
