#pragma once

#include <roerei/uri.hpp>

#include <boost/numeric/ublas/matrix_sparse.hpp>

#include <vector>

namespace roerei
{

struct dataset
{
	typedef boost::numeric::ublas::compressed_matrix<uint16_t> matrix_t;

	std::vector<uri_t> const objects, dependencies;
	matrix_t matrix;

	dataset(dataset&&) = default;
	dataset(dataset const&) = delete;

	template<typename CONTAINER>
	dataset(CONTAINER&& _objects, CONTAINER&& _dependencies);
};

namespace detail
{
	template<typename B>
	static inline std::vector<uri_t> construct_move_elements(B&& ys)
	{
		std::vector<uri_t> xs;
		for(auto&& y : ys)
			xs.emplace_back(std::move(y));

		return xs;
	}
}

template<typename CONTAINER>
dataset::dataset(CONTAINER&& _objects, CONTAINER&& _dependencies)
	: objects(detail::construct_move_elements(_objects))
	, dependencies(detail::construct_move_elements(_dependencies))
	, matrix(objects.size(), dependencies.size())
{

}

}
