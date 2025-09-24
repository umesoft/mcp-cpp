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

#include "mcp-cpp/mcp_client_authorization.h"

namespace Mcp {

McpClientAuthorization::McpClientAuthorization()
	: m_redirect_port_no(-1)
	, m_timeout(60 * 1000)
{
}

McpClientAuthorization::~McpClientAuthorization()
{
}

void McpClientAuthorization::SetRedirectPortNo(int port_no)
{
	m_redirect_port_no = port_no;
	m_redirect_url.clear();
}

void McpClientAuthorization::SetRedirectUrl(const std::string& url)
{
	m_redirect_url = url;
	m_redirect_port_no = -1;
}

void McpClientAuthorization::SetClientInfo(const std::string& client_id, const std::string& client_secret)
{
	m_client_id = client_id;
	m_client_secret = client_secret;
}

const std::string& McpClientAuthorization::GetClientId() const
{
	return m_client_id;
}

const std::string& McpClientAuthorization::GetClientSecret() const
{
	return m_client_secret;
}

void McpClientAuthorization::SetTimeout(int timeout)
{
	m_timeout = timeout;
}

}
