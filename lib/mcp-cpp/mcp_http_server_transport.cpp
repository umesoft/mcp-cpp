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

#define NOMINMAX
#include "platform/platform.h"

#include "mcp-cpp/mcp_http_server_transport.h"
#include "jwt-cpp/jwt.h"

#include "mongoose.c"

namespace Mcp {

typedef void (*mg_timer_handler_t)(void*);

McpHttpServerTransport::McpHttpServerTransport()
	: m_use_tls(false)
	, m_cert_file()
	, m_key_file()
	, m_use_authorization(false)
	, m_authorization_servers()
	, m_scopes_supported()
	, m_url()
	, m_host()
	, m_entry_point()
	, m_sessions()
	, m_mgr(nullptr)
	, m_timer(nullptr)
{
}

McpHttpServerTransport::~McpHttpServerTransport()
{
}

void McpHttpServerTransport::SetTls(
	const char* cert_file,
	const char* key_file
)
{
	m_cert_file = cert_file;
	m_key_file = key_file;

	if (!m_cert_file.empty() && !m_key_file.empty())
	{
		m_use_tls = true;
	}
	else
	{
		m_use_tls = false;
	}
}

void McpHttpServerTransport::SetAuthorization(const char* authorization_servers, const char* scopes_supported)
{
	m_authorization_servers = authorization_servers;
	m_scopes_supported = scopes_supported;

	if (!m_authorization_servers.empty() && !m_scopes_supported.empty())
	{
		m_use_authorization = true;
	}
	else
	{
		m_use_authorization = false;
	}
}

void McpHttpServerTransport::SetEntryPoint(const std::string& url, unsigned long long session_timeout)
{
	if (m_use_tls)
	{
		m_url = "https://";
	}
	else
	{
		m_url = "http://";
	}
	m_url += url;

	std::string::size_type pos = url.find('/');
	if (pos == std::string::npos)
	{
		m_host = url;
		m_entry_point = "/";
	}
	else
	{
		m_host = url.substr(0, pos);
		m_entry_point = url.substr(pos);
	}

	m_session_timeout = session_timeout;
}

void McpHttpServerTransport::OnOpen()
{
	mg_mgr* s_mgr = nullptr;
	s_mgr = new mg_mgr();
	mg_mgr_init(s_mgr);

	mg_timer* s_timer = nullptr;
	s_timer = new mg_timer();
	mg_timer_init(&s_mgr->timers, s_timer, m_session_timeout, MG_TIMER_REPEAT, (mg_timer_handler_t)McpHttpServerTransport::cbTimerHandler, this);

	m_mgr = s_mgr;
	m_timer = s_timer;

	mg_http_listen(
		s_mgr,
		m_host.c_str(),
		(mg_event_handler_t)cbEvHander,
		this
	);
}

void McpHttpServerTransport::OnClose()
{
	mg_mgr* s_mgr = (mg_mgr*)m_mgr;
	mg_timer* s_timer = (mg_timer*)m_timer;

	mg_timer_free(&s_mgr->timers, s_timer);
	m_timer = nullptr;

	mg_mgr_free(s_mgr);
	m_mgr = nullptr;
}

bool McpHttpServerTransport::OnProcRequest()
{
	mg_mgr* s_mgr = (mg_mgr*)m_mgr;

	mg_mgr_poll(s_mgr, 1000);

	return true;
}

void McpHttpServerTransport::cbEvHander(void* connection, int event_code, void* event_data)
{
	mg_connection* conn = (mg_connection*)connection;
	McpHttpServerTransport* self = (McpHttpServerTransport*)conn->fn_data;

	if (event_code == MG_EV_ACCEPT)
	{
		if (self->m_use_tls)
		{
			struct mg_tls_opts opts =
			{
				.cert = mg_file_read(&mg_fs_posix, self->m_cert_file.c_str()),
				.key = mg_file_read(&mg_fs_posix, self->m_key_file.c_str())
			};
			mg_tls_init(conn, &opts);
		}
	}
	else if (event_code == MG_EV_HTTP_MSG)
	{
		struct mg_http_message* hm = (struct mg_http_message*)event_data;
		if (mg_match(hm->uri, mg_str(self->m_entry_point.data()), NULL))
		{
			std::string auth_token = "";
			std::string session_id = "";

			mg_str authorization = mg_str_s("authorization");
			mg_str mcp_session_id = mg_str_s("mcp-session-id");
			for (int i = 0; i < MG_MAX_HTTP_HEADERS; i++)
			{
				if (hm->headers[i].name.buf == nullptr)
				{
					break;
				}
				if (mg_strcasecmp(hm->headers[i].name, authorization) == 0)
				{
					auth_token.assign(hm->headers[i].value.buf, hm->headers[i].value.len);
				}
				else if (mg_strcasecmp(hm->headers[i].name, mcp_session_id) == 0)
				{
					session_id.assign(hm->headers[i].value.buf, hm->headers[i].value.len);
				}
			}

			if (mg_strcasecmp(hm->method, mg_str("DELETE")) == 0)
			{
				mg_http_reply(conn, 200, "", "");
				self->EraseSession(session_id);
				return;
			}
			else if (mg_strcasecmp(hm->method, mg_str("GET")) == 0)
			{
				mg_http_reply(conn, 405, "", "");
				return;
			}
			else if (mg_strcasecmp(hm->method, mg_str("POST")) == 0)
			{
				if (self->m_use_authorization)
				{
					bool authorization_chk = false;

					if (!auth_token.empty())
					{
						std::string::size_type aPos = auth_token.find_first_of("Bearer ");
						if (aPos != std::string::npos)
						{
							std::string token = auth_token.substr(aPos + 7);
							auto decoded = jwt::decode(token);
							auto payload = decoded.get_payload_json();

							std::string aud_value = payload["aud"].get<std::string>();
							if (aud_value == self->m_url)
							{
								authorization_chk = true;
							}
						}
					}

					if (!authorization_chk)
					{
						std::string authenticate_header = "WWW-Authenticate: Bearer resource_metadata=\"";
						authenticate_header += self->m_host;
						authenticate_header += "/.well-known/oauth-protected-resource";
						authenticate_header += self->m_entry_point;
						authenticate_header += "\"\r\n";
						mg_http_reply(
							conn,
							401,
							authenticate_header.c_str(),
							""
						);
						return;
					}
				}

				char* method = mg_json_get_str(hm->body, "$.method");
				if (method != nullptr)
				{
					if (strcmp(method, "initialize") == 0)
					{
						session_id = CreateSessionId();
					}
					else
					{
						if (!self->IsEnableSessionId(session_id))
						{
							mg_http_reply(conn, 400, "", "");
							return;
						}
					}
					self->m_sessions[session_id] = 1;

					std::string request_str = std::string(hm->body.buf, hm->body.buf + hm->body.len);
					std::string response_str;
					if (self->m_handler->OnRecv(request_str, response_str))
					{
						std::string headers = "Content-Type: text/event-stream\r\nmcp-session-id: " + session_id + "\r\n";
						mg_http_reply(conn, 200, headers.c_str(), "event: message\ndata: %s\n\n", response_str.c_str());
					}
					else
					{
						std::string headers = "mcp-session-id: " + session_id + "\r\n";
						mg_http_reply(conn, 202, headers.c_str(), "");
						return;
					}
				}
			}
		}
		else if (self->m_use_authorization && mg_strcmp(hm->uri, mg_str(("/.well-known/oauth-protected-resource" + self->m_entry_point).c_str())) == 0)
		{
			if (mg_strcasecmp(hm->method, mg_str("GET")) == 0)
			{
				mg_http_reply(
					conn,
					200,
					"Access-Control-Allow-Origin: *\r\nContent-Type: application/json\r\n",
					"{"
					"\"resource\": \"%s\","
					"\"authorization_servers\": [%s],"
					"\"scopes_supported\": [%s],"
					"\"bearer_methods_supported\": [\"header\"]"
					"}",
					self->m_url.c_str(),
					self->m_authorization_servers.c_str(),
					self->m_scopes_supported.c_str()
				);
			}
			else if (mg_strcasecmp(hm->method, mg_str("OPTIONS")) == 0)
			{
				mg_http_reply(
					conn,
					204,
					"Access-Control-Allow-Origin: *\r\n"
					"Access-Control-Allow-Methods: GET\r\n"
					"Access-Control-Allow-Headers: mcp-protocol-version\r\n"
					"Access-Control-Max-Age: 864000\r\n",
					""
				);
			}
			return;
		}
		else
		{
			mg_http_reply(conn, 405, "", "");
			return;
		}
	}
}

void McpHttpServerTransport::cbTimerHandler(void* timer_data)
{
	McpHttpServerTransport* self = (McpHttpServerTransport*)timer_data;
	self->ClearSession();
}

bool McpHttpServerTransport::IsEnableSessionId(std::string session_id)
{
	return m_sessions.find(session_id) != m_sessions.end();
}

void McpHttpServerTransport::EraseSession(std::string session_id)
{
	auto it = m_sessions.find(session_id);
	if (it != m_sessions.end())
	{
		m_sessions.erase(it);
	}
}

void McpHttpServerTransport::ClearSession()
{
	auto it = m_sessions.begin();
	while (it != m_sessions.end())
	{
		if (it->second > 0)
		{
			it->second--;
			it++;
		}
		else
		{
			it = m_sessions.erase(it);
		}
	}
}

}
