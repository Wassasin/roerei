#pragma once

#include <roerei/storage.hpp>
#include <roerei/dataset.hpp>

#include <roerei/generic/create_map.hpp>

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

	static void map_summary_f(summary_t& s, std::map<uri_t, uri_t> const& mapping) {
		swap_f(s.uri, mapping);

		for(auto& t : s.type_uris)
			swap_f(t.uri, mapping);

		if(s.body_uris)
			for(auto& b : *s.body_uris)
				swap_f(b.uri, mapping);
	}

	/* Added late-stage in writing of thesis, as previous work thought the difference
	 * between ind and con to be irrelevant.
	 *
	 * Remove .con and .ind
	 * Keep .var, so that it later can be blacklisted (ugly, I know)
	 */
	static void remove_irrelevant(std::string& str)
	{
		auto i = str.find(".var");
		if(i != str.npos)
			return;

		i = str.find(".con");
		
		if(i == str.npos)
			i = str.find(".ind");
		
		if(i == str.npos)
			return;

		str.erase(i);
	}

	static void swap_f(std::string& str, std::map<uri_t, uri_t> const& mapping) {
		auto const it = mapping.find(str);
		if(it != mapping.end())
			str.assign(it->second); // Copy
		
		remove_irrelevant(str);
	}

	struct phase1
	{
		std::set<uri_t> objects, prior_objects, term_uris, type_uris;

		void add(summary_t&& s, bool prior, std::map<uri_t, uri_t> const& mapping)
		{
			if(s.type_uris.empty())
			{
				std::cerr << "Ignored " << s.uri << " (empty typeset)" << std::endl;
				return;
			}
			map_summary_f(s, mapping);

			for(auto&& t : s.type_uris)
			{
				if(blacklisted(t.uri))
					continue;

				type_uris.emplace(std::move(t.uri));
			}

			if(s.body_uris)
			{
				bool added_something = false;
				for(auto&& b : *s.body_uris)
				{
					if(blacklisted(b.uri))
						continue;

					term_uris.emplace(std::move(b.uri));
					added_something = true;
				}

				if(added_something) {
					if (prior) {
						prior_objects.emplace(s.uri);
					}

					objects.emplace(std::move(s.uri));
				}
			}
		}
	};

	struct phase2
	{
		std::set<uri_t> objects, prior_objects, term_uris, type_uris, dependencies;

		phase2(phase1&& rhs)
			: objects(std::move(rhs.objects))
			, prior_objects(std::move(rhs.prior_objects))
			, term_uris(std::move(rhs.term_uris))
			, type_uris(std::move(rhs.type_uris))
			, dependencies()
		{
			std::set_difference(term_uris.begin(), term_uris.end(), type_uris.begin(), type_uris.end(), std::inserter(dependencies, dependencies.begin()));
		}

		void add(summary_t&& s, std::map<uri_t, uri_t> const& mapping)
		{
			map_summary_f(s, mapping);

			auto it = objects.find(s.uri);
			if(it == objects.end())
				return; // Ignore

			bool remove_object = true;
			if(s.body_uris)
				for(auto&& b : *s.body_uris)
				{
					if(blacklisted(b.uri))
						continue;

					if(dependencies.find(b.uri) == dependencies.end())
						continue;

					remove_object = false;
				}

			// Remove objects without any dependency
			if(remove_object) {
				objects.erase(s.uri);
				prior_objects.erase(s.uri);
			}
		}
	};

	struct phase3
	{
		dataset_t d;
		std::map<uri_t, object_id_t> objects_map;
		std::map<uri_t, feature_id_t> type_uris_map;
		std::map<uri_t, dependency_id_t> dependency_map;

		phase3(phase2&& rhs)
			: d(std::move(rhs.objects), std::move(rhs.type_uris), std::move(rhs.dependencies))
			, objects_map(create_map(d.objects))
			, type_uris_map(create_map(d.features))
			, dependency_map(create_map(d.dependencies))
		{
			for (uri_t po : rhs.prior_objects) {
				d.prior_objects.emplace(objects_map.at(po));
			}
			std::cout << "Loaded maps" << std::endl;
		}

		template<typename F>
		void add(summary_t&& s, std::map<uri_t, uri_t> const& mapping, F const& read_value)
		{
			map_summary_f(s, mapping);

			auto it = objects_map.find(s.uri);
			if(it == objects_map.end())
				return; // Ignore
			object_id_t row = it->second;

			auto fv(d.feature_matrix[row]);
			auto dv(d.dependency_matrix[row]);

			for(auto const& t : s.type_uris)
			{
				if(blacklisted(t.uri))
					continue;

				feature_id_t col = type_uris_map.at(t.uri);
				fv[col] = read_value(t);
			}

			if(s.body_uris)
				for(auto&& b : *s.body_uris)
				{
					if(blacklisted(b.uri))
						continue;

					auto it_col = dependency_map.find(b.uri);
					if(it_col == dependency_map.end())
						continue;
					dependency_id_t col = it_col->second;
					dv[col] = read_value(b);
				}
		}

	};

public:
	enum class variant_e {
		frequency,
		depth,
		flat
	};

	template<typename MAP_F, typename SUMMARY_F>
	static std::map<std::string, dataset_t> construct(MAP_F&& read_mapping_f, SUMMARY_F&& read_summary_f, variant_e variant)
	{
		std::map<uri_t, uri_t> mapping;
		read_mapping_f([&](mapping_t&& m) {
			std::cout << m.src << " -> " << m.dest << std::endl;
			mapping.emplace(std::make_pair(std::move(m.src), std::move(m.dest)));
		});

		// List all objects, terms and types; filters using blacklists; mark prior objects
		std::map<std::string, phase1> p1;
		read_summary_f([&](summary_t&& s, bool prior) {
			p1[s.corpus].add(std::move(s), prior, mapping);
		});

		// Determine dependencies
		std::map<std::string, phase2> p2;
		for(auto&& kvp : p1)
			p2.emplace(std::move(kvp)); // Non-trivial move-constructor

		read_summary_f([&](summary_t&& s, bool /*prior*/) {
			p2.find(s.corpus)->second.add(std::move(s), mapping);
		});

		// Instance feature and dependency matrices
		std::map<std::string, phase3> p3;
		for(auto&& kvp : p2)
		{
			std::cout << kvp.first << std::endl;
			std::cout << "Defined constants: " << kvp.second.objects.size() << std::endl;
			std::cout << "Term constants: " << kvp.second.term_uris.size() << std::endl;
                        std::cout << "Type constants (defs): " << kvp.second.type_uris.size() << std::endl;
                        std::cout << "Dependencies (thms): " << kvp.second.dependencies.size() << std::endl;

			p3.emplace(std::move(kvp));
		}

		switch(variant) {
		case variant_e::frequency:
			read_summary_f([&](summary_t&& s, bool /*prior*/) {
				p3.find(s.corpus)->second.add(std::move(s), mapping, [](summary_t::occurance_t const& occ) { return occ.freq; });
			});
			break;
		case variant_e::depth:
			read_summary_f([&](summary_t&& s, bool /*prior*/) {
				p3.find(s.corpus)->second.add(std::move(s), mapping, [](summary_t::occurance_t const& occ) { return occ.depth; });
			});
			break;
		case variant_e::flat:
			read_summary_f([&](summary_t&& s, bool /*prior*/) {
				p3.find(s.corpus)->second.add(std::move(s), mapping, [](summary_t::occurance_t const& occ) { return occ.freq == 1 ? 1 : 0; });
			});
			break;
		}

		std::cout << "Loaded datasets" << std::endl;

		std::map<std::string, dataset_t> result;
		for(auto&& kvp : p3)
			result.emplace(kvp.first, std::move(kvp.second.d));

		return result;
	}

	static std::map<std::string, dataset_t> construct_from_repo(variant_e variant)
	{
		return construct(
			[](auto f) { storage::read_mapping(f); },
			[](auto f) {
				storage::read_summaries([&](summary_t&& s) {
					for(auto&& t : s.type_uris) {
						std::cout << s.uri << " <- " << t.freq << "*" << t.uri << std::endl;
					}

					auto rebrand_corpus = [](std::string const& new_corpus, summary_t t) -> summary_t { // Make a copy
						t.corpus = new_corpus;
						return t;
					};

					if (s.corpus == "Coq") {
						f(rebrand_corpus("ch2o", s), true);
						f(rebrand_corpus("CoRN", s), true);
						f(rebrand_corpus("MathClasses", s), true);
						f(rebrand_corpus("mathcomp", s), true);
					} else if(s.corpus == "MathClasses") {
						f(rebrand_corpus("CoRN", s), true);
					}

					f(std::move(s), false);
				});
			},
			variant
		);
	}
};

}
