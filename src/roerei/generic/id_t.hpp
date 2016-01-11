#pragma once

namespace roerei
{

class id_t
{
	size_t id;

public:
	id_t(size_t _id)
		: id(_id)
	{}

	size_t unseal() const
	{
		return id;
	}

	bool operator==(id_t const rhs) const
	{
		return id == rhs.id;
	}

	bool operator<(id_t const rhs) const
	{
		return id < rhs.id;
	}

	bool operator>=(id_t const rhs) const
	{
		return !operator<(rhs);
	}
};

struct object_id_t : public id_t
{
	object_id_t(size_t _id) : id_t(_id) {}
};

struct feature_id_t : public id_t
{
	feature_id_t(size_t _id) : id_t(_id) {}
};

struct dependency_id_t : public id_t
{
	dependency_id_t(size_t _id) : id_t(_id) {}
};

}
