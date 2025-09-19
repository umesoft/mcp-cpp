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

#include "../mcp_stdio_client_transport_impl.h"

#include <Windows.h>

namespace Mcp {

class McpStdioClientTransportImpl_Win32 : public McpStdioClientTransportImpl {
public:
	McpStdioClientTransportImpl_Win32(const std::wstring& filepath);
	virtual ~McpStdioClientTransportImpl_Win32();

	virtual bool Initialize(const std::string& request, std::string& response);
	virtual void Shutdown();
	virtual bool SendRequest(const std::string& request, std::string& response);

private:
	HANDLE m_hStdOutRead;
	HANDLE m_hStdInWrite;
	HANDLE m_hProcess;
};

}
