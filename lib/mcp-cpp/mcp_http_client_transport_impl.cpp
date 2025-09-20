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
#include "mcp_common.h"

namespace Mcp {

std::unique_ptr<McpHttpClientTransport> McpHttpClientTransport::CreateInstance(
    const std::string& host, 
    const std::string& entry_point,
    const std::string& token,
    std::function <void(const std::string& url, std::string& token)> auth_callback
)
{
	return std::make_unique<McpHttpClientTransportImpl>(host, entry_point, token, auth_callback);
}

McpHttpClientTransportImpl::McpHttpClientTransportImpl(
    const std::string& host, 
    const std::string& entry_point,
    const std::string& token,
    std::function <void(const std::string& auth_url, std::string& token)> auth_callback
)
	: m_host(host)
	, m_entry_point(entry_point)
	, m_auth_callback(auth_callback)
    , m_curl(nullptr)
{
	m_url = m_host + m_entry_point;

    UpdateAuthorization(token);
}

McpHttpClientTransportImpl::~McpHttpClientTransportImpl()
{
}

void McpHttpClientTransportImpl::UpdateAuthorization(const std::string& token)
{
    m_authorization = token.empty() ? "" : "Authorization: Bearer " + token;
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
            string_to_lower(key);

            std::string value = header_line.substr(pos + 2);
            self->m_headers[key] = string_trim(value);
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

bool McpHttpClientTransportImpl::Initialize(const std::string& request, std::string& response)
{
    m_curl = curl_easy_init();

    curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
    curl_easy_setopt(m_curl, CURLOPT_HEADERDATA, this);
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(m_curl, CURLOPT_TCP_KEEPALIVE, 1L);

    m_mcp_session_id = "";

    int status_code = 0;
    if (!Send(request, response, status_code))
    {
        if (status_code == 401)
        {
            m_authorization = "";

            auto it = m_headers.find("www-authenticate");
            if (it != m_headers.end())
            {
                std::string www_authenticate = it->second;
                auto url = ExtractResourceMetadata(www_authenticate);
                if (!url.has_value())
                {
                    return false;
                }

                std::string token = "";

                if (m_auth_callback != nullptr)
                {
                    m_auth_callback(*url, token);
                }
                else
                {
                    Authenticate(*url, token);
                }

                if (token.empty())
                {
                    return false;
                }

                UpdateAuthorization(token);

                status_code = 0;
                if (!Send(request, response, status_code))
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }
		else
		{
			return false;
		}
    }

    auto it = m_headers.find("mcp-session-id");
    if (it != m_headers.end())
	{
        m_mcp_session_id = "Mcp-Session-Id: " + it->second;
	}
    else
    {
        return false;
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

    m_mcp_session_id = "";
}

bool McpHttpClientTransportImpl::SendRequest(const std::string& request, std::string& response)
{
	int status_code = 0;
    return Send(request, response, status_code);
}

bool McpHttpClientTransportImpl::SendNotification(const std::string& notification)
{
	std::string response;
    int status_code = 0;
    return Send(notification, response, status_code);
}

bool McpHttpClientTransportImpl::Send(const std::string& request, std::string& response, int& status_code)
{
    if (m_curl == nullptr)
    {
        return false;
    }
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    if (!m_authorization.empty())
    {
        headers = curl_slist_append(headers, m_authorization.c_str());
    }
    if (!m_mcp_session_id.empty())
    {
        headers = curl_slist_append(headers, m_mcp_session_id.c_str());
    }

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

    curl_slist_free_all(headers);

    if (res != CURLE_OK)
    {
        return false;
    }

    long http_code;
    res = curl_easy_getinfo(m_curl, CURLINFO_HTTP_CODE, &http_code);
    if (res != CURLE_OK)
    {
        return false;
    }

    status_code = (int)http_code;

    if (status_code != 200)
    {
        return false;
    }

    const std::string prefix = "event: message\ndata: ";
    if (m_response.rfind(prefix, 0) != 0) {
        return false;
    }
    response = m_response.substr(prefix.size());

    return true;
}

std::optional<std::string> McpHttpClientTransportImpl::ExtractResourceMetadata(const std::string& header)
{
    const std::string prefix = "Bearer ";
    if (header.rfind(prefix, 0) != 0) {
        return std::nullopt;
    }

    const std::string key = "resource_metadata=\"";
    size_t start = header.find(key, prefix.size());
    if (start == std::string::npos) {
        return std::nullopt;
    }

    start += key.size();
    size_t end = header.find('"', start);
    if (end == std::string::npos) {
        return std::nullopt;
    }

    return header.substr(start, end - start);
}

void McpHttpClientTransportImpl::Authenticate(const std::string& url, std::string& token)
{
    // retrive oauth-protected-resource
    // ...

    // register dynamic client
    // ...

    // create callback http server
    // ...

	// open authorization url in browser
    // ...

	// wait for token
    // ...
}

}
