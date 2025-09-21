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

#ifdef _WIN32
const std::string NEW_LINE = "\r\n";
#else
const std::string NEW_LINE = "\n";
#endif

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
	, m_is_runnning(false)
	, m_timeout(timeout)
{
}

McpStdioClientTransportImpl::~McpStdioClientTransportImpl()
{
}

bool McpStdioClientTransportImpl::Initialize(
    const std::string& request, 
    std::function <bool(const std::string& response)> callback
)
{
    if (!OnCreateProcess(m_filepath))
    {
        return false;
    }

	m_is_runnning = true;

	m_response_worker = std::thread([this]
	{
		const int REQUEST_BUFFER_SIZE = 128 * 1024;
		std::vector<char> buffer(REQUEST_BUFFER_SIZE);

		std::string response_str;

		while (m_is_runnning)
		{
			int size = OnRecv(buffer);
			if (size < 0)
			{
				break;
			}

			if (0 < size)
			{
				response_str.append(&buffer[0], size);

				int pos = response_str.find(NEW_LINE);
				if (0 <= pos)
				{
					std::lock_guard<std::mutex> lock(m_response_mutex);

					m_response_queue.push(response_str.substr(0, pos));
					m_response_cv.notify_one();

					response_str = response_str.substr(pos + NEW_LINE.length());
				}
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
			}
		}
	});

	if (!OnSend(request + NEW_LINE))
	{
		return false;
	}

	if (!WaitResponse(callback))
	{
		return false;
	}

    return true;
}

void McpStdioClientTransportImpl::Shutdown()
{
	m_is_runnning = false;
	m_response_cv.notify_one();
	m_response_worker.join();

	ClearResponse();

    OnTerminateProcess();
}

bool McpStdioClientTransportImpl::SendRequest(
    const std::string& request,
    std::function <bool(const std::string& response)> callback
)
{
	if (!OnSend(request + NEW_LINE))
	{
		return false;
	}

	if (!WaitResponse(callback))
	{
		return false;
	}

	return true;
}

bool McpStdioClientTransportImpl::SendNotification(const std::string& notification)
{
	if (!OnSend(notification + NEW_LINE))
	{
		return false;
	}

	return true;
}

bool McpStdioClientTransportImpl::WaitResponse(std::function <bool(const std::string& response)> callback)
{
	auto start = std::chrono::steady_clock::now();

	while(m_is_runnning)
	{
		std::unique_lock<std::mutex> lock(m_response_mutex);

		if (m_response_queue.empty())
		{
			m_response_cv.wait_for(lock, std::chrono::milliseconds(50));
		}

		if (!m_response_queue.empty())
		{
			const std::string& response_str = m_response_queue.front();
			if (callback(response_str))
			{
				m_response_queue.pop();
				break;
			}
			m_response_queue.pop();
		}

		auto now = std::chrono::steady_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
		if (elapsed >= m_timeout)
		{
			return false;
		}
	}

	return true;
}

void McpStdioClientTransportImpl::ClearResponse()
{
	std::lock_guard<std::mutex> lock(m_response_mutex);

	while (!m_response_queue.empty())
	{
		m_response_queue.pop();
	}
}

}
