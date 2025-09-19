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

#include "mcp_stdio_client_transport_impl.h"

#ifdef _WIN32
#include "platform/mcp_stdio_client_transport_impl_win32.h"
#else
#include "platform/mcp_stdio_client_transport_impl_posix.h"
#endif

namespace Mcp
{

std::unique_ptr<McpStdioClientTransport> McpStdioClientTransport::CreateInstance(const std::wstring& filepath)
{
#ifdef _WIN32
    return std::make_unique<McpStdioClientTransportImpl_Win32>(filepath);
#else
    return std::make_unique<McpStdioClientTransportImpl_Posix>(filepath);
#endif
}

McpStdioClientTransportImpl::McpStdioClientTransportImpl(const std::wstring& filepath)
	: m_filepath(filepath)
{
}

McpStdioClientTransportImpl::~McpStdioClientTransportImpl()
{
}

bool McpStdioClientTransportImpl::Initialize(const std::string& request, std::string& response)
{
    if (!OnCreateProcess())
    {
        return false;
    }

	if (!OnSendRequest(request, response))
	{
		return false;
	}

    return true;
}

void McpStdioClientTransportImpl::Shutdown()
{
    OnTerminateProcess();
}

bool McpStdioClientTransportImpl::SendRequest(const std::string& request, std::string& response)
{
    return OnSendRequest(request, response);
}

}
