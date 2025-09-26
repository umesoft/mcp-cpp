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

#include "mcp_client_authorization_impl.h"

extern "C"
{
#include "oauth2/jose.h"
#include "oauth2/mem.h"
#include "oauth2/util.h"
}

namespace Mcp {

McpClientAuthorizationImpl::McpClientAuthorizationImpl()
	: m_mgr(nullptr)
	, m_is_running(false)
{
}

McpClientAuthorizationImpl::~McpClientAuthorizationImpl()
{
}

void McpClientAuthorizationImpl::Reset()
{
	m_token = "";
}

bool McpClientAuthorizationImpl::OpenCallbackServer()
{
	if (m_mgr != nullptr)
	{
		return false;
	}

	std::string host = "localhost:";
	host += std::to_string(m_redirect_port_no);

	mg_mgr* mgr = new mg_mgr;
	mg_mgr_init(mgr);

	if (mg_http_listen(
		mgr,
		host.c_str(),
		(mg_event_handler_t)cbEvHander,
		this) == nullptr)
	{
		mg_mgr_free(mgr);
		delete mgr;
		return false;
	}

	m_mgr = mgr;
	m_is_running = true;

	m_callback_worker = std::make_unique<std::thread>([this]
	{
		mg_mgr* mgr = (mg_mgr *)this->m_mgr;
		while (this->m_is_running)
		{
			mg_mgr_poll(mgr, 50);
		}
		while (true)
		{
			bool is_sending = false;

			mg_connection* conn = mgr->conns;
			while (conn != nullptr)
			{
				if (conn->is_writable)
				{
					is_sending = true;
					break;
				}
				conn = conn->next;
			}

			if (!is_sending)
			{
				break;
			}

			mg_mgr_poll(mgr, 1);
		}
	});

	return true;
}

void McpClientAuthorizationImpl::cbEvHander(void* connection, int event_code, void* event_data)
{
	mg_connection* conn = (mg_connection*)connection;
	McpClientAuthorizationImpl* self = (McpClientAuthorizationImpl*)conn->fn_data;

	if (event_code == MG_EV_HTTP_MSG)
	{
		struct mg_http_message* hm = (struct mg_http_message*)event_data;
		if (mg_strcasecmp(hm->method, mg_str("GET")) == 0)
		{
			if (mg_match(hm->uri, mg_str("/callback"), NULL))
			{
				mg_str v = mg_http_var(hm->query, mg_str("code"));  
				mg_http_reply(
					conn, 
					200, 
					"", 
					"<!DOCTYPE html><html><head><script>window.onload=function(){window.open('about:blank', '_self').close();};</script></head><body>Please close your browser.</body></html>"
				);
				std::string code;
				code.assign(v.buf, v.len);
				self->SetAuthorizationCode(code);
			}
		}
	}
}

void McpClientAuthorizationImpl::CloseCallbackServer()
{
	m_is_running = false;

	if (m_callback_worker && m_callback_worker->joinable())
	{
		m_callback_worker->join();
		m_callback_worker.reset();
	}

	if (m_mgr != nullptr)
	{
		mg_mgr* mgr = (mg_mgr*)m_mgr;
		mg_mgr_free(mgr);
		delete mgr;
		m_mgr = nullptr;
	}
}

bool McpClientAuthorizationImpl::Authorize(
	const std::string& resource_meta_url,
	const std::string& client_name,
	std::function <bool(const std::string& url)> open_browser
)
{
	if (m_redirect_url.empty())
	{
		if (!OpenCallbackServer())
		{
			return false;
		}
	}

	if (!GetOAuthProtectedResource(resource_meta_url))
	{
		CloseCallbackServer();
		return false;
	}
	if (!GetResourceMetaData())
	{
		CloseCallbackServer();
		return false;
	}
	if (m_client_id.empty())
	{
		if (!DynamicRegistration(client_name))
		{
			CloseCallbackServer();
			return false;
		}
	}

	std::string url = GetAuthUrl();

	if (open_browser)
	{
		if (!open_browser(url))
		{
			CloseCallbackServer();
			return false;
		}
	}
	else
	{
#ifdef _WIN32
		std::string cmd = "start \"\" \"" + url + "\"";
#else
		std::string cmd = "xdg-open " + url;
#endif

		std::system(cmd.c_str());
	}

	if (!WaitToken())
	{
		CloseCallbackServer();
		return false;
	}

	if (m_redirect_url.empty())
	{
		CloseCallbackServer();
	}

	if (!RequestToken())
	{
		return false;
	}

	return true;
}

bool McpClientAuthorizationImpl::GetOAuthProtectedResource(const std::string& resource_url)
{
	CURL* curl = curl_easy_init();

	std::string responseData;
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

	curl_easy_setopt(curl, CURLOPT_URL, resource_url.c_str());

	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

	CURLcode  res = curl_easy_perform(curl);
	if (res != CURLE_OK)
	{
		curl_easy_cleanup(curl);
		return false;
	}

	long http_code;
	res = curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &http_code);
	if (res != CURLE_OK)
	{
		curl_easy_cleanup(curl);
		return false;
	}
	curl_easy_cleanup(curl);
	if (http_code != 200)
	{
		return false;
	}

	try
	{
		m_protected_resource = nlohmann::json::parse(responseData);
	}
	catch (const nlohmann::json::parse_error& e)
	{
		return false;
	}

	return true;
}

bool McpClientAuthorizationImpl::GetResourceMetaData()
{
	auto it = m_protected_resource.find("authorization_servers");
	if (it == m_protected_resource.end())
	{
		return false;
	}

	auto& authorization_servers = it.value();

	for (auto it2 = authorization_servers.begin(); it2 != authorization_servers.end(); it2++)
	{
		std::string resource_url = *it2;
		if (resource_url.back() != '/')
		{
			resource_url += '/';
		}
		resource_url += ".well-known/oauth-authorization-server";

		CURL* curl = curl_easy_init();

		std::string responseData;
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

		curl_easy_setopt(curl, CURLOPT_URL, resource_url.c_str());

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

		CURLcode  res = curl_easy_perform(curl);
		if (res != CURLE_OK)
		{
			curl_easy_cleanup(curl);
			return false;
		}

		long http_code;
		res = curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &http_code);
		if (res != CURLE_OK)
		{
			curl_easy_cleanup(curl);
			return false;
		}
		curl_easy_cleanup(curl);
		if (http_code != 200)
		{
			return false;
		}

		try
		{
			m_resource_meta_data = nlohmann::json::parse(responseData);
		}
		catch (const nlohmann::json::parse_error& e)
		{
			return false;
		}

		m_authorization_server = *it2;

		return true;
	}

	return false;
}

bool McpClientAuthorizationImpl::DynamicRegistration(const std::string& client_name)
{
	auto it = m_resource_meta_data.find("registration_endpoint");
	if (it == m_resource_meta_data.end())
	{
		return false;
	}
	std::string regist_url = *it;

	auto regist_json = R"(
	{
		"redirect_uris": [],
		"token_endpoint_auth_method": "none",
		"grant_types": ["authorization_code", "refresh_token"],
		"response_types": ["code"],
		"client_name": ""
	}
	)"_json;

	regist_json["redirect_uris"].push_back(GetRedirectUrl());
	regist_json["client_name"] = client_name;

	std::string request = regist_json.dump();

	CURL* curl = curl_easy_init();

	std::string responseData;
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

	struct curl_slist* headers = NULL;
	headers = curl_slist_append(headers, "Content-Type: application/json");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	curl_easy_setopt(curl, CURLOPT_URL, regist_url.c_str());

	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, request.length());
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.c_str());

	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

	CURLcode res = curl_easy_perform(curl);
	curl_slist_free_all(headers);
	if (res != CURLE_OK)
	{
		curl_easy_cleanup(curl);
		return false;
	}

	long http_code;
	res = curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &http_code);
	if (res != CURLE_OK)
	{
		curl_easy_cleanup(curl);
		return false;
	}
	curl_easy_cleanup(curl);
	if (http_code != 201)
	{
		return false;
	}

	try
	{
		auto client_data = nlohmann::json::parse(responseData);

		auto it = client_data.find("client_id");
		if (it == client_data.end())
		{
			return false;
		}
		m_client_id = *it;

		auto it2 = client_data.find("client_secret");
		if (it2 == client_data.end())
		{
			return false;
		}
		m_client_secret = *it2;
	}
	catch (const nlohmann::json::parse_error& e)
	{
		return false;
	}

	return true;
}

bool McpClientAuthorizationImpl::WaitToken()
{
	std::unique_lock<std::mutex> lock(m_code_mutex);
	if (m_code_cv.wait_for(lock, std::chrono::milliseconds(m_timeout)) == std::cv_status::timeout)
	{
		return true;
	}

	return true;
}

void McpClientAuthorizationImpl::SetAuthorizationCode(const std::string& code)
{
	m_code = code;
	m_code_cv.notify_one();
}

bool McpClientAuthorizationImpl::RequestToken()
{
	auto it = m_resource_meta_data.find("token_endpoint");
	if (it == m_resource_meta_data.end())
	{
		return false;
	}
	std::string token_url = *it;

	CURL* curl = curl_easy_init();

	std::string responseData;
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

	struct curl_slist* headers = NULL;
	headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	curl_easy_setopt(curl, CURLOPT_URL, token_url.c_str());

	std::string request = "client_id=";
	request += m_client_id;
	request += "&scope=";
	request += m_scope;
	request += "&code_verifier=";
	request += m_code_verifier;	
	request += "&code=";
	request += m_code;
	request += "&redirect_uri=";
	request += GetRedirectUrl();
	request += "&grant_type=authorization_code&client_secret=";
	request += m_client_secret;

	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, request.length());
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.c_str());

	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

	CURLcode res = curl_easy_perform(curl);
	curl_slist_free_all(headers);
	if (res != CURLE_OK)
	{
		curl_easy_cleanup(curl);
		return false;
	}

	long http_code;
	res = curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &http_code);
	if (res != CURLE_OK)
	{
		curl_easy_cleanup(curl);
		return false;
	}
	curl_easy_cleanup(curl);
	if (http_code != 200)
	{
		return false;
	}

	try
	{
		auto client_data = nlohmann::json::parse(responseData);

		auto it = client_data.find("access_token");
		if (it == client_data.end())
		{
			return false;
		}
		m_token = *it;
	}
	catch (const nlohmann::json::parse_error& e)
	{
		return false;
	}

	return true;
}

std::string McpClientAuthorizationImpl::GetRedirectUrl()
{
	if (!m_redirect_url.empty())
	{
		return m_redirect_url;
	}

	std::string redirect_url = "http://localhost:";
	redirect_url += std::to_string(m_redirect_port_no);
	redirect_url += "/callback";
	return redirect_url;
}

std::string McpClientAuthorizationImpl::GetAuthUrl()
{
	auto it = m_resource_meta_data.find("authorization_endpoint");
	if (it == m_resource_meta_data.end())
	{
		return "";
	}

	GeneratePKCE();

	std::string auth_url = *it;
	auth_url += "?client_id=";
	auth_url += m_client_id;
	auth_url += "&code_challenge_method=S256&code_challenge=";
	auth_url += m_code_challenge;
	auth_url += "&response_type=code&redirect_uri=";
	auth_url += GetRedirectUrl();

	auto it2 = m_protected_resource.find("scopes_supported");
	if (it2 != m_protected_resource.end())
	{
		auto& scopes_supported = it2.value();
		for (auto it3 = scopes_supported.begin(); it3 != scopes_supported.end(); it3++)
		{
			auth_url += "&scope=";
			auth_url += *it3;
			m_scope = *it3;
			break;
		}
	}

	return auth_url;
}

#define OAUTH2_PKCE_LENGTH 48

void McpClientAuthorizationImpl::GeneratePKCE()
{
	oauth2_log_t* log = oauth2_init(OAUTH2_LOG_INFO, 0);

	char* pkce = oauth2_rand_str(log, OAUTH2_PKCE_LENGTH);

	unsigned char* dst = NULL;
	unsigned int dst_len = 0;
	oauth2_jose_hash_bytes(
		log,
		"sha256",
		(const unsigned char*)pkce,
		strlen(pkce),
		&dst,
		&dst_len
	);
	char* code_challenge;
	oauth2_base64url_encode(log, dst, dst_len, &code_challenge);

	m_code_verifier = pkce;
	m_code_challenge = code_challenge;

	oauth2_mem_free(pkce);
	oauth2_mem_free(code_challenge);

	oauth2_shutdown(log);
}

size_t McpClientAuthorizationImpl::WriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
	auto responseData = (std::string*)userdata;

	size_t totalSize = size * nmemb;
	responseData->append(ptr, totalSize);
	return totalSize;
}

}
