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

#ifndef _WIN32
#include <sys/wait.h>
#endif

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
#else
    , m_child_pid(-1)
    , m_stdin_fd(-1)
    , m_stdout_fd(-1)
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
#else
    int stdin_pipe[2];
    int stdout_pipe[2];
    if (pipe(stdin_pipe) == -1 || pipe(stdout_pipe) == -1) 
    {
        Shutdown();
        return false;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        Shutdown();
        return false;
    }

    if (pid == 0)
    {
        // 子プロセス
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        close(stdin_pipe[0]);
        close(stdout_pipe[1]);

        // ワイド文字列からUTF-8へ変換
        std::string exe_path;
        {
            std::wstring ws = m_filepath;
            std::vector<char> buf(ws.size() * 4 + 1);
            std::wcstombs(buf.data(), ws.c_str(), buf.size());
            exe_path = buf.data();
        }

        execl(exe_path.c_str(), exe_path.c_str(), nullptr);
        _exit(127); // exec失敗
    }

    // 親プロセス
    close(stdin_pipe[0]);
    close(stdout_pipe[1]);
    m_child_pid = pid;
    m_stdin_fd = stdin_pipe[1];
    m_stdout_fd = stdout_pipe[0];

    write(m_stdin_fd, request.c_str(), request.size());
    write(m_stdin_fd, "\n", 1);

    char buffer[4096];
    ssize_t read_size = read(m_stdout_fd, buffer, sizeof(buffer) - 1);
    if (read_size <= 0)
    {
        return false;
    }

    buffer[read_size] = '\0';
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
#else
    if (m_stdin_fd != -1)
    {
        close(m_stdin_fd);
        m_stdin_fd = -1;
    }
    
    if (m_child_pid > 0)
    {
        kill(m_child_pid, SIGTERM);

        sleep(1);

        int status = 0;
        if (waitpid(m_child_pid, &status, WNOHANG) != m_child_pid)
        {
            kill(m_child_pid, SIGKILL);
        }

        m_child_pid = -1;
    }

    if (m_stdout_fd != -1)
    {
        close(m_stdout_fd);
        m_stdout_fd = -1;
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
#else
    if (m_stdin_fd == -1 || m_stdout_fd == -1)
        return false;

    write(m_stdin_fd, request.c_str(), request.size());
    write(m_stdin_fd, "\n", 1);

    char buffer[4096];
    ssize_t read_size = read(m_stdout_fd, buffer, sizeof(buffer) - 1);
    if (read_size <=0 )
    {
        return false;
    }

    buffer[read_size] = '\0';
    response = buffer;
#endif

	return true;
}

}
