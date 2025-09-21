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

#include "mongoose.c"

#include "mcp_http_server_transport_impl.h"
#include "mcp_common.h"
#include "jwt-cpp/jwt.h"

namespace Mcp {

std::unique_ptr<McpHttpServerTransport> McpHttpServerTransport::CreateInstance(const std::string& host, const std::string& entry_point, unsigned long long session_timeout)
{
	return  std::make_unique<McpHttpServerTransportImpl>(host, entry_point, session_timeout);
}

typedef void (*mg_timer_handler_t)(void*);

McpHttpServerTransportImpl::McpHttpServerTransportImpl(const std::string& host, const std::string& entry_point, unsigned long long session_timeout)
	: m_host(host)
	, m_entry_point(entry_point)
	, m_session_timeout(session_timeout)
	, m_use_tls(false)
	, m_use_authorization(false)
{
}

McpHttpServerTransportImpl::~McpHttpServerTransportImpl()
{
}

void McpHttpServerTransportImpl::SetTls(const std::string& cert_file, const std::string& key_file)
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

void McpHttpServerTransportImpl::SetAuthorization(const std::string& authorization_servers, const std::string& scopes_supported)
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

bool McpHttpServerTransportImpl::OnOpen()
{
	UpdateUrl();

	mg_mgr_init(&m_mgr);

	mg_timer_init(&m_mgr.timers, &m_timer, m_session_timeout, MG_TIMER_REPEAT, (mg_timer_handler_t)McpHttpServerTransportImpl::cbTimerHandler, this);

	if (mg_http_listen(
		&m_mgr,
		m_host.c_str(),
		(mg_event_handler_t)cbEvHander,
		this) == nullptr)
	{
		return false;
	}

	return true;
}

void McpHttpServerTransportImpl::UpdateUrl()
{
	if (m_use_tls)
	{
		m_url = "https://";
	}
	else
	{
		m_url = "http://";
	}
	m_url += m_host;
	m_url += m_entry_point;
}

void McpHttpServerTransportImpl::OnClose()
{
	mg_timer_free(&m_mgr.timers, &m_timer);
	mg_mgr_free(&m_mgr);
}

bool McpHttpServerTransportImpl::OnProcRequest()
{
	mg_mgr_poll(&m_mgr, 50);
	return true;
}

void McpHttpServerTransportImpl::cbEvHander(void* connection, int event_code, void* event_data)
{
	mg_connection* conn = (mg_connection*)connection;
	McpHttpServerTransportImpl* self = (McpHttpServerTransportImpl*)conn->fn_data;

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
	else if (event_code == MG_EV_CLOSE)
	{
		std::lock_guard<std::recursive_mutex> lock(self->m_mutex);

		for (auto it = self->m_sessions.begin(); it != self->m_sessions.end(); it++)
		{
			SessionInfo& session_info = it->second;
			if (session_info.connection == connection)
			{
				session_info.connection = nullptr;
				break;
			}
		}
	}
	else if (event_code == MG_EV_HTTP_MSG)
	{
		struct mg_http_message* hm = (struct mg_http_message*)event_data;
		if (mg_match(hm->uri, mg_str(self->m_entry_point.c_str()), NULL))
		{
			std::string auth_token = "";
			std::string session_id = "";

			for (int i = 0; i < MG_MAX_HTTP_HEADERS; i++)
			{
				if (hm->headers[i].name.buf == nullptr)
				{
					break;
				}

				std::string key = std::string(hm->headers[i].name.buf, hm->headers[i].name.len);
				string_to_lower(key);

				if (key == "authorization")
				{
					auth_token.assign(hm->headers[i].value.buf, hm->headers[i].value.len);
				}
				else if (key == "mcp-session-id")
				{
					session_id.assign(hm->headers[i].value.buf, hm->headers[i].value.len);
				}
			}

			if (mg_strcasecmp(hm->method, mg_str("DELETE")) == 0)
			{
				mg_http_reply(conn, 200, "", "");
				self->EraseSession(session_id);
			}
			else if (mg_strcasecmp(hm->method, mg_str("GET")) == 0)
			{
				mg_http_reply(conn, 405, "", "");
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
					std::lock_guard<std::recursive_mutex> lock(self->m_mutex);

					SessionInfo* session_info = nullptr;
					if (strcmp(method, "initialize") == 0)
					{
						session_info = self->CreateSession(conn);
					}
					else
					{
						session_info = self->FindSession(session_id);
					}
					if (session_info == nullptr)
					{
						mg_http_reply(conn, 400, "", "");
						return;
					}
					session_info->connection = connection;
					session_info->notification_is_start = false;

					std::string request_str = std::string(hm->body.buf, hm->body.buf + hm->body.len);
					if (!self->m_handler->OnRecv(session_info->session_id, request_str))
					{
						std::string headers = "mcp-session-id: " + session_info->session_id + "\r\n";
						mg_http_reply(conn, 202, headers.c_str(), "");
					}
				}
				else
				{
					mg_http_reply(conn, 405, "", "");
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
		}
		else
		{
			mg_http_reply(conn, 405, "", "");
		}
	}
	else if (event_code == MG_EV_POLL)
	{
		std::lock_guard<std::recursive_mutex> lock(self->m_mutex);

		for (auto it = self->m_sessions.begin(); it != self->m_sessions.end(); it++)
		{
			SessionInfo& session_info = it->second;
			if (session_info.connection == connection)
			{
				mg_connection* target_conn = (mg_connection*)session_info.connection;
				while (!session_info.notifications.empty())
				{
					if (!session_info.notification_is_start)
					{
						std::string headers =
							"HTTP/1.1 200 OK\r\n"
							"Transfer-Encoding: chunked\r\n"
							"Content-Type: text/event-stream\r\nmcp-session-id: " + session_info.session_id + "\r\n"
							"\r\n";
						mg_printf(conn, headers.c_str());

						session_info.notification_is_start = true;
					}

					std::string& notification_str = session_info.notifications.front();
					mg_http_printf_chunk(target_conn, "event: message\ndata: %s\n\n", notification_str.c_str());
					session_info.notifications.pop();
				}
				if (session_info.notification_is_finish)
				{
					mg_http_write_chunk(conn, "", 0);
					session_info.notification_is_finish = false;
				}
				break;
			}
		}
	}
}

void McpHttpServerTransportImpl::OnSendResponse(const std::string& session_id, const std::string& notification_str, bool is_finish)
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	auto it = m_sessions.find(session_id);
	if (it != m_sessions.end())
	{
		SessionInfo& session_info = it->second;
		session_info.notifications.push(notification_str);
		session_info.notification_is_finish = is_finish;
	}
}

void McpHttpServerTransportImpl::cbTimerHandler(void* timer_data)
{
	McpHttpServerTransportImpl* self = (McpHttpServerTransportImpl*)timer_data;
	self->ClearSession();
}

McpHttpServerTransportImpl::SessionInfo* McpHttpServerTransportImpl::CreateSession(void* connection)
{
	std::string session_id = CreateSessionId();
	SessionInfo& session_info = m_sessions[session_id];

	session_info.session_id = session_id;
	session_info.connection = connection;
	session_info.is_alive = 1;

	return &session_info;
}

McpHttpServerTransportImpl::SessionInfo* McpHttpServerTransportImpl::FindSession(std::string session_id)
{
	auto it = m_sessions.find(session_id);
	if (it == m_sessions.end())
	{
		return nullptr;
	}

	SessionInfo& session_info = it->second;
	session_info.is_alive = 1;

	return &session_info;
}

void McpHttpServerTransportImpl::EraseSession(std::string session_id)
{
	auto it = m_sessions.find(session_id);
	if (it != m_sessions.end())
	{
		m_sessions.erase(it);
	}
}

void McpHttpServerTransportImpl::ClearSession()
{
	auto it = m_sessions.begin();
	while (it != m_sessions.end())
	{
		SessionInfo& session_info = it->second;
		if (session_info.is_alive > 0)
		{
			session_info.is_alive--;
			it++;
		}
		else
		{
			it = m_sessions.erase(it);
		}
	}
}

}
