#pragma once

namespace roerei
{

class normalize
{
public:
	static void exec(std::vector<std::pair<dependency_id_t, float>>& suggestions)
	{
		if(suggestions.size() == 0)
			return;

		auto it = suggestions.begin();
		float min = it->second;
		float max = it->second;
		for(it++; it != suggestions.end(); it++)
		{
			min = std::min(min, it->second);
			max = std::max(max, it->second);
		}

		float diff = max - min;

		if(diff == 0.0f)
			diff = 1.0f; // Will result in entire set to be 0.0f
		
		for(auto& kvp : suggestions)
			kvp.second = std::min((kvp.second - min) / diff, std::numeric_limits<float>::max());
	};
};

}
