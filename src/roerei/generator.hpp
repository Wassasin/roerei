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
		std::set<uri_t> objects, dependencies;
		storage::read_summaries([&](summary_t&& s) {
			objects.emplace(std::move(s.uri));

			for(auto&& t : s.type_uris)
			{
				objects.emplace(t.uri); // Copy
				dependencies.emplace(std::move(t.uri));
			}

			if(s.body_uris)
				for(auto&& b : *s.body_uris)
				{
					objects.emplace(std::move(b.uri));
				}
		});

		std::set<uri_t> theorems;
		std::set_difference(objects.begin(), objects.end(), dependencies.begin(), dependencies.end(), std::inserter(theorems, theorems.begin()));

		std::cout << "Objects: " << objects.size() << std::endl;
		std::cout << "Dependencies: " << dependencies.size() << std::endl;
		std::cout << "Theorems: " << theorems.size() << std::endl;

		dataset_t d(std::move(objects), std::move(dependencies));
		std::cout << "Initialized dataset" << std::endl;

		std::map<uri_t, size_t> objects_map(create_map<uri_t>(d.objects)), dependencies_map(create_map<uri_t>(d.dependencies));
		std::cout << "Loaded maps" << std::endl;

		size_t i = 0, total = 0;
		storage::read_summaries([&](summary_t&& s) {
			size_t row = objects_map.at(s.uri);
			auto v(d.matrix[row]);

			for(auto const& t : s.type_uris)
			{
				size_t col = dependencies_map.at(t.uri);
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
