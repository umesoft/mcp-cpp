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

#pragma once

#include "nlohmann/json.hpp"

#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>
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
