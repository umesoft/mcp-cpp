#include "mcp-cpp/mcp_client.h"
#include "openai-cpp/openai.hpp"
#include "nlohmann/json.hpp"
#include "curl/curl.h"
#include <iostream>

#include <string>
#include <vector>

size_t HeaderCallback(char* ptr, size_t size, size_t nmemb, std::string* headerData)
{
    printf("+++ HeaderCallback +++ : size = %d, nmemb = %d\n", (int)size, (int)nmemb);

    size_t totalSize = size * nmemb;
    std::string header(ptr, totalSize);
    printf("%s", header.c_str());

    headerData->append(ptr, totalSize);
    return totalSize;
}

size_t WriteCallback(char* ptr, size_t size, size_t nmemb, std::string* responseData)
{
    printf("+++ WriteCallback +++ : size = %d, nmemb = %d\n", (int)size, (int)nmemb);

    size_t totalSize = size * nmemb;
    std::string response(ptr, totalSize);
    printf("%s", response.c_str());

    responseData->append(ptr, totalSize);
    return totalSize;
}

void ReplaceAll(std::string& stringreplace, const std::string& origin, const std::string& dest)
{
    size_t pos = 0;
    size_t offset = 0;
    size_t len = origin.length();
    while ((pos = stringreplace.find(origin, offset)) != std::string::npos) {
        stringreplace.replace(pos, len, dest);
        offset = pos + dest.length();
    }
}

std::vector<std::string> split_naive(const std::string& s, char delim) 
{
    std::vector<std::string> elems;
    std::string item;
    for (char ch : s) 
    {
        if (ch == delim) 
        {
            if (!item.empty())
            {
                elems.push_back(item);
            }
            item.clear();
        }
        else 
        {
            item += ch;
        }
    }
    if (!item.empty())
    {
        elems.push_back(item);
    }
    return elems;
}

void test()
{
    std::string headerData;
    std::string responseData;

    CURL* curl = curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headerData);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8000/mcp");

    auto initialize = R"(
        {
            "jsonrpc": "2.0",
            "id": 1,
            "method": "initialize",
                "params": {
                "protocolVersion": "2025-06-18",
                "capabilities": {},
                "clientInfo": {
                    "name": "ExampleClient",
                    "title": "Example Client Display Name",
                    "version": "1.0.0"
                }
            }
        }    
    )"_json;

    std::string request = initialize.dump();

    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, request.length());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.c_str());

    CURLcode  res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        return;
    }

    std::cout << "Header data  : " << headerData << std::endl;
    std::cout << "Response data: " << responseData << std::endl;

    ReplaceAll(headerData, "\r\n", "\n");
    auto header_record = split_naive(headerData, '\n');

    std::string session_id = "";
    for (int i = 0; i < header_record.size(); i++)
    {
        int pos = header_record[i].find("mcp-session-id: ");
        if (pos == 0)
        {
            session_id = header_record[i];
            break;
        }
    }

    curl_slist_free_all(headers);
    headers = NULL;

    //---------------------------------

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, session_id.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8000/mcp");

    auto tool_call = R"(
        {
            "jsonrpc": "2.0",
            "id": 2,
            "method": "tools/call",
                "params": {
                "name": "count_down",
                "arguments": {
                    "value": "5"
                }
            }
        }    
    )"_json;

    request = tool_call.dump();

    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, request.length());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.c_str());

    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        return;
    }

    std::cout << "Header data  : " << headerData << std::endl;
    std::cout << "Response data: " << responseData << std::endl;

    curl_slist_free_all(headers);

    curl_easy_cleanup(curl);

}

int main()
{
    /*
    Mcp::McpClient client;
    client.Test();
    */

    /*
    openai::start("", "", true, "http://127.0.0.1:1234/v1/");

    auto chat = openai::chat().create(R"(
    {
        "model": "phi-4",
        "messages":[{"role":"user", "content":"hello!"}]
    }
    )"_json);
    std::cout << "Response is:\n" << chat.dump(2) << '\n'; 
    */

    test();

    return 0;
}
