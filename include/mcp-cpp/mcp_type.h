#pragma once

namespace Mcp {

enum McpPropertyType {
	MCP_PROPERTY_TYPE_NUMBER = 1,
	MCP_PROPERTY_TYPE_TEXT,
	MCP_PROPERTY_TYPE_STRING,
	MCP_PROPERTY_TYPE_OBJECT
};

struct McpProperty {
	std::string property_name;
	McpPropertyType property_type;
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
	std::string property_name;
	std::string value;
};

struct McpContent {
	std::vector<McpPropertyValue> properties;
};

}
