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

}
