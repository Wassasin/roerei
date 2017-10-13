#pragma once

#include <roerei/summary.hpp>
#include <roerei/mapping.hpp>
#include <roerei/dataset.hpp>
#include <roerei/cv_result.hpp>

#include <functional>

namespace roerei
{

class storage
{
private:
	storage() = delete;

public:
	static void read_summaries(std::function<void(summary_t&&)> const& f);
	static void read_mapping(std::function<void(mapping_t&&)> const& f);

	static void read_result(std::function<void(cv_result_t)> const& f, std::string const& results_path = "./data/results.msgpack");
	static void write_result(cv_result_t const& r);

	static void read_v1_result(std::function<void(cv_result_v1_t)> const& f, std::string const& results_path);

	static dataset_t read_dataset(std::string const& corpus);
	static void write_dataset(std::string const& corpus, dataset_t const& d);

	static void write_legacy_dataset(std::string const& path, dataset_t const& d);
};

}
