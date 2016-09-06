#pragma once

#include <roerei/dataset.hpp>
#include <roerei/distance.hpp>

#include <functional>
#include <vector>

namespace roerei
{

template<typename ROW>
class ensemble
{
private:
	typedef std::pair<object_id_t, float> distance_t;
	typedef std::vector<std::pair<dependency_id_t, float>> prediction_t;
	typedef std::function<prediction_t(ROW const&)> predict_f_t;

private:
	dataset_t const& d;
	std::vector<std::pair<predict_f_t, float>> predictors;

public:
	ensemble(dataset_t const& _d)
	: d(_d)
	, predictors()
	{}

	void add_predictor(predict_f_t&& predict_f, float weight)
	{
		predictors.emplace_back(std::make_pair(std::move(predict_f), weight));
	}

	prediction_t predict(ROW const& ys) const
	{
		float weight_sum = 0.0f;
		encapsulated_vector<dependency_id_t, float> value_sums(d.dependencies.size());

		for(auto predictor_kvp : predictors)
		{
			auto predict_f = predictor_kvp.first;
			float weight = predictor_kvp.second;

			prediction_t ps = predict_f(ys);
			for(auto prediction_kvp : ps)
				value_sums[prediction_kvp.first] += weight * prediction_kvp.second;

			weight_sum += weight;
		}

		std::vector<std::pair<dependency_id_t, float>> result;
		result.reserve(d.dependencies.size()); // TODO too much
		value_sums.iterate([&result, weight_sum](dependency_id_t id, float value) {
			if(value == 0.0f)
				return;

			result.emplace_back(std::make_pair(id, value / weight_sum));
		});
		return result;
	}
};

}
