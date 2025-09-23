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

bool McpClientAuthorizationImpl::DynamicRegistration(const std::string& client_name)
{
	return true;
}

bool McpClientAuthorizationImpl::Authorize(
	const std::string& resource_meta_url,
	const std::string& client_name,
	std::function <bool(const std::string& url)> open_browser
)
{
	if (!GetServerMeta(resource_meta_url))
	{
		return false;
	}

	if (m_client_id.empty())
	{
		if (!DynamicRegistration(client_name))
		{
			return false;
		}
	}

	if (m_redirect_url.empty())
	{
		// ...
	}

	std::string url;

	if (open_browser)
	{
		if (!open_browser(url))
		{
			return false;
		}
	}
	else
{
#ifdef _WIN32
		std::string cmd = "start \"\" \"" + url + "\"";
#else
		std::string cmd = "xdg-open " + url;
#endif

		std::system(cmd.c_str());
}

	if (!WaitToken())
{
		// ...
		return false;
	}

	// ...

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
