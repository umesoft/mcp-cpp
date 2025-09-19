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

namespace Mcp
{

std::unique_ptr<McpStdioClientTransport> McpStdioClientTransport::CreateInstance(const std::wstring& filepath)
{
	return std::make_unique<McpStdioClientTransportImpl>(filepath);
}

McpStdioClientTransportImpl::McpStdioClientTransportImpl(const std::wstring& filepath)
	: m_filepath(filepath)
#ifdef _WIN32
    , m_hStdOutRead(NULL)
    , m_hStdInWrite(NULL)
    , m_hProcess(NULL)
#endif
{
}

McpStdioClientTransportImpl::~McpStdioClientTransportImpl()
{
}

bool McpStdioClientTransportImpl::Initialize(const std::string& request, std::string& response)
{
#ifdef _WIN32
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

    DWORD written;
    WriteFile(m_hStdInWrite, request.c_str(), (DWORD)request.size(), &written, NULL);
    WriteFile(m_hStdInWrite, "\n", 1, &written, NULL);
    FlushFileBuffers(m_hStdInWrite);

    char buffer[4096];
    DWORD read;
    ReadFile(m_hStdOutRead, buffer, sizeof(buffer) - 1, &read, NULL);
    buffer[read] = '\0';

    response = buffer;
#endif

	return true;
}

void McpStdioClientTransportImpl::Shutdown()
{
#ifdef _WIN32
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
#endif
}

bool McpStdioClientTransportImpl::SendRequest(const std::string& request, std::string& response)
{
#ifdef _WIN32
    DWORD written;
    WriteFile(m_hStdInWrite, request.c_str(), (DWORD)request.size(), &written, NULL);
    WriteFile(m_hStdInWrite, "\n", 1, &written, NULL);
    FlushFileBuffers(m_hStdInWrite);

    char buffer[4096];
    DWORD read;
    ReadFile(m_hStdOutRead, buffer, sizeof(buffer) - 1, &read, NULL);
    buffer[read] = '\0';

    response = buffer;
#endif

	return true;
}

}
