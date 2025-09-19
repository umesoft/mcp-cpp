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
