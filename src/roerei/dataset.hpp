#pragma once

#include <roerei/uri.hpp>
#include <roerei/sparse_matrix.hpp>
#include <roerei/create_map.hpp>
#include <roerei/encapsulated_vector.hpp>
#include <roerei/id_t.hpp>

#include <boost/fusion/include/adapt_struct.hpp>

#include <vector>

namespace roerei
{

struct dataset_t
{
	typedef uint16_t value_t;
	typedef sparse_matrix_t<object_id_t, feature_id_t, value_t> feature_matrix_t;
	typedef sparse_matrix_t<object_id_t, dependency_id_t, value_t> dependency_matrix_t;

	encapsulated_vector<object_id_t, uri_t> objects;
	encapsulated_vector<feature_id_t, uri_t> features;
	encapsulated_vector<dependency_id_t, uri_t> dependencies;
	feature_matrix_t feature_matrix;
	dependency_matrix_t dependency_matrix;

public:
	dataset_t(dataset_t&&) = default;
	dataset_t(dataset_t&) = delete;
	dataset_t(
			std::remove_const<decltype(objects)>::type&& _objects,
			std::remove_const<decltype(features)>::type&& _features,
			std::remove_const<decltype(dependencies)>::type&& _dependencies,
			feature_matrix_t&& _feature_matrix,
			dependency_matrix_t&& _dependency_matrix
	);

	template<typename CONTAINER>
	dataset_t(CONTAINER&& _objects, CONTAINER&& _features, CONTAINER&& _dependencies);

	std::map<feature_id_t, object_id_t> create_feature_map() const;
	std::map<dependency_id_t, object_id_t> create_dependency_map() const;
};

namespace detail
{
	template<typename ID, typename B>
	static inline encapsulated_vector<ID, uri_t> construct_move_elements(B&& ys)
	{
		encapsulated_vector<ID, uri_t> xs;
		xs.reserve(ys.size());
		for(auto&& y : ys)
			xs.emplace_back(std::move(y));

		return xs;
	}
}

std::map<feature_id_t, object_id_t> dataset_t::create_feature_map() const
{
	std::map<uri_t, object_id_t> object_map;
	objects.iterate([&](object_id_t id, uri_t const& u) {
		object_map.emplace(std::make_pair(u, id));
	});

	std::map<feature_id_t, object_id_t> feature_map;
	features.iterate([&](feature_id_t id, uri_t const& u) { feature_map.emplace(std::make_pair(id, object_map.at(u))); });
	return feature_map;
}

std::map<dependency_id_t, object_id_t> dataset_t::create_dependency_map() const
{
	std::map<uri_t, object_id_t> object_map;
	objects.iterate([&](object_id_t id, uri_t const& u) {
		object_map.emplace(std::make_pair(u, id));
	});

	std::map<dependency_id_t, object_id_t> dependency_map;
	dependencies.iterate([&](dependency_id_t id, uri_t const& u) {
		auto it = object_map.find(u);
		if(it == object_map.end())
			return;

		dependency_map.emplace(std::make_pair(id, it->second));
	});
	return dependency_map;
}

template<typename CONTAINER>
dataset_t::dataset_t(CONTAINER&& _objects, CONTAINER&& _features, CONTAINER&& _dependencies)
	: objects(detail::construct_move_elements<object_id_t>(_objects))
	, features(detail::construct_move_elements<feature_id_t>(_features))
	, dependencies(detail::construct_move_elements<dependency_id_t>(_dependencies))
	, feature_matrix(objects.size(), features.size())
	, dependency_matrix(objects.size(), dependencies.size())
{}

dataset_t::dataset_t(
		std::remove_const<decltype(objects)>::type&& _objects,
		std::remove_const<decltype(features)>::type&& _features,
		std::remove_const<decltype(dependencies)>::type&& _dependencies,
		feature_matrix_t&& _feature_matrix,
		dependency_matrix_t&& _dependency_matrix
)
	: objects(std::move(_objects))
	, features(std::move(_features))
	, dependencies(std::move(_dependencies))
	, feature_matrix(std::move(_feature_matrix))
	, dependency_matrix(std::move(_dependency_matrix))
{}

}

BOOST_FUSION_ADAPT_STRUCT(
		roerei::dataset_t,
		(decltype(roerei::dataset_t::objects), objects)
		(decltype(roerei::dataset_t::features), features)
		(decltype(roerei::dataset_t::dependencies), dependencies)
		(roerei::dataset_t::feature_matrix_t, feature_matrix)
		(roerei::dataset_t::dependency_matrix_t, dependency_matrix)
)
