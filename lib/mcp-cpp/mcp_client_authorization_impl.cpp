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

#include "mcp_client_authorization_impl.h"

namespace Mcp {

McpClientAuthorizationImpl::McpClientAuthorizationImpl()
	: m_redirect_port_no(-1)
{
}

McpClientAuthorizationImpl::~McpClientAuthorizationImpl()
{
}

void McpClientAuthorizationImpl::Reset()
{
	m_token = "";
}

bool McpClientAuthorizationImpl::GetServerMeta(const std::string& resource_meta_url)
{
	return true;
}

const std::string& McpClientAuthorizationImpl::GetAuthUrl() const
{
	return "";
}

bool McpClientAuthorizationImpl::DynamicRegistration(const std::string& client_name)
{
	return true;
}

bool McpClientAuthorizationImpl::Authorize(std::function <bool(const std::string& url)> open_browser)
{
	return true;
}

void McpClientAuthorizationImpl::SetAuthorizationCode(const std::string& code)
{
}

bool McpClientAuthorizationImpl::WaitToken()
{
	return true;
}

}
