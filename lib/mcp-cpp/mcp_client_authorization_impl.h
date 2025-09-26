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

#include "mcp-cpp/mcp_client_authorization.h"

#include <curl/curl.h>

namespace Mcp {

class McpClientAuthorizationImpl : public McpClientAuthorization
{
public:
	McpClientAuthorizationImpl();
	virtual ~McpClientAuthorizationImpl();

	void Reset();

	bool Authorize(
		const std::string& resource_meta_url,
		const std::string& client_name,
		std::function <bool(const std::string& url)> open_browser = nullptr
	);

	virtual void SetAuthorizationCode(const std::string& code);

	const std::string& GetToken() const { return m_token; }

private:
	nlohmann::json m_protected_resource;
	std::string m_authorization_server;
	nlohmann::json m_resource_meta_data;
	std::string m_scope;
	std::string m_code_verifier;
	std::string m_code_challenge;
	std::string m_code;

	std::string m_token;

	void* m_mgr;
	bool m_is_running;
	std::unique_ptr<std::thread> m_callback_worker;

	std::mutex m_code_mutex;
	std::condition_variable m_code_cv;

	static void cbEvHander(void* connection, int event_code, void* event_data);

	bool OpenCallbackServer();
	void CloseCallbackServer();

	bool GetOAuthProtectedResource(const std::string& resource_url);
	bool GetResourceMetaData();
	bool DynamicRegistration(const std::string& client_name);
	bool WaitToken();
	bool RequestToken();

	std::string GetRedirectUrl();
	std::string GetAuthUrl();
	void GeneratePKCE();

	static size_t WriteCallback(char* ptr, size_t size, size_t nmemb, void* responseData);
};

}
