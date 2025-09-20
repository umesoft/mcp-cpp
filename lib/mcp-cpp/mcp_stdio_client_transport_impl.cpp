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

std::unique_ptr<McpStdioClientTransport> McpStdioClientTransport::CreateInstance(const std::wstring& filepath, int timeout)
{
#ifdef _WIN32
    return std::make_unique<McpStdioClientTransportImpl_Win32>(filepath, timeout);
#else
    return std::make_unique<McpStdioClientTransportImpl_Posix>(filepath, timeout);
#endif
}

McpStdioClientTransportImpl::McpStdioClientTransportImpl(const std::wstring& filepath, int timeout)
	: m_filepath(filepath)
	, m_timeout(timeout)
{
    const int REQUEST_BUFFER_SIZE = 128 * 1024;
    m_request_buffer.resize(REQUEST_BUFFER_SIZE);
}

McpStdioClientTransportImpl::~McpStdioClientTransportImpl()
{
}

bool McpStdioClientTransportImpl::Initialize(
    const std::string& request, 
    std::function <bool(const std::string& response)> callback
)
{
    if (!OnCreateProcess())
    {
        return false;
    }

    m_response_str.clear();

	if (!OnSendRequest(request, callback))
	{
		return false;
	}

    return true;
}

void McpStdioClientTransportImpl::Shutdown()
{
    OnTerminateProcess();
}

bool McpStdioClientTransportImpl::SendRequest(
    const std::string& request,
    std::function <bool(const std::string& response)> callback
)
{
    return OnSendRequest(request, callback);
}

bool McpStdioClientTransportImpl::SendNotification(const std::string& notification)
{
    return OnSendNotification(notification);
}

bool McpStdioClientTransportImpl::AppendResponse(char* response_buffer, int size, std::string& response_str)
{
    m_response_str.append(response_buffer, size);

    int pos = m_response_str.find("\n");
    if (pos <= 0)
    {
        return false;
    }

    response_str = m_response_str.substr(0, pos);
    m_response_str = m_response_str.substr(pos + 1);

	return true;
}

}
