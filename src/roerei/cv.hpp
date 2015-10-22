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

			knn<decltype(train_m)> c(5, train_m);

			size_t const test_m_size = test_m.size();
			float avgoocover = 0.0f, avgooprecision = 0.0f;
			size_t j = 0;
			for(auto const& test_row : test_m)
			{
				performance::result_t r(performance::measure(d, c, test_row));

				avgoocover += r.oocover;
				avgooprecision += r.ooprecision;


				if(j % (test_m_size / 100) == 0)
				{
					size_t percentage = j / (test_m_size / 100);
					std::cout << '\r' << i << ": " << percentage << '%';
					std::cout.flush();
				}

				j++;
			}
			std::cout << '\r';
			std::cout.flush();

			avgoocover /= (float)test_m_size;
			avgooprecision /= (float)test_m_size;

			std::cout << i << ": " << avgoocover << " + " << avgooprecision << std::endl;

			i++;
		});
	}
};

}
