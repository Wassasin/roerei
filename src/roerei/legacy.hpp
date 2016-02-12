#pragma once

#include <roerei/generator.hpp>
#include <roerei/util/string_view.hpp>

#include <fstream>
#include <set>
#include <vector>

namespace roerei
{

class legacy
{
	static void eat(string_view& str, std::string const& what)
	{
		if(str.substr(0, what.size()).to_string() != what)
			throw std::runtime_error(std::string("Expected ") + what + " got " + str.to_string());

		str.remove_prefix(what.size());
	}

	static string_view eat_fact(string_view& str)
	{
		assert(str.find('\\') == str.npos); // No escaping allowed
		string_view intermediate(str.substr(str.find('"')+1));
		size_t next = intermediate.find('"');
		str.remove_prefix(intermediate.get_start() + next - str.get_start() + 1);

		return intermediate.substr(0, next);
	}

	template<typename F>
	static void read_feat(std::string const& path, F f)
	{
		std::ifstream is(path + "/feat");
		for(std::string line; std::getline(is, line); )
		{
			string_view remainder(line);
			f(eat_fact(remainder));
		}
	}

	template<typename F1, typename F2>
	static void read_symb(std::string const& path, F1 feat_f, F2 deliver_symb_f)
	{
		std::ifstream is(path+"/symb");
		for(std::string line; std::getline(is, line); )
		{
			string_view remainder(line);
			string_view symb(eat_fact(remainder));
			eat(remainder, ":");

			bool first = true;
			while(remainder.get_size() > 0)
			{
				if(first)
					first = false;
				else
					eat(remainder, ", ");

				feat_f(eat_fact(remainder));
			}

			deliver_symb_f(symb);
		}
	}

	template<typename F1, typename F2>
	static void read_deps(std::string const& path, F1 dep_f, F2 deliver_symb_f)
	{
		std::ifstream is(path+"/deps");
		for(std::string line; std::getline(is, line); )
		{
			string_view remainder(line);
			string_view symb(eat_fact(remainder));
			eat(remainder, ":");

			bool first = true;
			while(remainder.get_size() > 0)
			{
				if(first)
					first = false;
				else
					eat(remainder, " ");

				dep_f(eat_fact(remainder));
			}

			deliver_symb_f(symb);
		}
	}

public:
	static void exec()
	{
		const std::string dir = "/home/wgeraedts/tmp";
		const std::string corpus = "CoRN-legacy";

		std::map<uri_t, summary_t> result;

		{
			std::map<uri_t, size_t> features;
			read_symb(
				dir,
				[&features](string_view const& feat) {
					features[feat.to_string()]++;
				},
				[&result, &features, &dir, &corpus](string_view const& symb) {
					summary_t s = {corpus, dir, symb.to_string(), {}, boost::none};

					for(auto kvp : features)
						s.type_uris.emplace_back((summary_t::frequency_t){kvp.first, kvp.second});

					std::sort(s.type_uris.begin(), s.type_uris.end(), [](auto x, auto y) {
						return x.freq > y.freq;
					});
					result.emplace(std::make_pair(symb.to_string(), std::move(s)));

					features.clear();
				}
			);
		}

		{
			std::map<uri_t, size_t> dependencies;
			read_deps(
				dir,
				[&dependencies](string_view const& feat) {
					dependencies[feat.to_string()]++;
				},
				[&result, &dependencies, &dir, &corpus](string_view const& symb) {
					summary_t& s = result.at(symb.to_string());
					s.body_uris.reset(std::vector<summary_t::frequency_t>());

					for(auto kvp : dependencies)
						s.body_uris->emplace_back((summary_t::frequency_t){kvp.first, kvp.second});

					std::sort(s.body_uris->begin(), s.body_uris->end(), [](auto x, auto y) {
						return x.freq > y.freq;
					});

					dependencies.clear();
				}
			);
		}

		auto d_map(generator::construct(
			[](auto) {},
			[&result](auto f) {
				for(std::pair<uri_t, summary_t>&& kvp : result)
					f(std::move(kvp.second));
			}
		));

		storage::write_dataset(corpus, d_map.at(corpus));
	}
};

}