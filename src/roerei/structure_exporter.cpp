#include <roerei/cpp14_fix.hpp>

#include <fstream>
#include <vector>
#include <iterator>
#include <sstream>
#include <algorithm>

#include <roerei/storage.hpp>

#include <roerei/structure_exporter.hpp>
#include <roerei/generic/sparse_unit_matrix.hpp>
#include <roerei/generic/id_t.hpp>

namespace roerei {
  typedef std::vector<std::string> subgroup_t;
  typedef std::pair<subgroup_t, subgroup_t> edge_key_t;

  struct subgroup_id_t : public id_t<subgroup_id_t>
  {
    subgroup_id_t(size_t _id) : id_t(_id) {}
  };

  struct synopsis_t {
    size_t count;
    boost::optional<std::string> source;

    inline synopsis_t& operator+=(synopsis_t const& rhs) {
      count += rhs.count;
      return *this;
    }
  };

  template<typename Out>
  void split(std::string const& s, char delim, Out result) {
      std::stringstream ss;
      ss.str(s);
      std::string item;
      while (std::getline(ss, item, delim)) {
          *(result++) = item;
      }
  }

  subgroup_t split(std::string const& s, char delim) {
    subgroup_t elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
  }

  inline bool is_too_small(subgroup_t const& group, synopsis_t const& s) {
    return false;
    return group.size() > 0 && s.count < 500;
  }

  inline void print_subgroup(std::ostream& os, subgroup_t const& group) {
    bool first = true;
    for(auto u : group) {
      if (first) {
        first = false;
      } else {
        os << '/';
      }

      os << u;
    }
  }

  inline subgroup_t create_subgroup(uri_t const& u) {
    auto us = split(u, '/');
    return subgroup_t(us.begin()+1, us.end());
  }

  void structure_exporter::exec(std::string const& output_path)
  {
    exec_dependencies(output_path);
    exec_features(output_path);
  }

  void structure_exporter::exec_dependencies(std::string const& output_path)
  {
    std::ofstream os(output_path+"/dependencies.dot");

    os << "digraph dependencies {" << std::endl;

    os << "model=\"subset\";" << std::endl;
    os << "splines=\"true\";" << std::endl;
    os << "size=\"16.4, 10.7\";" << std::endl;
    os << "overlap=false;" << std::endl;
    os << "rankdir=\"LR\";" << std::endl;
    os << "root=\"Coq\";" << std::endl;

    auto corpii = {
        "nat.frequency",
        /*"ch2o.frequency",
        "CoRN.frequency",
        "MathClasses.frequency",
        "mathcomp.frequency"*/
    };

    std::map<subgroup_t, synopsis_t> result;
    for(std::string const corpus : corpii) {
        dataset_t d(storage::read_dataset(corpus));
        for(uri_t const& u : d.objects) {
          auto& x = result[create_subgroup(u)];
          x.count = 1;
          x.source = corpus;
        }
        for(uri_t const& u : d.dependencies) {
          result[create_subgroup(u)].count = 1;
        }
    }

    size_t max_depth = std::numeric_limits<size_t>::min();
    for (auto const& kvp : result) {
      max_depth = std::max(max_depth, kvp.first.size());
    }

    for (size_t i = max_depth; i > 0; i--) {
      std::cerr << i << std::endl;
      auto const result_hardcopy = result; // Immutable copy to operate from
      for (auto const& kvp : result_hardcopy) {
        if (kvp.first.size() != i) {
          continue;
        }

        if (!is_too_small(kvp.first, kvp.second)) {
          continue;
        }

        auto us = kvp.first; // Make a copy;
        us.pop_back();
        result[us] += kvp.second;
        result.erase(kvp.first);
      }
    }

    /*size_t i = 0;
    while(std::any_of(result.begin(), result.end(), [](auto const& kvp) {
      return is_too_small(kvp.first, kvp.second);
    })) {
      std::cerr << i << std::endl;
      i++;
      auto const result_hardcopy = result; // Immutable copy to operate from
      for(auto const& kvp : result_hardcopy) {
        if (!is_too_small(kvp.first, kvp.second)) {
          continue;
        }
        print_subgroup(std::cerr, kvp.first);
        std::cerr << std::endl;
        auto us = kvp.first; // Make a copy;
        us.pop_back();
        result[us] += kvp.second;
        result.erase(kvp.first);
      }
    }*/

    encapsulated_vector<subgroup_id_t, subgroup_t> subgroups;
    for (auto const& kvp : result) {
      subgroups.emplace_back(kvp.first);
    }
    std::map<subgroup_t, subgroup_id_t> subgroup_map;
    subgroups.iterate([&](subgroup_id_t id, subgroup_t const& g) {
      subgroup_map.emplace(g, id);
    });

    sparse_unit_matrix_t<subgroup_id_t, subgroup_id_t> edge_matrix(subgroups.size(), subgroups.size());
    std::map<edge_key_t, float> edges;

    auto find_largest_f = [&](subgroup_t g) { // Copy
      while (g.size() > 0) {
        if (result.find(g) != result.end()) {
          break;
        }
        g.pop_back();
      }
      return g;
    };

    for(std::string const corpus : corpii) {
      dataset_t d(storage::read_dataset(corpus));

      d.dependency_matrix.citerate([&](auto const& row) {
        subgroup_t from = find_largest_f(create_subgroup(d.objects[row.row_i]));
        for(auto const& kvp : row) {
          subgroup_t fromNew = from;
          subgroup_t toNew = find_largest_f(create_subgroup(d.dependencies[kvp.first]));;

          subgroup_id_t x(subgroup_map.at(fromNew));
          subgroup_id_t y(subgroup_map.at(toNew));

          if (x.unseal() > y.unseal()) {
            std::swap(x, y);
            std::swap(fromNew, toNew);
          }

          edges[std::make_pair(fromNew, toNew)] += kvp.second;
          edge_matrix.set(std::make_pair(x, y));
        }
      });
    }

    edge_matrix.transitive();
    edge_matrix.transpose();

    std::vector<subgroup_t> gs;
    for(auto const& kvp : result) {
      gs.emplace_back(kvp.first);
    }

    std::stable_sort(gs.begin(), gs.end(), [&](subgroup_t const& x, subgroup_t const& y) {
      return !edge_matrix[std::make_pair(subgroup_map.at(x), subgroup_map.at(y))];
    });

    auto should_parse_f = [](subgroup_t const& g) {
      return (!g.empty()/* && g[0] == "Coq"*/);
    };

    auto should_parse_pair_f = [&](subgroup_t const& x, subgroup_t const& y) {
      return !(x == y || !should_parse_f(x) || !should_parse_f(y));
    };

    for(auto const& g : gs) {
      if (!should_parse_f(g)) {
        continue;
      }

      os << "\"";
      print_subgroup(os, g);
      os << "\";" << std::endl;
    }

    float max_edge_width = std::numeric_limits<float>::min();
    for(auto const& kvp : edges) {
      if (!should_parse_pair_f(kvp.first.first, kvp.first.second)) {
        continue;
      }
      max_edge_width = std::max(max_edge_width, kvp.second);
    }

    auto transform_range_f = [](float value, float max, float desired_max) {
      return (value / max) * desired_max;
    };
    auto cap_f = [](float x, float factor) {
      return std::ceil(x*factor)/factor;
    };

    for(auto const& kvp : edges) {
      /*if (!should_parse_pair_f(kvp.first.first, kvp.first.second) || kvp.second == 0.0f) {
        continue;
      }*/

      os << "\"";
      print_subgroup(os, kvp.first.first);
      os << "\" -> \"";
      print_subgroup(os, kvp.first.second);
      os << "\" [weight=" << kvp.second
        << ", label=\"" << kvp.second << "\""
        //<< ", len=" << cap_f(transform_range_f(1.0f/kvp.second, 1.0f, 20.0f), 10.0f)
        //<< ", penwidth=" << cap_f(transform_range_f(kvp.second, max_edge_width, 5.0f), 1.0f)
        << "];" << std::endl;
    }
    os << "}" << std::endl;
  }

  void structure_exporter::exec_features(std::string const& output_path)
  {
    std::ofstream os(output_path+"/features.dot");

    os << "digraph features {" << std::endl;

    os << "model=\"subset\";" << std::endl;
    os << "splines=\"true\";" << std::endl;
    os << "size=\"16.4, 10.7\";" << std::endl;
    os << "overlap=false;" << std::endl;
    os << "rankdir=\"LR\";" << std::endl;
    os << "root=\"Coq\";" << std::endl;

    auto corpii = {
        "nat.frequency",
        /*"ch2o.frequency",
        "CoRN.frequency",
        "MathClasses.frequency",
        "mathcomp.frequency"*/
    };

    std::map<subgroup_t, synopsis_t> result;
    for(std::string const corpus : corpii) {
        dataset_t d(storage::read_dataset(corpus));
        for(uri_t const& u : d.objects) {
          auto& x = result[create_subgroup(u)];
          x.count = 1;
          x.source = corpus;
        }
        for(uri_t const& u : d.features) {
          result[create_subgroup(u)].count = 1;
        }
    }

    size_t max_depth = std::numeric_limits<size_t>::min();
    for (auto const& kvp : result) {
      max_depth = std::max(max_depth, kvp.first.size());
    }

    for (size_t i = max_depth; i > 0; i--) {
      std::cerr << i << std::endl;
      auto const result_hardcopy = result; // Immutable copy to operate from
      for (auto const& kvp : result_hardcopy) {
        if (kvp.first.size() != i) {
          continue;
        }

        if (!is_too_small(kvp.first, kvp.second)) {
          continue;
        }

        auto us = kvp.first; // Make a copy;
        us.pop_back();
        result[us] += kvp.second;
        result.erase(kvp.first);
      }
    }

    /*size_t i = 0;
    while(std::any_of(result.begin(), result.end(), [](auto const& kvp) {
      return is_too_small(kvp.first, kvp.second);
    })) {
      std::cerr << i << std::endl;
      i++;
      auto const result_hardcopy = result; // Immutable copy to operate from
      for(auto const& kvp : result_hardcopy) {
        if (!is_too_small(kvp.first, kvp.second)) {
          continue;
        }
        print_subgroup(std::cerr, kvp.first);
        std::cerr << std::endl;
        auto us = kvp.first; // Make a copy;
        us.pop_back();
        result[us] += kvp.second;
        result.erase(kvp.first);
      }
    }*/

    encapsulated_vector<subgroup_id_t, subgroup_t> subgroups;
    for (auto const& kvp : result) {
      subgroups.emplace_back(kvp.first);
    }
    std::map<subgroup_t, subgroup_id_t> subgroup_map;
    subgroups.iterate([&](subgroup_id_t id, subgroup_t const& g) {
      subgroup_map.emplace(g, id);
    });

    sparse_unit_matrix_t<subgroup_id_t, subgroup_id_t> edge_matrix(subgroups.size(), subgroups.size());
    std::map<edge_key_t, float> edges;

    auto find_largest_f = [&](subgroup_t g) { // Copy
      while (g.size() > 0) {
        if (result.find(g) != result.end()) {
          break;
        }
        g.pop_back();
      }
      return g;
    };

    for(std::string const corpus : corpii) {
      dataset_t d(storage::read_dataset(corpus));

      d.feature_matrix.citerate([&](auto const& row) {
        subgroup_t from = find_largest_f(create_subgroup(d.objects[row.row_i]));
        for(auto const& kvp : row) {
          subgroup_t fromNew = from;
          subgroup_t toNew = find_largest_f(create_subgroup(d.features[kvp.first]));;

          subgroup_id_t x(subgroup_map.at(fromNew));
          subgroup_id_t y(subgroup_map.at(toNew));

          if (x.unseal() > y.unseal()) {
            std::swap(x, y);
            std::swap(fromNew, toNew);
          }

          edges[std::make_pair(fromNew, toNew)] += kvp.second;
          edge_matrix.set(std::make_pair(x, y));
        }
      });
    }

    edge_matrix.transitive();
    edge_matrix.transpose();

    std::vector<subgroup_t> gs;
    for(auto const& kvp : result) {
      gs.emplace_back(kvp.first);
    }

    std::stable_sort(gs.begin(), gs.end(), [&](subgroup_t const& x, subgroup_t const& y) {
      return !edge_matrix[std::make_pair(subgroup_map.at(x), subgroup_map.at(y))];
    });

    auto should_parse_f = [](subgroup_t const& g) {
      return (!g.empty()/* && g[0] == "Coq"*/);
    };

    auto should_parse_pair_f = [&](subgroup_t const& x, subgroup_t const& y) {
      return !(x == y || !should_parse_f(x) || !should_parse_f(y));
    };

    for(auto const& g : gs) {
      if (!should_parse_f(g)) {
        continue;
      }

      os << "\"";
      print_subgroup(os, g);
      os << "\";" << std::endl;
    }

    float max_edge_width = std::numeric_limits<float>::min();
    for(auto const& kvp : edges) {
      if (!should_parse_pair_f(kvp.first.first, kvp.first.second)) {
        continue;
      }
      max_edge_width = std::max(max_edge_width, kvp.second);
    }

    auto transform_range_f = [](float value, float max, float desired_max) {
      return (value / max) * desired_max;
    };
    auto cap_f = [](float x, float factor) {
      return std::ceil(x*factor)/factor;
    };

    for(auto const& kvp : edges) {
      if (!should_parse_pair_f(kvp.first.first, kvp.first.second) || kvp.second == 0.0f) {
        continue;
      }

      os << "\"";
      print_subgroup(os, kvp.first.first);
      os << "\" -> \"";
      print_subgroup(os, kvp.first.second);
      os << "\" [weight=" << kvp.second
        << ", label=\"" << kvp.second << "\""
        //<< ", len=" << cap_f(transform_range_f(1.0f/kvp.second, 1.0f, 20.0f), 10.0f)
        //<< ", penwidth=" << cap_f(transform_range_f(kvp.second, max_edge_width, 5.0f), 1.0f)
        << "];" << std::endl;
    }
    os << "}" << std::endl;
  }
}
