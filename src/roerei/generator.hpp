#pragma once

#include <roerei/storage.hpp>
#include <roerei/dataset.hpp>

#include <set>

namespace roerei
{

class generator
{
private:
	generator() = delete;

public:
	static dataset_t construct_from_repo()
	{
		std::set<uri_t> official_objects, real_objects, definitions;
		storage::read_summaries([&](summary_t&& s) {
			if(s.type_uris.empty())
			{
				std::cerr << "Ignored " << s.uri << " (empty typeset)" << std::endl;
				return;
			}

			official_objects.emplace(s.uri); // Copy
			real_objects.emplace(std::move(s.uri));

			for(auto&& t : s.type_uris)
			{
				real_objects.emplace(t.uri); // Copy
				definitions.emplace(std::move(t.uri));
			}

			if(s.body_uris)
				for(auto&& b : *s.body_uris)
				{
					real_objects.emplace(std::move(b.uri));
				}
		});

		std::set<uri_t> theorems;
		std::set_difference(real_objects.begin(), real_objects.end(), definitions.begin(), definitions.end(), std::inserter(theorems, theorems.begin()));

		std::cout << "Objects (real): " << real_objects.size() << std::endl;
		std::cout << "Objects (official): " << official_objects.size() << std::endl;
		std::cout << "Dependencies: " << definitions.size() << std::endl;
		std::cout << "Theorems: " << theorems.size() << std::endl;

		dataset_t d(std::move(official_objects), std::move(definitions));
		std::cout << "Initialized dataset" << std::endl;

		std::map<uri_t, size_t> objects_map(create_map<uri_t>(d.objects)), dependencies_map(create_map<uri_t>(d.dependencies));
		std::cout << "Loaded maps" << std::endl;

		size_t i = 0, total = 0;
		storage::read_summaries([&](summary_t&& s) {
			size_t row;

			try
			{
				row = objects_map.at(s.uri);
			} catch (std::out_of_range)
			{
				return; // Ignore
			}

			auto v(d.matrix[row]);

			for(auto const& t : s.type_uris)
			{
				size_t col = dependencies_map.at(t.uri);
				assert(t.freq > 0);
				v[col] = t.freq;
				i++;
				total += t.freq;
			}
		});
		std::cout << "Loaded dataset" << std::endl;
		std::cout << "Total: " << total << " (" << i << ")" << std::endl;
		return std::move(d);
	}
};

}
