#pragma once

#include <roerei/uri.hpp>

#include <roerei/generic/sparse_matrix.hpp>
#include <roerei/generic/create_map.hpp>
#include <roerei/generic/encapsulated_vector.hpp>
#include <roerei/generic/id_t.hpp>

#include <boost/fusion/include/adapt_struct.hpp>

#include <vector>
#include <set>

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

	std::set<object_id_t> prior_objects;

public:
	dataset_t(dataset_t&&) = default;
	dataset_t(dataset_t const&) = delete;
	dataset_t(
			std::remove_const<decltype(objects)>::type&& _objects,
			std::remove_const<decltype(features)>::type&& _features,
			std::remove_const<decltype(dependencies)>::type&& _dependencies,
			feature_matrix_t&& _feature_matrix,
			dependency_matrix_t&& _dependency_matrix,
			std::remove_const<decltype(prior_objects)>::type&& _prior_objects
	);

	template<typename CONTAINER>
	dataset_t(CONTAINER&& _objects, CONTAINER&& _features, CONTAINER&& _dependencies);

	std::map<dependency_id_t, object_id_t> create_dependency_map() const;
	std::map<object_id_t, dependency_id_t> create_dependency_revmap() const;
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

inline std::map<dependency_id_t, object_id_t> dataset_t::create_dependency_map() const
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

inline std::map<object_id_t, dependency_id_t> dataset_t::create_dependency_revmap() const
{
	std::map<uri_t, dependency_id_t> dependency_urimap;
	dependencies.iterate([&](dependency_id_t id, uri_t const& u) {
		dependency_urimap.emplace(std::make_pair(u, id));
	});

	std::map<object_id_t, dependency_id_t> dependency_revmap;
	objects.iterate([&](object_id_t id, uri_t const& u) {
		auto it = dependency_urimap.find(u);
		if(it == dependency_urimap.end())
			return;

		dependency_revmap.emplace(std::make_pair(id, it->second));
	});
	return dependency_revmap;
}

template<typename CONTAINER>
dataset_t::dataset_t(CONTAINER&& _objects, CONTAINER&& _features, CONTAINER&& _dependencies)
	: objects(detail::construct_move_elements<object_id_t>(_objects))
	, features(detail::construct_move_elements<feature_id_t>(_features))
	, dependencies(detail::construct_move_elements<dependency_id_t>(_dependencies))
	, feature_matrix(objects.size(), features.size())
	, dependency_matrix(objects.size(), dependencies.size())
	, prior_objects()
{}

inline dataset_t::dataset_t(
		std::remove_const<decltype(objects)>::type&& _objects,
		std::remove_const<decltype(features)>::type&& _features,
		std::remove_const<decltype(dependencies)>::type&& _dependencies,
		feature_matrix_t&& _feature_matrix,
		dependency_matrix_t&& _dependency_matrix,
		std::remove_const<decltype(prior_objects)>::type&& _prior_objects
)
	: objects(std::move(_objects))
	, features(std::move(_features))
	, dependencies(std::move(_dependencies))
	, feature_matrix(std::move(_feature_matrix))
	, dependency_matrix(std::move(_dependency_matrix))
	, prior_objects(std::move(_prior_objects))
{}

}

BOOST_FUSION_ADAPT_STRUCT(
		roerei::dataset_t,
		(decltype(roerei::dataset_t::objects), objects)
		(decltype(roerei::dataset_t::features), features)
		(decltype(roerei::dataset_t::dependencies), dependencies)
		(roerei::dataset_t::feature_matrix_t, feature_matrix)
		(roerei::dataset_t::dependency_matrix_t, dependency_matrix)
		(decltype(roerei::dataset_t::prior_objects), prior_objects)
)
