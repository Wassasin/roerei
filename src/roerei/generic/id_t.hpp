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

namespace detail
{

template<typename T, typename S>
struct serialize_value;

template<typename S>
struct serialize_value<object_id_t, S>
{
	static inline void exec(S& s, std::string const& name, object_id_t const& id)
	{
		s.write(name, id.unseal());
	}
};

template<typename T, typename D>
struct deserialize_value;

template<typename D>
struct deserialize_value<object_id_t, D>
{
	static inline object_id_t exec(D& s, const std::string& name)
	{
		size_t i;
		s.read(name, i);
		return object_id_t(i);
	}
};

}

}
