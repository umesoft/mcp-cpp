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

#include <functional>
#include <map>
#include <vector>

namespace Mcp {

class McpHttpServerTransport : public McpServerTransport {
public:
	McpHttpServerTransport();
	virtual ~McpHttpServerTransport();

	void SetTls(
		const char* cert_file,
		const char* key_file
	);
	void SetAuthorization(
		const char* authorization_servers,
		const char* scopes_supported
	);

	void SetEntryPoint(const std::string& url, unsigned long long session_timeout);

	virtual void OnOpen();
	virtual void OnClose();

	virtual bool OnProcRequest();

private:
	bool m_use_tls;
	std::string m_cert_file;
	std::string m_key_file;
	bool m_use_authorization;
	std::string m_authorization_servers;
	std::string m_scopes_supported;
	std::string m_url;
	std::string m_host;
	std::string m_entry_point;

	unsigned long long m_session_timeout;

	std::map<std::string, long> m_sessions;

	bool IsEnableSessionId(std::string session_id);
	void EraseSession(std::string session_id);
	void ClearSession();

	void* m_mgr;
	void* m_timer;

	static void cbEvHander(void* connection, int event_code, void* event_data);
	static void cbTimerHandler(void* timer_data);
};

}
