#include <cstdlib>
#include <memory>

#include <roerei/sparse_matrix.hpp>
#include <roerei/sliced_sparse_matrix.hpp>
#include <roerei/compact_sparse_matrix.hpp>

#include <roerei/id_t.hpp>

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

Suite* roerei_suite(void)
{
	Suite* s = suite_create("roerei");
	TCase *tc_core = tcase_create("Core");

	tcase_add_test(tc_core, test_matrix_arr_eq);
	tcase_add_test(tc_core, test_matrix_iter_eq);
	tcase_add_test(tc_core, test_sliced_matrix_iter_eq);
	tcase_add_test(tc_core, test_compact_matrix_iter_eq);

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
