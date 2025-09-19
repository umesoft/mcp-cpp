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

#include "mcp_stdio_client_transport_impl_posix.h"

#include <sys/wait.h>

namespace Mcp
{

McpStdioClientTransportImpl_Posix::McpStdioClientTransportImpl_Posix(const std::wstring& filepath)
	: m_filepath(filepath)
    , m_child_pid(-1)
    , m_stdin_fd(-1)
    , m_stdout_fd(-1)
{
}

McpStdioClientTransportImpl_Posix::~McpStdioClientTransportImpl_Posix()
{
}

bool McpStdioClientTransportImpl_Posix::Initialize(const std::string& request, std::string& response)
{
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
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        close(stdin_pipe[0]);
        close(stdout_pipe[1]);

        std::string exe_path;
        {
            std::wstring ws = m_filepath;
            std::vector<char> buf(ws.size() * 4 + 1);
            std::wcstombs(buf.data(), ws.c_str(), buf.size());
            exe_path = buf.data();
        }

        execl(exe_path.c_str(), exe_path.c_str(), nullptr);
        _exit(127);
    }

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

	return true;
}

void McpStdioClientTransportImpl_Posix::Shutdown()
{
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
}

bool McpStdioClientTransportImpl_Posix::SendRequest(const std::string& request, std::string& response)
{
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

	return true;
}

}
