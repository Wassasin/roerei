#pragma once

#include <roerei/storage.hpp>
#include <roerei/dataset.hpp>

#include <roerei/create_map.hpp>

#include <boost/algorithm/string/predicate.hpp>

#include <set>
#include <iostream>

namespace roerei
{

class generator
{
private:
	generator() = delete;

	static bool blacklisted(uri_t const& u)
	{
		return boost::algorithm::ends_with(u, ".var");
	}

public:
	static dataset_t construct_from_repo()
	{
		std::map<uri_t, uri_t> mapping;
		storage::read_mapping([&](mapping_t&& m) {
			std::cout << m.src << " -> " << m.dest << std::endl;
			mapping.emplace(std::make_pair(std::move(m.src), std::move(m.dest)));
		});
		auto swap_f([&](std::string& str) {
			auto const it = mapping.find(str);
			if(it != mapping.end())
				str.assign(it->second); // Copy
		});

		auto map_summary_f([&](summary_t& s) {
			swap_f(s.uri);

			for(auto& t : s.type_uris)
				swap_f(t.uri);

			if(s.body_uris)
				for(auto& b : *s.body_uris)
					swap_f(b.uri);
		});

		std::set<uri_t> objects, term_uris, type_uris;
		storage::read_summaries([&](summary_t&& s) {
			if(s.type_uris.empty())
			{
				std::cerr << "Ignored " << s.uri << " (empty typeset)" << std::endl;
				return;
			}
			map_summary_f(s);

			objects.emplace(s.uri); // Copy

			for(auto&& t : s.type_uris)
			{
				if(blacklisted(t.uri))
					continue;

				type_uris.emplace(std::move(t.uri));
			}

			if(s.body_uris)
				for(auto&& b : *s.body_uris)
				{
					if(blacklisted(b.uri))
						continue;

					term_uris.emplace(std::move(b.uri));
				}
		});

		std::set<uri_t> dependencies;
		std::set_difference(term_uris.begin(), term_uris.end(), type_uris.begin(), type_uris.end(), std::inserter(dependencies, dependencies.begin()));

		std::cout << "Defined constants: " << objects.size() << std::endl;
		std::cout << "Term constants: " << term_uris.size() << std::endl;
		std::cout << "Type constants: " << type_uris.size() << std::endl;
		std::cout << "Dependencies: " << dependencies.size() << std::endl;

		dataset_t d(std::move(objects), std::move(type_uris), std::move(dependencies));
		std::cout << "Initialized dataset" << std::endl;

		std::map<uri_t, size_t> objects_map(create_map<uri_t>(d.objects)), type_uris_map(create_map<uri_t>(d.features)), dependency_map(create_map<uri_t>(d.dependencies));
		std::cout << "Loaded maps" << std::endl;

		size_t fi = 0, ftotal = 0, di = 0, dtotal = 0;
		storage::read_summaries([&](summary_t&& s) {
			map_summary_f(s);

			size_t row;

			try
			{
				row = objects_map.at(s.uri);
			} catch (std::out_of_range)
			{
				return; // Ignore
			}

			auto fv(d.feature_matrix[row]);
			auto dv(d.dependency_matrix[row]);

			for(auto const& t : s.type_uris)
			{
				if(blacklisted(t.uri))
					continue;

				size_t col = type_uris_map.at(t.uri);
				assert(t.freq > 0);
				fv[col] = t.freq;
				fi++;
				ftotal += t.freq;
			}

			if(s.body_uris)
				for(auto&& b : *s.body_uris)
				{
					if(blacklisted(b.uri))
						continue;

					size_t col;

					try
					{
						col = dependency_map.at(b.uri);
					} catch (std::out_of_range)
					{
						continue; // Ignore
					}

					assert(b.freq > 0);
					dv[col] = b.freq;
					di++;
					dtotal += b.freq;
				}
		});
		std::cout << "Loaded dataset" << std::endl;
		std::cout << "Total features: " << ftotal << " (" << fi << ")" << std::endl;
		std::cout << "Total dependencies: " << dtotal << " (" << di << ")" << std::endl;
		return std::move(d);
	}
};

}
