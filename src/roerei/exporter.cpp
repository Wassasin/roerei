#include <roerei/cpp14_fix.hpp>

#include <roerei/exporter.hpp>

#include <roerei/storage.hpp>
#include <roerei/export/latex_tabular.hpp>
#include <roerei/export/csv_dump.hpp>

#include <fstream>
#include <iostream>
#include <sstream>

namespace roerei
{
	inline static std::string round_print(float f, size_t digits)
	{
		std::stringstream ss;

		size_t factor = std::pow(10, digits);
		ss << std::round(f*factor)/factor;

		std::string result = ss.str();
		size_t split = result.find(',');

		if(split == result.npos) {
			return result;
		}

		for(size_t i = digits - split; i > 0; i--)
		{
			ss << '0';
		}

		return ss.str();
	}

	void export_knn(std::string const& source_path, std::string const& output_path)
	{
		std::vector<cv_result_t> results;
		storage::read_result([&](cv_result_t const& result) {
			if(result.ml == ml_type::knn && result.strat == posetcons_type::pessimistic && result.corpus == "Coq.flat") {
				results.emplace_back(result);
			}
		}, source_path);

		std::sort(results.begin(), results.end(), [](cv_result_t const& x, cv_result_t const& y) {
			return *x.knn_params < *y.knn_params;
		});

		{
			std::ofstream os(output_path+"/knn_coq_k.tex");
			latex_tabular t(os, {'r', 'r', 'r', 'r', 'r', 'r', 'r'});
			t.write_header({"K", "\\oocover", "\\ooprecision", "\\recall", "\\rank", "\\auc", "\\volume"});
			
			for(cv_result_t const& row : results) {
				t.write_row({
					std::to_string(row.knn_params->k),
					round_print(row.metrics.oocover, 3),
					round_print(row.metrics.ooprecision, 3),
					round_print(row.metrics.recall, 3),
					round_print(row.metrics.rank, 1),
					round_print(row.metrics.auc, 3),
					round_print(row.metrics.volume, 3)
				});
			}
		}
		
		{
			std::ofstream os(output_path+"/knn_coq_oocover.csv");
			csv_dump c(os);
			for(cv_result_t const& row : results) {
				c.write_row({
					std::to_string(row.knn_params->k),
					round_print(row.metrics.oocover, 5),
				});
			}
		}
	}

	void exporter::exec(std::string const& source_path, std::string const& output_path)
	{
		export_knn(source_path, output_path);
	}
}
