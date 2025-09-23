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

namespace mcp {

class McpClientAuthorization
{
public:
	std::unique_ptr<McpClientAuthorization> CreateInstance();

	virtual ~McpClientAuthorization() {}
	
	virtual bool GetServerMeta(const std::string& resource_meta_url) = 0;
	
	virtual bool CreateLocalRegidrect(int port_no) = 0;
	virtual void CloseLocalRegidrect() = 0;
	virtual void SetRedirect(const std::string& redirect_url) = 0;

	virtual bool DynamicRegistration(const std::string& client_name, std::string& client_id, std::string& client_secret) = 0;
	virtual void SetRegistration(const std::string& client_id, const std::string& client_secret) = 0;

	virtual std::string Authorize(const std::string& scope, std::function <bool(const std::string& url)> open_browser = nullptr) = 0;

	virtual std::string RequestToken(const std::string& code) = 0;

protected:
	McpClientAuthorization() {}
};

}
