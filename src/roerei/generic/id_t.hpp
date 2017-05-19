#pragma once

#include <cstddef>

namespace roerei
{

template<typename T>
class id_t
{
	size_t id;

protected:
	id_t(size_t _id)
		: id(_id)
	{}

public:
	size_t unseal() const
	{
		return id;
	}

	bool operator==(T const rhs) const
	{
		return id == rhs.id;
	}

	bool operator<(T const rhs) const
	{
		return id < rhs.id;
	}

	bool operator>=(T const rhs) const
	{
		return !operator<(rhs);
	}

	template<typename F>
	static void iterate(F&& f, size_t count)
	{
		for (size_t i = 0; i < count; ++i) {
			f(T(i));
		}
	}
};

struct object_id_t : public id_t<object_id_t>
{
	object_id_t(size_t _id) : id_t(_id) {}
};

struct feature_id_t : public id_t<feature_id_t>
{
	feature_id_t(size_t _id) : id_t(_id) {}
};

struct dependency_id_t : public id_t<dependency_id_t>
{
	dependency_id_t(size_t _id) : id_t(_id) {}
};

}
