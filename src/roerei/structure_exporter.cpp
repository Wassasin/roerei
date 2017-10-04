#include <roerei/cpp14_fix.hpp>

#include <fstream>
#include <vector>
#include <iterator>
#include <sstream>
#include <algorithm>

#include <roerei/storage.hpp>

#include <roerei/structure_exporter.hpp>

namespace roerei {
  typedef std::vector<std::string> subgroup_t;
  typedef std::pair<subgroup_t, subgroup_t> edge_key_t;

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
    return group.size() > 1 && s.count < 200;
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
    std::ofstream os(output_path+"/dependencies.dot");

    os << "digraph dependencies {" << std::endl;
    os << "model=\"subset\";" << std::endl;
    os << "remincross=\"true\";" << std::endl;
    os << "size=\"16.4, 10.7\";" << std::endl;
    os << "overlap=false;" << std::endl;
    os << "ratio=fill;" << std::endl;
    os << "root=\"Coq\";" << std::endl;

    auto corpii = {
        "Coq.frequency",
        "ch2o.frequency",
        "CoRN.frequency",
        "MathClasses.frequency",
        "mathcomp.frequency"
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

    size_t i = 0;
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
    }

    std::map<edge_key_t, float> edges;
    auto find_largest_f = [&](subgroup_t g) { // Copy
      while (g.size() > 1) {
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
          subgroup_t to = find_largest_f(create_subgroup(d.dependencies[kvp.first]));
          edges[std::make_pair(to, from)] += kvp.second;
        }
      });
    }

    std::vector<subgroup_t> gs;
    for(auto const& kvp : result) {
      gs.emplace_back(kvp.first);
    }
    std::stable_sort(gs.begin(), gs.end(), [&](subgroup_t const& x, subgroup_t const& y) {
      return edges.find(std::make_pair(x, y)) == edges.end();
    });

    for(auto const& kvp : result) {
      os << "\"";
      print_subgroup(os, kvp.first);
      os << "\";" << std::endl;
    }

    for(auto const& kvp : edges) {
      if (kvp.first.first == kvp.first.second || kvp.second == 0.0f) {
        continue;
      }

      os << "\"";
      print_subgroup(os, kvp.first.first);
      os << "\" -> \"";
      print_subgroup(os, kvp.first.second);
      os << "\";" << std::endl;
    }
    os << "}" << std::endl;
  }
}