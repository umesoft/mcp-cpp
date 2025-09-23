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

class McpClientAuthorization
{
public:
	virtual ~McpClientAuthorization();

	void SetRedirectPortNo(int port_no);
	void SetRedirectUrl(const std::string& url);

	void SetClientInfo(const std::string& client_id, const std::string& client_secret);
	const std::string& GetClientId() const;
	const std::string& GetClientSecret() const;

	virtual void SetAuthorizationCode(const std::string& code) = 0;

protected:
	int m_redirect_port_no;
	std::string m_redirect_url;

	std::string m_client_id;
	std::string m_client_secret;

	McpClientAuthorization();
};

}
