#pragma once

#include <roerei/partition.hpp>
#include <roerei/dataset.hpp>

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

		std::vector<size_t> partition_subdivision = partition::generate_bare(d.objects.size(), n);
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

			//STUB

			std::cout << "#" << i;
			for(auto x : train_ps)
				std::cout << ' ' << x;

			std::cout << " |";
			for(auto x : test_ps)
				std::cout << ' ' << x;

			i++;
			std::cout << std::endl;
		});
	}
};

}
