#pragma once

#include <string>
#include <vector>

namespace Mcp {

enum McpPropertyType {
	MCP_PROPERTY_TYPE_UNKNOWN = -1,
	MCP_PROPERTY_TYPE_NUMBER = 0,
	MCP_PROPERTY_TYPE_TEXT,
	MCP_PROPERTY_TYPE_STRING,
	MCP_PROPERTY_TYPE_OBJECT
};

std::string McpPropertyTypeToString(McpPropertyType type);
McpPropertyType StringToMcpPropertyType(const std::string& type);

struct McpProperty {
	std::string name;
	McpPropertyType type;
	std::string description;
	bool required;
};

struct McpTool {
	std::string name;
	std::string description;
	std::vector<McpProperty> input_schema;
	std::vector<McpProperty> output_schema;
};

struct McpPropertyValue {
	std::string name;
	std::string value;
};

struct McpContent {
	std::vector<McpPropertyValue> properties;
};

const std::vector<McpContent> CreateSimpleContent(const std::string& value);

}
