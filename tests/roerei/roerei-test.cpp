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

START_TEST(test_sparse_unit_matrix_transitive)
{
  roerei::sparse_unit_matrix_t<roerei::object_id_t, roerei::object_id_t> m(4, 4);
  m.set(std::make_pair(0, 1));
  m.set(std::make_pair(0, 2));
  m.set(std::make_pair(1, 2));
  m.set(std::make_pair(2, 0));
  m.set(std::make_pair(2, 3));
  m.set(std::make_pair(3, 3));

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

START_TEST(test_sparse_unit_matrix_non_cyclic)
{
  roerei::sparse_unit_matrix_t<roerei::object_id_t, roerei::object_id_t> m(5, 5);
  m.set(std::make_pair(0, 1));
  m.set(std::make_pair(0, 2));
  m.set(std::make_pair(2, 1));
  m.set(std::make_pair(1, 3));
  m.set(std::make_pair(3, 4));
  m.set(std::make_pair(2, 4));

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
