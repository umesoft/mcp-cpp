#include "mcp_client_impl.h"

namespace Mcp {

std::unique_ptr<McpClient> McpClient::CreateInstance()
{
	return  std::make_unique<McpClientImpl>();
}

McpClientImpl::McpClientImpl()
{
}

bool McpClientImpl::Initialize(std::shared_ptr<McpClientTransport> m_transport)
{
	return true;
}

void McpClientImpl::Shutdown()
{
}

bool McpClientImpl::ToolsList(std::vector<McpTool>& tools)
{
	return true;
}

bool McpClientImpl::ToolsCall(std::string name, const std::map<std::string, std::string>& args)
{
	return true;
}

}
