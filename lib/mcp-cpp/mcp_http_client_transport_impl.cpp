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

#include "mcp_http_client_transport_impl.h"

#include <string.h>

namespace Mcp {

std::unique_ptr<McpHttpClientTransport> McpHttpClientTransport::CreateInstance(const std::string& host, const std::string& entry_point)
{
	return std::make_unique<McpHttpClientTransportImpl>(host, entry_point);
}

McpHttpClientTransportImpl::McpHttpClientTransportImpl(const std::string& host, const std::string& entry_point)
	: m_host(host)
	, m_entry_point(entry_point)
    , m_curl(nullptr)
{
	m_url = m_host + m_entry_point;
}

McpHttpClientTransportImpl::~McpHttpClientTransportImpl()
{
}

size_t McpHttpClientTransportImpl::HeaderCallback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    auto self = (McpHttpClientTransportImpl*)userdata;
    
    size_t totalSize = size * nmemb;
    self->m_header_buffer.append(ptr, totalSize);

    while (true)
    {
        int pos = self->m_header_buffer.find("\r\n");
        if (pos <= 0)
        {
            break;
        }

        std::string header_line = self->m_header_buffer.substr(0, pos);
        self->m_header_buffer = self->m_header_buffer.substr(pos + 2);
        pos = header_line.find(": ");
        if (pos > 0)
        {
            std::string key = header_line.substr(0, pos);
            std::string value = header_line.substr(pos + 2);
            self->m_headers[key] = value;
        }
    }

    return totalSize;
}

size_t McpHttpClientTransportImpl::WriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    auto self = (McpHttpClientTransportImpl*)userdata;

    size_t totalSize = size * nmemb;
    self->m_response_buffer.append(ptr, totalSize);

    while (true)
    {
        int pos = self->m_response_buffer.find("\n\n");
		if (pos <= 0)
		{
			break;
		}

        self->m_response = self->m_response_buffer.substr(0, pos);
		self->m_response_buffer = self->m_response_buffer.substr(pos + 2);
    }

    return totalSize;
}

bool McpHttpClientTransportImpl::Initialize(const std::string& request)
{
    m_curl = curl_easy_init();

    curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
    curl_easy_setopt(m_curl, CURLOPT_HEADERDATA, this);
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(m_curl, CURLOPT_TCP_KEEPALIVE, 1L);

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(m_curl, CURLOPT_URL, m_url.c_str());

    curl_easy_setopt(m_curl, CURLOPT_POST, 1L);
    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE_LARGE, request.length());
    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, request.c_str());

    m_headers.clear();
    m_header_buffer = "";
    m_response = "";
    m_response_buffer = "";

    CURLcode  res = curl_easy_perform(m_curl);
    if (res != CURLE_OK)
    {
        curl_slist_free_all(headers);
        return false;
    }

    curl_slist_free_all(headers);

	if (m_headers.find("mcp-session-id") != m_headers.end())
	{
		m_session_id = "mcp-session-id: " + m_headers["mcp-session-id"];
	}

    return true;
}

void McpHttpClientTransportImpl::Shutdown()
{
    if (m_curl != nullptr)
    {
        curl_easy_cleanup(m_curl);
        m_curl = nullptr;
    }
}

bool McpHttpClientTransportImpl::SendRequest(const std::string& request, std::string& response)
{
	if (m_curl == nullptr)
	{
		return false;
	}
    if (m_session_id.empty())
    {
        return false;
    }

	struct curl_slist* headers = NULL;
	headers = curl_slist_append(headers, "Content-Type: application/json");
	headers = curl_slist_append(headers, m_session_id.c_str());
	curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);

	curl_easy_setopt(m_curl, CURLOPT_URL, m_url.c_str());

	curl_easy_setopt(m_curl, CURLOPT_POST, 1L);
	curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE_LARGE, request.length());
	curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, request.c_str());

    m_headers.clear();
    m_header_buffer = "";
    m_response = "";
    m_response_buffer = "";

	CURLcode  res = curl_easy_perform(m_curl);
	if (res != CURLE_OK)
	{
        curl_slist_free_all(headers);
        return false;
	}

    response = m_response.substr(21);       // #TODO#

    curl_slist_free_all(headers);

    return true;
}

}
