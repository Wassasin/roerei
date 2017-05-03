#include <roerei/cpp14_fix.hpp>

#include <roerei/exporter.hpp>

#include <roerei/storage.hpp>
#include <roerei/export/latex_tabular.hpp>
#include <roerei/export/data_dump.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>

namespace roerei
{
	inline static std::string round_print(float f, size_t digits)
	{
		std::stringstream ss;

		size_t factor = std::pow(10, digits);
		ss << std::round(f*factor)/factor;

		std::string result = ss.str();
		size_t split = result.find('.');

		if(split == result.npos) {
			if(digits > 0) {
				ss << '.';
				split = result.size()-1;
			} else {
				return result;
			}
		}

		for(size_t i = digits - (result.size() - 1 - split); i > 0; i--)
		{
			ss << '0';
		}

		return ss.str();
	}

	inline static std::string make_prefix(std::string const& dataset)
	{
		std::string result(dataset);
		std::transform(result.begin(), result.end(), result.begin(), [](const int c) {
			if(c == '.') {
				return static_cast<int>('-');
			}
			return ::tolower(c);
		});
		return result;
	}

	void export_knn(
		std::string const& dataset,
		std::string const& source_path,
		std::string const& output_path
	)
	{
		std::vector<cv_result_t> results;
		storage::read_result([&](cv_result_t const& result) {
			if(result.ml == ml_type::knn && result.strat == posetcons_type::pessimistic && result.corpus == dataset) {
				results.emplace_back(result);
			}
		}, source_path);

		std::sort(results.begin(), results.end(), [](cv_result_t const& x, cv_result_t const& y) {
			return *x.knn_params < *y.knn_params;
		});

		{
			std::ofstream os(output_path+"/knn-"+make_prefix(dataset)+".tex");
			latex_tabular t(os);
			
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
			std::ofstream os(output_path+"/knn-"+make_prefix(dataset)+".dat");
			data_dump dd(os);
			for(cv_result_t const& row : results) {
				dd.write_row({
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
	}
	
	void export_nb(
		std::string const& dataset,
		std::string const& source_path,
		std::string const& output_path
	)
	{
		std::vector<cv_result_t> results;
		storage::read_result([&](cv_result_t const& result) {
			if(result.ml == ml_type::naive_bayes && result.strat == posetcons_type::pessimistic && result.corpus == dataset) {
				results.emplace_back(result);
			}
		}, source_path);

		std::sort(results.begin(), results.end(), [](cv_result_t const& x, cv_result_t const& y) {
			return *x.nb_params < *y.nb_params;
		});

		auto it = std::unique(results.begin(), results.end(), [](cv_result_t const& x, cv_result_t const& y) {
			return *x.nb_params == *y.nb_params;
		});
		results.erase(it, results.end());

		{
			std::set<std::pair<float, float>> range_pi_tau;
			for(float pi = 0; pi < 40; pi+=4) {
				for(float tau = -20; tau < 20; tau+=4) {
					range_pi_tau.emplace(pi, tau);
				}
			}

			std::ofstream os(output_path+"/nb-"+make_prefix(dataset)+"-pi-tau.dat");
			data_dump dd(os);
			for(cv_result_t const& row : results) {
				if(row.nb_params->sigma != -15) {
					continue;
				}
				if(range_pi_tau.erase(std::make_pair(row.nb_params->pi, row.nb_params->tau)) == 0) {
					continue;
				}
				dd.write_row({
					round_print(row.nb_params->pi, 0),
					round_print(row.nb_params->tau, 0),
					round_print(row.metrics.oocover, 3),
					round_print(row.metrics.ooprecision, 3),
					round_print(row.metrics.recall, 3),
					round_print(row.metrics.rank, 1),
					round_print(row.metrics.auc, 3),
					round_print(row.metrics.volume, 3)
				});
			}

			if(range_pi_tau.size() > 0) {
				std::cerr << "nb pi_tau for " << dataset << " is missing " << range_pi_tau.size() << " elements" << std::endl;
			}
		}
		
		{
			std::set<std::pair<float, float>> range_pi_sigma;
			for(float pi = 0; pi < 40; pi+=4) {
				for(float sigma = -20; sigma < 20; sigma+=4) {
					range_pi_sigma.emplace(pi, sigma);
				}
			}

			std::ofstream os(output_path+"/nb-"+make_prefix(dataset)+"-pi-sigma.dat");
			data_dump dd(os);
			for(cv_result_t const& row : results) {
				if(row.nb_params->tau != 0) {
					continue;
				}
				if(range_pi_sigma.erase(std::make_pair(row.nb_params->pi, row.nb_params->sigma)) == 0) {
					continue;
				}
				dd.write_row({
					round_print(row.nb_params->pi, 0),
					round_print(row.nb_params->sigma, 0),
					round_print(row.metrics.oocover, 3),
					round_print(row.metrics.ooprecision, 3),
					round_print(row.metrics.recall, 3),
					round_print(row.metrics.rank, 1),
					round_print(row.metrics.auc, 3),
					round_print(row.metrics.volume, 3)
				});
			}

			if(range_pi_sigma.size() > 0) {
				std::cerr << "nb pi_sigma for " << dataset << " is missing " << range_pi_sigma.size() << " elements" << std::endl;
			}
		}
	}

  std::ostream& describe_ml(std::ostream& os, cv_result_t const& rhs)
  {
    switch(rhs.ml)
    {
    case ml_type::knn:
      return os << "\\knn K=" << rhs.knn_params->k;
    case ml_type::knn_adaptive:
      return os << "\\knn (adaptive)";
    case ml_type::omniscient:
      return os << "\\omniscient";
    case ml_type::ensemble:
      return os << "\\ensemble";
    case ml_type::naive_bayes:
      return os << "\\nb $\\pi=" << rhs.nb_params->pi << "$ $\\sigma=" << rhs.nb_params->sigma << "$ $\\tau=" << rhs.nb_params->tau << "$";
    }
  }

  void export_best(
      std::string const& dataset,
      std::string const& source_path,
      std::string const& output_path
  )
  {
    std::map<ml_type, cv_result_t> results;
		storage::read_result([&](cv_result_t const& result) {
			if(result.strat == posetcons_type::pessimistic && result.corpus == dataset) {
        auto it = results.find(result.ml);
        if(it != results.end() && it->second.metrics.oocover >= result.metrics.oocover) {
          return;
        } 

        results[result.ml] = result;
			}
		}, source_path);

    std::ofstream os(output_path+"/best-"+make_prefix(dataset)+".tex");
    latex_tabular t(os);
    
    for(auto const& kvp : results) {
      auto const& row = kvp.second;

      std::stringstream ss;
      describe_ml(ss, row);
      t.write_row({
        ss.str(),
        round_print(row.metrics.oocover, 3),
        round_print(row.metrics.ooprecision, 3),
        round_print(row.metrics.recall, 3),
        round_print(row.metrics.rank, 1),
        round_print(row.metrics.auc, 3),
        round_print(row.metrics.volume, 3)
      });
    }
  }

  void export_counts(std::string const& output_path)
  {
      std::ofstream os(output_path+"/counts.tex");
      latex_tabular t(os);

      auto corpii = {
          "Coq.frequency",
          "ch2o.frequency",
          "CoRN.frequency",
          "MathClasses.frequency",
          "mathcomp.frequency"
      };

      for(std::string const corpus : corpii) {
          dataset_t d(storage::read_dataset(corpus));
          t.write_row({
                corpus,
                round_print(d.objects.size(), 0),
                round_print(d.features.size(), 0),
                round_print(d.dependencies.size(), 0),
          });
      }
  }

	void exporter::exec(std::string const& source_path, std::string const& output_path)
	{
        export_counts(output_path);

		export_knn("Coq.flat", source_path, output_path);
		export_knn("Coq.depth", source_path, output_path);
		export_knn("Coq.frequency", source_path, output_path);
		
		export_knn("ch2o.frequency", source_path, output_path);
		export_knn("CoRN.frequency", source_path, output_path);
		export_knn("MathClasses.frequency", source_path, output_path);
		export_knn("mathcomp.frequency", source_path, output_path);

		export_knn("ch2o.depth", source_path, output_path);
		export_knn("CoRN.depth", source_path, output_path);
		export_knn("MathClasses.depth", source_path, output_path);
		export_knn("mathcomp.depth", source_path, output_path);
		
		export_nb("Coq.frequency", source_path, output_path);
		export_nb("Coq.depth", source_path, output_path);
		export_nb("CoRN.frequency", source_path, output_path);

		export_best("Coq.frequency", source_path, output_path);
	}
}
