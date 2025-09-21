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

#include <sys/select.h>
#include <sys/wait.h>
#include <unistd.h>

namespace Mcp
{

McpStdioClientTransportImpl_Posix::McpStdioClientTransportImpl_Posix(const std::wstring& filepath, int timeout)
	: McpStdioClientTransportImpl(filepath, timeout)
    , m_child_pid(-1)
    , m_stdin_fd(-1)
    , m_stdout_fd(-1)
{
}

McpStdioClientTransportImpl_Posix::~McpStdioClientTransportImpl_Posix()
{
}

bool McpStdioClientTransportImpl_Win32::OnCreateProcess(const std::wstring& filepath)
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
            std::wstring ws = filepath;
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

	return true;
}

void McpStdioClientTransportImpl_Posix::OnTerminateProcess()
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

bool McpStdioClientTransportImpl_Posix::OnSend(const std::string& request)
{
    size_t total = 0;
    const char* data = request.c_str();
    size_t len = request.size();

    while (total < len)
    {
        ssize_t written = write(m_stdin_fd, data + total, len - total);
        if (written < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return false;
        }
        if (written == 0)
        {
            return false;
        }
        total += (size_t)written;
    }
    
    return true;
}

ssize_t McpStdioClientTransportImpl_Posix::OnRecv(std::vector<char>& buffer)
{
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(m_stdout_fd, &rfds);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100 * 1000;

    int ret = select(m_stdout_fd + 1, &rfds, NULL, NULL, &tv);
    if (ret == -1)
    {
        return -1;
    }
	else if (ret == 0)
    {
        return 0;
    }

    ssize_t read_size = read(m_stdout_fd, &buffer[0], buffer.size());
    if (read_size <= 0)
    {
        return -1;
    }
    return read_size;
}

}
