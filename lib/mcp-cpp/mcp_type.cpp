#pragma once

#include "mcp-cpp/mcp_type.h"

namespace Mcp {

const std::vector<McpContent> CreateSimpleContent(const std::string& value)
{
	return
	{
		{
			{
				{
					.value = value
				}
			}
		}
	};
}

std::string McpPropertyTypeToString(McpPropertyType type)
{
	switch (type) {
	case MCP_PROPERTY_TYPE_NUMBER:
		return "number";
	case MCP_PROPERTY_TYPE_TEXT:
		return "text";
	case MCP_PROPERTY_TYPE_STRING:
		return "string";
	case MCP_PROPERTY_TYPE_OBJECT:
		return "object";
	default:
		return "unknown";
	}
}

McpPropertyType StringToMcpPropertyType(const std::string& type)
{
	if (type == "number")
	{
		return MCP_PROPERTY_TYPE_NUMBER;
	}
	else if (type == "text")
	{
		return MCP_PROPERTY_TYPE_TEXT;
	}
	else if (type == "string")
	{
		return MCP_PROPERTY_TYPE_STRING;
	}
	else if (type == "object")
	{
		return MCP_PROPERTY_TYPE_OBJECT;
	}

	return MCP_PROPERTY_TYPE_UNKNOWN;
}

}
