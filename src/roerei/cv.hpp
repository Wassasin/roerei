#pragma once

#include <roerei/performance.hpp>
#include <roerei/knn.hpp>
#include <roerei/partition.hpp>
#include <roerei/dataset.hpp>

#include <roerei/sliced_sparse_matrix.hpp>

namespace roerei
{

class cv
{
private:
	cv() = delete;

	template<typename F>
	static void _combs_helper(size_t const n, size_t const k, F const& yield, size_t offset, std::vector<size_t>& buf)
	{
		if(k == 0)
		{
			yield(buf);
			return;
		}

		for(size_t i = offset; i <= n - k; ++i)
		{
			buf.emplace_back(i);
			_combs_helper(n, k-1, yield, i+1, buf);
			buf.pop_back();
		}
	}

	template<typename F>
	static void combs(size_t const n, size_t const k, F const& yield)
	{
		std::vector<size_t> buf;
		buf.reserve(k);
		_combs_helper(n, k, yield, 0, buf);
	}

public:
	static inline void exec(dataset_t const& d, size_t const n, size_t const k = 1)
	{
		assert(n >= k);

		std::vector<size_t> partition_subdivision(partition::generate_bare(d.objects.size(), n));
		std::vector<size_t> partitions(n);
		std::iota(partitions.begin(), partitions.end(), 0);

		size_t i = 0;
		combs(n, (n-k), [&](std::vector<size_t> const& train_ps) {
			std::vector<size_t> test_ps;
			std::set_difference(
				partitions.begin(), partitions.end(),
				train_ps.begin(), train_ps.end(),
				std::inserter(test_ps, test_ps.begin())
			);

			sliced_sparse_matrix_t<decltype(d.feature_matrix) const> train_m(d.feature_matrix), test_m(d.feature_matrix);
			for(size_t j = 0; j < d.objects.size(); ++j)
			{
				size_t p = partition_subdivision[j];
				if(std::binary_search(test_ps.begin(), test_ps.end(), p))
					test_m.add_key(j);
				else
					train_m.add_key(j);
			}

			std::cout << "Size: " << train_m.size() << "+" << test_m.size() << std::endl;

			knn<decltype(train_m)> c(5, train_m);

			size_t j = 0;
			for(auto const& test_row : test_m)
			{
				performance::result_t r(performance::measure(d, c, test_row));

				std::cout << i << " - " << j << " [" << test_row.row_i << "]: " << r.oocover << std::endl;
				j++;
			}
			i++;
		});
	}
};

}
