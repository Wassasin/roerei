#pragma once

#include <roerei/uri.hpp>
#include <roerei/sparse_matrix.hpp>

#include <vector>

namespace roerei
{

struct dataset_t
{
	typedef uint16_t value_t;
	typedef sparse_matrix_t<value_t> matrix_t;

	std::vector<uri_t> const objects, dependencies;
	matrix_t matrix;

	dataset_t(dataset_t&&) = default;
	dataset_t(dataset_t&) = delete;
	dataset_t(std::vector<uri_t>&& _objects, std::vector<uri_t>&& _dependencies, matrix_t&& _matrix);

	template<typename CONTAINER>
	dataset_t(CONTAINER&& _objects, CONTAINER&& _dependencies);
};

namespace detail
{
	template<typename B>
	static inline std::vector<uri_t> construct_move_elements(B&& ys)
	{
		std::vector<uri_t> xs;
		xs.reserve(ys.size());
		for(auto&& y : ys)
			xs.emplace_back(std::move(y));

		return xs;
	}
}

template<typename CONTAINER>
dataset_t::dataset_t(CONTAINER&& _objects, CONTAINER&& _dependencies)
	: objects(detail::construct_move_elements(_objects))
	, dependencies(detail::construct_move_elements(_dependencies))
	, matrix(objects.size(), dependencies.size())
{}

dataset_t::dataset_t(std::vector<uri_t>&& _objects, std::vector<uri_t>&& _dependencies, matrix_t&& _matrix)
	: objects(std::move(_objects))
	, dependencies(std::move(_dependencies))
	, matrix(std::move(_matrix))
{}

}

BOOST_FUSION_ADAPT_STRUCT(
		roerei::dataset_t,
		(std::vector<roerei::uri_t>, objects)
		(std::vector<roerei::uri_t>, dependencies)
		(roerei::dataset_t::matrix_t, matrix)
)
