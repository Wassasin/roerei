#include <roerei/cpp14_fix.hpp>

#include <roerei/generic/sparse_matrix.hpp>
#include <roerei/generic/sliced_sparse_matrix.hpp>
#include <roerei/generic/compact_sparse_matrix.hpp>
#include <roerei/generic/sparse_unit_matrix.hpp>

#include <roerei/generic/id_t.hpp>

#include <cstdlib>
#include <memory>
#include <random>
#include <iostream>
#include <algorithm>

#include <check.h>

std::map<std::pair<roerei::object_id_t, roerei::object_id_t>, uint16_t> create_mat(size_t m, size_t n, size_t c)
{
	std::map<std::pair<roerei::object_id_t, roerei::object_id_t>, uint16_t> result;

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> i_dis(0, m-1);
	std::uniform_int_distribution<> j_dis(0, n-1);
	std::uniform_int_distribution<> v_dis(0, std::numeric_limits<uint16_t>::max());

	for(size_t i = 0; i < c; ++i)
	{
		std::pair<roerei::object_id_t, roerei::object_id_t> coord(roerei::object_id_t(i_dis(gen)), roerei::object_id_t(j_dis(gen)));
		if(!result.emplace(coord, v_dis(gen)).second)
			--i;
	}

	return result;
}

START_TEST(test_matrix_arr_eq) // Preserve values after init with array access
{
	size_t const m = 1000, n = 1000;

	auto values = create_mat(m, n, 10000);

	roerei::sparse_matrix_t<roerei::object_id_t, roerei::object_id_t, uint16_t> mat(m, n);

	for(auto coord : values)
		mat[coord.first.first][coord.first.second] = coord.second;

	for(auto coord : values)
		ck_assert_int_eq(coord.second, mat[coord.first.first][coord.first.second]);
}
END_TEST

START_TEST(test_matrix_iter_eq) // Preserve values after init with iter access
{
	size_t const m = 1000, n = 1000, c = 10000;

	auto values = create_mat(m, n, c);

	roerei::sparse_matrix_t<roerei::object_id_t, roerei::object_id_t, uint16_t> mat(m, n);

	for(auto coord : values)
		mat[coord.first.first][coord.first.second] = coord.second;

	mat.iterate([&](decltype(mat)::row_proxy_t const& row) {
		for(auto value_kvp : row)
		{
			auto it = values.find(std::make_pair(row.row_i, value_kvp.first));
			ck_assert_int_eq(it->second, value_kvp.second);
			values.erase(it);
		}
	});

	ck_assert(values.empty());
}
END_TEST

START_TEST(test_sliced_matrix_iter_eq) // Preserve values after init with iter access
{
	size_t const m = 1000, n = 1000, c = 10000;

	auto values = create_mat(m, n, c);

	roerei::sparse_matrix_t<roerei::object_id_t, roerei::object_id_t, uint16_t> mat(m, n);

	for(auto coord : values)
		mat[coord.first.first][coord.first.second] = coord.second;

	roerei::sliced_sparse_matrix_t<decltype(mat)> mat_copy(mat);
	for(size_t i = 0; i < m; ++i)
		mat_copy.add_key(i);

	mat_copy.citerate([&](decltype(mat_copy)::const_row_proxy_t const& row) {
		for(auto value_kvp : row)
		{
			auto it = values.find(std::make_pair(row.row_i, value_kvp.first));
			ck_assert_int_eq(it->second, value_kvp.second);
			values.erase(it);
		}
	});

	//for(auto kvp : values)
	//	std::cerr << kvp.first.first << "," << kvp.first.second << ": " << kvp.second << std::endl;

	ck_assert(values.empty());
}
END_TEST

START_TEST(test_compact_matrix_iter_eq) // Preserve values after init with iter access
{
	size_t const m = 1000, n = 1000, c = 10000;

	auto values = create_mat(m, n, c);

	roerei::sparse_matrix_t<roerei::object_id_t, roerei::object_id_t, uint16_t> mat(m, n);

	for(auto coord : values)
		mat[coord.first.first][coord.first.second] = coord.second;

	roerei::compact_sparse_matrix_t<roerei::object_id_t, roerei::object_id_t, uint16_t> mat_copy(mat);

	mat_copy.citerate([&](decltype(mat_copy)::const_row_proxy_t const& row) {
		for(auto value_kvp : row)
		{
			auto it = values.find(std::make_pair(row.row_i, value_kvp.first));
			ck_assert_int_eq(it->second, value_kvp.second);
			values.erase(it);
		}
	});

	//for(auto kvp : values)
	//	std::cerr << kvp.first.first << "," << kvp.first.second << ": " << kvp.second << std::endl;

	ck_assert(values.empty());
}
END_TEST

auto create_default_cyclic()
{
    roerei::sparse_unit_matrix_t<roerei::object_id_t, roerei::object_id_t> m(4, 4);
    m.set(std::make_pair(0, 1));
    m.set(std::make_pair(0, 2));
    m.set(std::make_pair(1, 2));
    m.set(std::make_pair(2, 0));
    m.set(std::make_pair(2, 3));
    m.set(std::make_pair(3, 3));
    return m;
}

START_TEST(test_sparse_unit_matrix_transitive)
{
  auto m(create_default_cyclic());

  m.transitive();

  for(size_t x = 0; x < 3; ++x) {
      for(size_t y = 0; y < 4; ++y) {
      ck_assert(m[std::make_pair(x, y)]);
    }
  }

  for(size_t y = 0; y < 3; ++y) {
    ck_assert(!m[std::make_pair(3, y)]);
  }

  ck_assert(m[std::make_pair(3, 3)]);
}
END_TEST

auto create_default_non_cyclic()
{
  roerei::sparse_unit_matrix_t<roerei::object_id_t, roerei::object_id_t> m(5, 5);
  m.set(std::make_pair(0, 1));
  m.set(std::make_pair(0, 2));
  m.set(std::make_pair(2, 1));
  m.set(std::make_pair(1, 3));
  m.set(std::make_pair(3, 4));
  m.set(std::make_pair(2, 4));
  return m;
}

auto generate_dag(size_t const elements, size_t const depth, size_t const edges)
{
  typedef size_t rank_t;
  std::mt19937 gen(1337);

  roerei::sparse_unit_matrix_t<roerei::object_id_t, roerei::object_id_t> m(elements, elements);

  roerei::encapsulated_vector<roerei::object_id_t, rank_t> rank_map;
  for(rank_t r = 0; r < depth; ++r) {
    for(size_t i = 0; i <= elements/depth && rank_map.size() < elements; ++i) {
      rank_map.emplace_back(r);
    }
  }

  std::shuffle(rank_map.begin(), rank_map.end(), gen);

  std::map<rank_t, std::vector<roerei::object_id_t>> ranks;
  rank_map.iterate([&ranks](roerei::object_id_t i, rank_t r) {
    ranks[r].emplace_back(i);
  });

  for(size_t i = 0; i < edges; ++i) {
    do {
      std::uniform_int_distribution<> from_dist(0, depth-2);
      rank_t a = from_dist(gen);
      std::uniform_int_distribution<> to_dist(a+1, depth-1);
      rank_t b = to_dist(gen);

      if (ranks[a].empty() || ranks[b].empty()) {
        continue;
      }

      auto get_random_from_rank_f = [&](rank_t const r) {
        ck_assert(ranks[r].size() > 0);
        std::uniform_int_distribution<> dist(0, ranks[r].size()-1);
        return ranks[r][dist(gen)];
      };

      roerei::object_id_t x(get_random_from_rank_f(a));
      roerei::object_id_t y(get_random_from_rank_f(b));

      if (m[std::make_pair(x, y)]) {
        continue;
      }

      m.set(std::make_pair(x, y));
      break;
    } while (true);
  }

  return m;
}

START_TEST(test_sparse_unit_matrix_non_cyclic)
{
  auto m(create_default_non_cyclic());

  m.transitive();

  for(size_t i = 0; i < 5; ++i) {
    m.set(std::make_pair(i, i), false);
  }

  ck_assert(!m.find_cyclic([](auto) {}));

  std::vector<size_t> xs(5);
  std::iota(xs.begin(), xs.end(), 0);
  std::reverse(xs.begin(), xs.end());

  std::stable_sort(xs.begin(), xs.end(), [&](size_t i, size_t j) {
    return m[std::make_pair(i, j)];
  });

  std::vector<size_t> ys({0, 2, 1, 3, 4});
  ck_assert(xs == ys);
}
END_TEST

START_TEST(test_topological_sort)
{
  auto m(generate_dag(1000, 500, 3000));
  m.transitive();

  for(size_t i = 0; i < m.size_m(); ++i) {
    m.set(std::make_pair(i, i), false);
  }

  std::vector<roerei::object_id_t> xs;
  m.topological_sort([&xs](roerei::object_id_t x) {
    xs.emplace_back(x);
  });
  std::reverse(xs.begin(), xs.end());

  auto comp_f = [&](roerei::object_id_t i, roerei::object_id_t j) {
    return m[std::make_pair(i, j)];
  };

  ck_assert(std::is_sorted(xs.begin(), xs.end(), comp_f));

  for(size_t x = 0; x < m.size_m(); ++x) {
    for(size_t y = x+1; y < m.size_m(); ++y) {
      ck_assert(!comp_f(xs[y], xs[x]));
    }
  }
}
END_TEST

START_TEST(test_transitive)
{
  auto m(generate_dag(1000, 500, 3000));
  m.transitive();

  auto n(m); // Copy
  n.transitive();

  for(size_t i = 0; i < m.size_m(); ++i) {
    if (m[i] != n[i]) {
      for(auto x : m[i]) {
        std::cout << x.unseal() << ' ';
      }
      std::cout << std::endl;
      for(auto y : n[i]) {
        std::cout << y.unseal() << ' ';
      }
      std::cout << std::endl;
    }
    ck_assert(m[i] == n[i]);
  }
}
END_TEST

Suite* roerei_suite(void)
{
	Suite* s = suite_create("roerei");
	TCase *tc_core = tcase_create("Core");

	tcase_add_test(tc_core, test_matrix_arr_eq);
	tcase_add_test(tc_core, test_matrix_iter_eq);
	tcase_add_test(tc_core, test_sliced_matrix_iter_eq);
	tcase_add_test(tc_core, test_compact_matrix_iter_eq);
  tcase_add_test(tc_core, test_sparse_unit_matrix_transitive);
  tcase_add_test(tc_core, test_sparse_unit_matrix_non_cyclic);
  tcase_add_test(tc_core, test_topological_sort);
  tcase_add_test(tc_core, test_transitive);

	suite_add_tcase(s, tc_core);

	return s;
}

int main()
{
	std::unique_ptr<SRunner, decltype(srunner_free)*> sr(srunner_create(roerei_suite()), &srunner_free);
	srunner_run_all(sr.get(), CK_NORMAL);
	int number_failed = srunner_ntests_failed(sr.get());
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
