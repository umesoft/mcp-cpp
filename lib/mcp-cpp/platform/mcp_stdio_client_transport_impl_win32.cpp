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

#include "mcp_stdio_client_transport_impl_win32.h"

namespace Mcp
{

McpStdioClientTransportImpl_Win32::McpStdioClientTransportImpl_Win32(const std::wstring& filepath, int timeout)
	: McpStdioClientTransportImpl(filepath, timeout)
    , m_hStdOutRead(NULL)
    , m_hStdInWrite(NULL)
    , m_hProcess(NULL)
{
}

McpStdioClientTransportImpl_Win32::~McpStdioClientTransportImpl_Win32()
{
}

bool McpStdioClientTransportImpl_Win32::OnCreateProcess()
{
    SECURITY_ATTRIBUTES saAttr{};
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE hStdInRead = NULL;
    HANDLE hStdOutWrite = NULL;

    if (!CreatePipe(&m_hStdOutRead, &hStdOutWrite, &saAttr, 0)) {
        Shutdown();
        return false;
    }
    SetHandleInformation(m_hStdOutRead, HANDLE_FLAG_INHERIT, 0);

    if (!CreatePipe(&hStdInRead, &m_hStdInWrite, &saAttr, 0)) {
        Shutdown();
        return false;
    }
    SetHandleInformation(m_hStdInWrite, HANDLE_FLAG_INHERIT, 0);

    PROCESS_INFORMATION pi{};
    STARTUPINFO si{};
    si.cb = sizeof(STARTUPINFO);
    si.hStdOutput = hStdOutWrite;
    si.hStdInput = hStdInRead;
    si.dwFlags |= STARTF_USESTDHANDLES;

    if (!CreateProcess(
        NULL,
        m_filepath.data(),
        NULL,
        NULL,
        TRUE,
        CREATE_NEW_CONSOLE,
        NULL,
        NULL,
        &si,
        &pi))
    {
        Shutdown();
        return false;
    }

    CloseHandle(hStdOutWrite);
    CloseHandle(hStdInRead);

    CloseHandle(pi.hThread);

    m_hProcess = pi.hProcess;

    return true;
}

void McpStdioClientTransportImpl_Win32::OnTerminateProcess()
{
    if (m_hStdInWrite != NULL)
    {
        CloseHandle(m_hStdInWrite);
        m_hStdInWrite = NULL;
    }

    if (m_hProcess != NULL)
    {
        if (WaitForSingleObject(m_hProcess, 500) != WAIT_OBJECT_0)
        {
            TerminateProcess(m_hProcess, 0);
        }

        CloseHandle(m_hProcess);
        m_hProcess = NULL;
    }

    if (m_hStdOutRead != NULL)
    {
        CloseHandle(m_hStdOutRead);
        m_hStdOutRead = NULL;
    }
}

bool McpStdioClientTransportImpl_Win32::OnSendRequest(
    const std::string& request,
    std::function <bool(const std::string& response)> callback
)
{
    DWORD written;
    WriteFile(m_hStdInWrite, request.c_str(), (DWORD)request.size(), &written, NULL);
    WriteFile(m_hStdInWrite, "\n", 1, &written, NULL);

    while (true)
    {
        DWORD read = 0;
        if (!ReadFile(m_hStdOutRead, &m_request_buffer[0], m_request_buffer.size(), &read, NULL))
        {
            return false;
        }

		std::string response_str;
        if (AppendResponse(&m_request_buffer[0], read, response_str))
        {
            if (callback(response_str))
            {
                break;
            }
        }
    }

	return true;
}

bool McpStdioClientTransportImpl_Win32::OnSendNotification(const std::string& notification)
{
    DWORD written;
    WriteFile(m_hStdInWrite, notification.c_str(), (DWORD)notification.size(), &written, NULL);
    WriteFile(m_hStdInWrite, "\n", 1, &written, NULL);

    return true;
}

}
