#include <roerei/cpp14_fix.hpp>
#include <roerei/storage.hpp>

#include <roerei/generic/encapsulated_vector.hpp>

#include <roerei/serialization/serialize_fusion.hpp>
#include <roerei/serialization/deserialize_fusion.hpp>

#include <roerei/serialization/msgpack_serializer.hpp>
#include <roerei/serialization/msgpack_deserializer.hpp>

#include <roerei/generic/common.hpp>

#include <boost/filesystem.hpp>

#include <fstream>

namespace roerei
{

namespace detail
{

template<typename F>
static inline void read_msgpack_lined_file(std::string const& filename, F const& f)
{
	/* The summary repository consists of summary_t objects separated by newlines.
	 * The same is true for mappings.
	 * These objects do not conform to the serialize_fusion standard.
	 * Because these objects might have newline codepoints, retry parsing a line if
	 * EOB is encountered.
	 */

	std::ifstream is(filename, std::ios::binary);
	std::string line, tmp;

	while(std::getline(is, tmp))
	{
		line += tmp;

		msgpack_deserializer d;
		d.feed(line);

		try
		{
			f(d);
			line.clear();
		} catch(eob_error)
		{
			line += '\n';
		}
	}
}

inline std::string read_to_string(std::string const& filename)
{
	std::ifstream is(filename, std::ios::binary);
	std::string buf;

	is.seekg(0, std::ios::end);
	buf.reserve(is.tellg());
	is.seekg(0, std::ios::beg);

	buf.assign(
		(std::istreambuf_iterator<char>(is)), // Most Vexing Parse Parenthesis; necessary.
		std::istreambuf_iterator<char>()
	);

	return buf;
}

}

void storage::read_summaries(std::function<void(summary_t&&)> const& f)
{
	static const std::string repo_path = "./data/repo.msgpack";
	static const std::string __bogus = "__bogus";

	auto read_freq_or_depth = [&](msgpack_deserializer& d) {
		if(2 != d.read_array(__bogus))
			throw std::logic_error("Pair 'freq or depth'' does not have appropriate size");

		std::pair<std::string, size_t> p;
		d.read(__bogus, p.first);
		d.read(__bogus, p.second);
		return p;
	};

	auto read_occurance_map = [&](msgpack_deserializer& d) {
		std::map<uri_t, summary_t::occurance_t> map;
		if(2 != d.read_array(__bogus))
			throw std::logic_error("Pair 'freqlist and depthlist' does not have appropriate size");

		{
			size_t size = d.read_array(__bogus);
			for(size_t i = 0; i < size; ++i)
			{
				auto p = read_freq_or_depth(d);
				auto result = map.emplace(std::make_pair(p.first, summary_t::occurance_t{p.first, p.second, 0}));

				if(!result.second)
					result.first->second.freq += p.second;
			}
		}

		{
			size_t size = d.read_array(__bogus);
			for(size_t i = 0; i < size; ++i)
			{
				auto p = read_freq_or_depth(d);
				auto result = map.emplace(std::make_pair(p.first, summary_t::occurance_t{p.first, 0, p.second}));

				if(!result.second)
					result.first->second.depth += p.second;
			}
		}

		return map;
	};

	if(!boost::filesystem::exists(repo_path))
		throw std::runtime_error("Repo does not exist");

	detail::read_msgpack_lined_file(repo_path, [&](msgpack_deserializer& d) {
		summary_t s;

		if(5 != d.read_array(__bogus))
			throw std::logic_error("Pair 'summary' does not have appropriate size");

		d.read(__bogus, s.corpus);
		d.read(__bogus, s.file);
		d.read(__bogus, s.uri);

		{
			auto map = read_occurance_map(d);
			s.type_uris.reserve(map.size());
			for(auto&& kvp : map)
				s.type_uris.emplace_back(std::move(kvp.second));
		}

		try
		{
			auto map = read_occurance_map(d);
			s.body_uris.reset(std::vector<summary_t::occurance_t>());
			s.body_uris->reserve(map.size());
			for(auto&& kvp : map)
				s.body_uris->emplace_back(std::move(kvp.second));
		} catch(type_error /*e*/)
		{}

		f(std::move(s));
	});
}

void storage::read_mapping(std::function<void(mapping_t&&)> const& f)
{
	static const std::string mapping_path = "./data/mapping.msgpack";
	static const std::string __bogus = "__bogus";

	if(!boost::filesystem::exists(mapping_path))
		throw std::runtime_error("Mapping does not exist");

	detail::read_msgpack_lined_file(mapping_path, [&](msgpack_deserializer& d) {
		mapping_t m;

		if(3 != d.read_array(__bogus))
			throw std::logic_error("Array does not have appropriate size");

		d.read(__bogus, m.file);
		d.read(__bogus, m.src);
		d.read(__bogus, m.dest);

		f(std::move(m));
	});
}

void storage::write_dataset(std::string const& corpus, const dataset_t &d)
{
	std::string const dataset_path = std::string("./data/")+corpus+".msgpack";

	msgpack_serializer s;
	serialize(s, "dataset", d);

	std::ofstream os(dataset_path, std::ios::binary);

	s.dump([&](const char* buf, size_t len) {
		os.write(buf, len);
	});
}

dataset_t storage::read_dataset(std::string const& corpus)
{
	std::string const dataset_path = std::string("./data/")+corpus+".msgpack";

	if(!boost::filesystem::exists(dataset_path))
		throw std::runtime_error(std::string("Dataset ")+corpus+" does not exist");

	msgpack_deserializer d;
	d.feed(detail::read_to_string(dataset_path));

	return deserialize<dataset_t>(d, "dataset");
}

void storage::read_result(std::function<void(cv_result_t)> const& f, std::string const& results_path)
{
	if(!boost::filesystem::exists(results_path))
		return; // Do nothing

	msgpack_deserializer d;
	d.feed(detail::read_to_string(results_path));

	try
	{
		while(true)
		{
			f(deserialize<cv_result_t>(d, "cv_result_v2"));
		}
	} catch(eob_error)
	{
		// Done!
	}
}

void storage::write_result(cv_result_t const& result)
{
	msgpack_serializer s;
	serialize(s, "cv_result_v2", result);

	std::ofstream os("./data/results.msgpack", std::ios::app | std::ios::out | std::ios::binary);
	s.dump([&os](const char* buf, size_t len) {
		os.write(buf, len);
		os.flush();
	});
}

void storage::read_v1_result(std::function<void(cv_result_v1_t)> const& f, std::string const& results_path)
{
	if(!boost::filesystem::exists(results_path))
	return; // Do nothing

	msgpack_deserializer d;
	d.feed(detail::read_to_string(results_path));

	try
	{
		while(true)
		{
			f(deserialize<cv_result_v1_t>(d, "cv_result"));
		}
	} catch(eob_error)
	{
		// Done!
	}
}

void storage::write_legacy_dataset(std::string const& path, dataset_t const& d)
{
	{
		std::ofstream os_feat(path + "/feat");
		os_feat << '\n';
		for(uri_t const& feat_str : d.features)
			os_feat << '"' << feat_str << '"' << "\n";
	}

	{
		std::ofstream os_symb(path + "/symb");
		d.feature_matrix.citerate([&](auto row) {
			os_symb << '"' << d.objects[row.row_i] << "\":";

			bool first = true;
            for(std::pair<feature_id_t, dataset_t::value_t> const kvp : row)
			{
				if(first)
					first = false;
				else
					os_symb << ", ";

				os_symb << '"' << d.features[kvp.first] << '"';
			}
			os_symb << '\n';
		});
	}

	{
		std::ofstream os_deps(path + "/deps");
		d.dependency_matrix.citerate([&](auto row) {
			os_deps << '"' << d.objects[row.row_i] << "\":";

			bool first = true;
            for(std::pair<dependency_id_t, dataset_t::value_t> const kvp : row)
			{
				if(first)
					first = false;
				else
					os_deps << ' ';

				os_deps << '"' << d.dependencies[kvp.first] << '"';
			}
			os_deps << '\n';
		});
	}
}

}
