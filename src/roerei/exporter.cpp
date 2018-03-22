#include <roerei/cpp14_fix.hpp>

#include <roerei/exporter.hpp>

#include <roerei/storage.hpp>
#include <roerei/export/latex_tabular.hpp>
#include <roerei/export/data_dump.hpp>

#include <roerei/structure_exporter.hpp>
#include <roerei/ml/cv.hpp>

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

  inline static std::string make_prefix(std::string const& dataset, posetcons_type const p)
	{
		std::string result(dataset);
		std::transform(result.begin(), result.end(), result.begin(), [](const int c) {
			if(c == '.') {
				return static_cast<int>('-');
			}
			return ::tolower(c);
		});
    return result + '-' + to_string(p);
	}

	void export_knn(
		std::string const& dataset,
		std::string const& source_path,
    std::string const& output_path,
		posetcons_type const p = posetcons_type::canonical,
    bool const prior = true,
    size_t const cv_n = cv::default_n,
    size_t const cv_k = cv::default_k
	)
	{
		std::vector<cv_result_t> results;
		storage::read_result([&](cv_result_t const& result) {
      if(result.n == cv_n && result.k == cv_k && result.prior == prior && result.ml == ml_type::knn && result.strat == p && result.corpus == dataset) {
				results.emplace_back(result);
			}
		}, source_path);

		std::sort(results.begin(), results.end(), [](cv_result_t const& x, cv_result_t const& y) {
			return *x.knn_params < *y.knn_params;
		});
		
		std::string prior_str = prior ? "" : "nonprior-";

		{
      std::ofstream os(output_path+"/knn-"+prior_str+make_prefix(dataset, p)+".dat");
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

  void export_adarank(
      std::string const& dataset,
      std::string const& source_path,
      std::string const& output_path,
      posetcons_type const p = posetcons_type::canonical
    )
    {
        std::vector<cv_result_t> results;
        storage::read_result([&](cv_result_t const& result) {
            if(result.n == cv::default_n && result.k == cv::default_k &&
               result.prior && result.ml == ml_type::adarank && result.strat == p && result.corpus == dataset) {
                results.emplace_back(result);
            }
        }, source_path);

        std::sort(results.begin(), results.end(), [](cv_result_t const& x, cv_result_t const& y) {
            return *x.adarank_params < *y.adarank_params;
        });

        {
            std::ofstream os(output_path+"/adarank-"+make_prefix(dataset, p)+".tex");
            latex_tabular t(os);

            for(cv_result_t const& row : results) {
                t.write_row({
                    std::to_string(row.adarank_params->T),
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
            std::ofstream os(output_path+"/adarank-"+make_prefix(dataset, p)+".dat");
            data_dump dd(os);
            for(cv_result_t const& row : results) {
                dd.write_row({
                    std::to_string(row.adarank_params->T),
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
	
	template<typename F>
	void select_best(boost::optional<cv_result_t>& result, cv_result_t const& current, F&& f)
	{
		if (!result || f(*result) < f(current)) {
			result = current;
		}
	}

	void export_nb(
		std::string const& dataset,
		std::string const& source_path,
    std::string const& output_path,
    posetcons_type const p = posetcons_type::canonical
	)
	{
		std::vector<cv_result_t> results;
		storage::read_result([&](cv_result_t const& result) {
      if(result.n == cv::default_n && result.k == cv::default_k &&
         result.prior && result.ml == ml_type::naive_bayes && result.strat == p && result.corpus == dataset) {
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
      for(float pi = 0; pi <= 160; pi+=8) {
        for(float tau = -20; tau <= 20; tau+=2) {
					range_pi_tau.emplace(pi, tau);
				}
			}

			{
        std::ofstream os(output_path+"/nb-"+make_prefix(dataset, p)+"-pi-tau.dat");
				data_dump dd(os);
        boost::optional<cv_result_t> best_result;
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

					select_best(best_result, row, [](cv_result_t r) { return r.metrics.oocover; });
				}

        if (best_result) {
					auto const& rowbest = *best_result;
					std::ofstream osbest(output_path+"/nb-"+make_prefix(dataset, p)+"-pi-tau-best-oocover.tex");
					osbest << "\\node[draw, circle, very thick, fill=white, inner sep=1.5pt] (P) at ("
						<< round_print(rowbest.nb_params->pi, 0) << ","
						<< round_print(rowbest.nb_params->tau, 0) << ") {};";
						osbest.flush();
				}
			}

			if(range_pi_tau.size() > 0) {
				std::cerr << "nb pi_tau for " << dataset << " is missing " << range_pi_tau.size() << " elements" << std::endl;
				for (auto kvp : range_pi_tau) {
					std::cerr << "\tpi " << kvp.first << ", tau " << kvp.second << std::endl;
				}
			}
		}
		
		{
			std::set<std::pair<float, float>> range_pi_sigma;
      for(float pi = 0; pi <= 160; pi+=8) {
        for(float sigma = -20; sigma <= 20; sigma+=2) {
					range_pi_sigma.emplace(pi, sigma);
				}
			}

      std::ofstream os(output_path+"/nb-"+make_prefix(dataset, p)+"-pi-sigma.dat");
			data_dump dd(os);
      boost::optional<cv_result_t> best_result;
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

				select_best(best_result, row, [](cv_result_t r) { return r.metrics.oocover; });
			}

      if (best_result) {
        auto const& rowbest = *best_result;
				std::ofstream osbest(output_path+"/nb-"+make_prefix(dataset, p)+"-pi-sigma-best-oocover.tex");
				osbest << "\\node[draw, circle, very thick, fill=white, inner sep=1.5pt] (P) at ("
					<< round_print(rowbest.nb_params->pi, 0) << ","
					<< round_print(rowbest.nb_params->sigma, 0) << ") {};";
					osbest.flush();
      }

			if(range_pi_sigma.size() > 0) {
				std::cerr << "nb pi_sigma for " << dataset << " is missing " << range_pi_sigma.size() << " elements" << std::endl;
				for (auto kvp : range_pi_sigma) {
					std::cerr << "\tpi " << kvp.first << ", sigma " << kvp.second << std::endl;
				}
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
    case ml_type::adarank:
      return os << "\\adarank T=" << rhs.adarank_params->T;
    }
  }

  void export_best(
      std::string const& dataset,
      std::string const& source_path,
      std::string const& output_path,
      posetcons_type const p = posetcons_type::canonical,
      size_t const cv_n = cv::default_n,
      size_t const cv_k = cv::default_k
  )
  {
    std::map<ml_type, cv_result_t> results;
		storage::read_result([&](cv_result_t const& result) {
      if(result.n == cv_n && result.k == cv::default_k && result.prior && result.strat == p && result.corpus == dataset) {
        auto it = results.find(result.ml);
        if(it != results.end() && it->second.metrics.oocover >= result.metrics.oocover) {
          return;
        } 

        results[result.ml] = result;
			}
		}, source_path);

    std::ofstream os(output_path+"/best-"+make_prefix(dataset, p)+".tex");
    latex_tabular t(os);

		auto emit_f = [&](auto const& row) {
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
		};

		for(ml_type mlt : {ml_type::knn, ml_type::knn_adaptive, ml_type::naive_bayes, ml_type::adarank, ml_type::ensemble, ml_type::omniscient}) {
			auto it = results.find(mlt);
			if (it == results.end()) {
				throw std::runtime_error(std::string("Could not find ") + to_string(mlt) + " for " + dataset);
			}

			emit_f(it->second);
		}
	}

	void export_prior(
		std::string const& dataset,
		std::string const& source_path,
    std::string const& output_path,
		posetcons_type const p = posetcons_type::canonical
	)
	{
		auto find_overwrite_f = [](std::map<ml_type, cv_result_t>& results, cv_result_t const& result) {
			auto it = results.find(result.ml);
			if(it != results.end() && it->second.metrics.oocover >= result.metrics.oocover) {
				return;
			}

			results[result.ml] = result;
		};

		std::map<ml_type, cv_result_t> prior_results, nonprior_results;
		storage::read_result([&](cv_result_t const& result) {
      if(result.strat == p && result.corpus == dataset && result.ml != ml_type::omniscient) {
        if (result.prior) {
					find_overwrite_f(prior_results, result);
				} else {
					find_overwrite_f(nonprior_results, result);
				}
			}
		}, source_path);

    std::ofstream os(output_path+"/prior-vs-nonprior-"+make_prefix(dataset, p)+".tex");
    latex_tabular t(os);

		auto emit_f = [&](auto const& prior, auto const& nonprior) {
      {
        std::stringstream ss;
        describe_ml(ss, prior);
        t.write_row({ss.str(), std::string("prior"),
          round_print(prior.metrics.oocover, 3),
          round_print(prior.metrics.auc, 3)
        });
      }

      {
        float oocover_ratio = 1.0f - prior.metrics.oocover / nonprior.metrics.oocover;
        float auc_ratio = 1.0f - prior.metrics.auc / nonprior.metrics.auc;

        std::string oocover_color = oocover_ratio >= 0.0f ? "green" : "red";
        std::string auc_color = auc_ratio >= 0.0f ? "green" : "red";

        std::stringstream ss;
        cv_result_t nonprior_faux = nonprior;
        nonprior_faux.prior = true;
        if (!result_same_base(prior, nonprior_faux)) {
          describe_ml(ss, nonprior);
        }

        auto round_print_sign = [&](float x, float n) {
          return (x < 0.0f ? std::string("") : std::string("+")) + round_print(x, n);
        };

        t.write_row({ss.str(), std::string("without"),
          round_print(nonprior.metrics.oocover, 3) + " ({\\color{" + oocover_color + "}" + round_print_sign(oocover_ratio * 100.0f, 1) + "\\%})",
          round_print(nonprior.metrics.auc, 3) + " ({\\color{" + auc_color + "}" + round_print_sign(auc_ratio * 100.0f, 1) + "\\%})"
        });
      }
		};

		for(ml_type mlt : {ml_type::knn, ml_type::knn_adaptive, ml_type::naive_bayes, ml_type::adarank, ml_type::ensemble}) {
			auto prior_it = prior_results.find(mlt);
			auto nonprior_it = nonprior_results.find(mlt);
			if (prior_it == prior_results.end() || nonprior_it == nonprior_results.end()) {
				throw std::runtime_error(std::string("Could not find ") + to_string(mlt) + " for " + dataset + " (prior or nonprior)");
			}

			emit_f(prior_it->second, nonprior_it->second);
		}
	}

  void export_counts(std::string const& output_path)
  {
      std::ofstream os(output_path+"/counts.tex");
      latex_tabular t(os);

			auto process_f = [&](std::initializer_list<std::string>&& list) {
				std::vector<std::string> corpus(list);
				dataset_t d(storage::read_dataset(corpus[0]));
				t.write_row({
							std::string("\\") + corpus[1],
							corpus[2],
							corpus[3],
							round_print(d.objects.size(), 0),
							round_print(d.features.size(), 0),
							round_print(d.dependencies.size(), 0),
				});
			};

      process_f({"Coq.frequency", "coq", "b705cf0", "april 2015"});
      process_f({"ch2o.frequency", "formalin", "64d98fa", "november 2015"});
			process_f({"MathClasses.frequency", "mathclasses", "751e63b", "june 2016"});
			process_f({"CoRN.frequency", "corn", "4860de7", "september 2009"});
      process_f({"mathcomp.frequency", "mathcomp", "9e81c8f", "december 2015"});
	}
	
	void export_relative_kaliszyk(std::string const& source_path, std::string const& output_path)
	{
		std::ofstream os(output_path+"/relative.tex");
		latex_tabular t(os);

		std::map<ml_type, cv_result_t> results;
		storage::read_result([&](cv_result_t const& result) {
      if(result.prior && result.strat == posetcons_type::canonical && result.corpus == "CoRN.frequency") {
        auto it = results.find(result.ml);
        if(it != results.end() && it->second.metrics.oocover >= result.metrics.oocover) {
          return;
        } 

        results[result.ml] = result;
			}
		}, source_path);

		auto emit_row_f = [&](ml_type ml, std::string const& ml_str, float oocover, float auc, float show_theirs = true) {
			float oocover_ratio = (results[ml].metrics.oocover - oocover) / oocover;
			float auc_ratio = (results[ml].metrics.auc - auc) / auc;

			std::string oocover_color = oocover_ratio >= 0.0f ? "green" : "red";
			std::string auc_color = auc_ratio >= 0.0f ? "green" : "red";

			auto round_print_sign = [&](float x, float n) {
				return (x < 0.0f ? std::string("") : std::string("+")) + round_print(x, n);
			};

			std::string oocover_str = show_theirs ? round_print(oocover, 3) : "-";
			std::string auc_str = show_theirs ? round_print(auc, 3) : "-";

			t.write_row({
				std::string("\\") + ml_str,
				oocover_str,
				round_print(results[ml].metrics.oocover, 3),
				std::string("({\\color{") + oocover_color + "}" + round_print_sign(oocover_ratio * 100.0f, 1) + "\\%})",
				auc_str,
				round_print(results[ml].metrics.auc, 3),
				std::string("({\\color{") + auc_color + "}" + round_print_sign(auc_ratio * 100.0f, 1) + "\\%})"
			});
		};

		emit_row_f(ml_type::knn_adaptive, "knnadaptive", 0.723, 0.834);
		emit_row_f(ml_type::naive_bayes, "nb", 0.726, 0.836);
		emit_row_f(ml_type::ensemble, "ensemble", 0.749, 0.849);
		emit_row_f(ml_type::adarank, "adarank", 0.749, 0.849, false);
	}

	void exporter::exec(std::string const& source_path, std::string const& output_path)
	{
		/*export_counts(output_path);

    // Prior vs non-prior
		export_prior("CoRN.frequency", source_path, output_path);
		export_knn("CoRN.frequency", source_path, output_path, posetcons_type::pessimistic, false);

    // Flat vs depth vs frequency
		export_knn("CoRN.flat", source_path, output_path);
		export_knn("CoRN.depth", source_path, output_path);
		export_knn("CoRN.frequency", source_path, output_path);
		
    // Strategy: pessimistic, canonical, optimistic
    // Duplicate export_knn("CoRN.frequency", source_path, output_path, posetcons_type::pessimistic);
    export_knn("CoRN.frequency", source_path, output_path, posetcons_type::optimistic);
    export_knn("CoRN.frequency", source_path, output_path, posetcons_type::canonical);

    // Methods: KNN for each corpora (in K)
    export_knn("CoRN.frequency", source_path, output_path);
    export_knn("Coq.frequency", source_path, output_path);
    export_knn("ch2o.frequency", source_path, output_path);
		export_knn("MathClasses.frequency", source_path, output_path);
		export_knn("mathcomp.frequency", source_path, output_path);

    // Methods: NB (in pi & sigma)
		export_nb("CoRN.frequency", source_path, output_path);

    // Methods: Adarank (in T)
    export_adarank("Coq.frequency", source_path, output_path);
    export_adarank("ch2o.frequency", source_path, output_path);
		export_adarank("CoRN.frequency", source_path, output_path);
    export_adarank("MathClasses.frequency", source_path, output_path);
    export_adarank("mathcomp.frequency", source_path, output_path);

    // Corpora: for each method
		export_best("Coq.frequency", source_path, output_path);
		export_best("ch2o.frequency", source_path, output_path);
		export_best("CoRN.frequency", source_path, output_path);
		export_best("MathClasses.frequency", source_path, output_path);
		export_best("mathcomp.frequency", source_path, output_path);

		export_relative_kaliszyk(source_path, output_path);*/

		structure_exporter::exec(output_path);
	}
}
