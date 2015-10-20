#pragma once

#include <boost/optional.hpp>

#include <random>
#include <algorithm>
#include <vector>
#include <list>

namespace roerei
{

class partition
{
private:
	partition() = delete;

public:
	//static std::vector<std::vector<size_t>> generate(size_t n, boost::optional<uint64_t> seed = boost::none)
	static void generate(size_t n, size_t parts, boost::optional<uint64_t> seed = boost::none)
	{
		std::vector<size_t> result(n);

		auto it = result.begin();
		size_t remaining = n;
		for(size_t parts_left = parts; parts_left > 0; parts_left--)
		{
			size_t size = remaining / parts_left;
			if(remaining % parts_left > 0)
				size++;

			remaining -= size;
			it = std::fill_n(it, size, parts - parts_left);
		}

		assert(it == result.end());

		if(!seed)
		{
			std::random_device rd;
			seed = rd();
		}

		std::mt19937 g(*seed);
		std::shuffle(result.begin(), result.end(), g);

		for(size_t i = 0; i < n; ++i)
		{
			std::cout << i << ": " << result[i] << std::endl;
		}
	}
};

}
