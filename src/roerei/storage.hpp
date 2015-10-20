#pragma once

#include <roerei/serialize_fusion.hpp>
#include <roerei/deserialize_fusion.hpp>

#include <roerei/msgpack_serializer.hpp>
#include <roerei/msgpack_deserializer.hpp>

#include <roerei/summary.hpp>
#include <roerei/dataset.hpp>
#include <roerei/common.hpp>

#include <boost/filesystem.hpp>

#include <fstream>

namespace roerei
{

class storage
{
private:
	storage() = delete;

public:
	template<typename F, typename = std::enable_if_t<is_function<F, void(summary_t&&)>::value>>
	static void read_summaries(F const& f);

	static void write_dataset(dataset_t const& d);
	static dataset_t read_dataset();
};

template<typename F, typename>
void storage::read_summaries(F const& f)
{
	/* The summary repository consists of summary_t objects separated by newlines.
	 * These objects do not conform to the serialize_fusion standard.
	 * Because these objects might have newline codepoints, retry parsing a line if
	 * EOB is encountered.
	 */

	static const std::string repo_path = "./data/repo.msgpack";
	static const std::string __bogus = "__bogus";

	if(!boost::filesystem::exists(repo_path))
		throw std::runtime_error("Repo does not exist");

	std::ifstream is(repo_path, std::ios::binary);
	std::string line, tmp;

	while(std::getline(is, tmp))
	{
		msgpack_deserializer d;
		line += tmp;
		d.feed(line);

		try
		{
			summary_t s;

			if(4 != d.read_array(__bogus))
				throw std::logic_error("Array does not have appropriate size");

			d.read(__bogus, s.file);
			d.read(__bogus, s.uri);

			size_t type_uris_size = d.read_array(__bogus);
			s.type_uris.reserve(type_uris_size);
			for(size_t i = 0; i < type_uris_size; ++i)
			{
				if(2 != d.read_array(__bogus))
					throw std::logic_error("Pair does not have appropriate size");

				summary_t::frequency_t freq;
				d.read(__bogus, freq.uri);
				d.read(__bogus, freq.freq);

				s.type_uris.emplace_back(std::move(freq));
			}

			boost::optional<size_t> body_uris_size;
			try
			{
				body_uris_size = d.read_array(__bogus);
			} catch(type_error e)
			{}

			if(body_uris_size)
			{
				s.body_uris.reset(std::vector<summary_t::frequency_t>());
				s.body_uris->reserve(*body_uris_size);

				for(size_t i = 0; i < *body_uris_size; ++i)
				{
					if(2 != d.read_array(__bogus))
						throw std::logic_error("Pair does not have appropriate size");

					summary_t::frequency_t freq;
					d.read(__bogus, freq.uri);
					d.read(__bogus, freq.freq);

					s.body_uris->emplace_back(std::move(freq));
				}
			}

			f(std::move(s));
			line.clear();
		} catch(eob_error)
		{
			line += '\n';
		}
	}
}

void storage::write_dataset(const dataset_t &d)
{
	static std::string const dataset_path = "./data/dataset.msgpack";

	msgpack_serializer s;
	serialize(s, "dataset", d);

	std::ofstream os(dataset_path, std::ios::binary);

	s.dump([&](const char* buf, size_t len) {
		os.write(buf, len);
	});
}

dataset_t storage::read_dataset()
{
	static std::string const dataset_path = "./data/dataset.msgpack";

	if(!boost::filesystem::exists(dataset_path))
		throw std::runtime_error("Dataset does not exist");

	std::ifstream is(dataset_path, std::ios::binary);
	std::string buf;

	is.seekg(0, std::ios::end);
	buf.reserve(is.tellg());
	is.seekg(0, std::ios::beg);

	buf.assign(
		(std::istreambuf_iterator<char>(is)), // Most Vexing Parse Parenthesis; necessary.
		std::istreambuf_iterator<char>()
	);

	msgpack_deserializer d;
	d.feed(buf);

	return deserialize<dataset_t>(d, "dataset");
}

}
