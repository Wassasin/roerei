#pragma once

#include <string>

namespace roerei
{
	class exporter
	{
	public:
		static void exec(std::string const& source_path, std::string const& export_path);
	};
}
