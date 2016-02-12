#pragma once

#include <roerei/serialization/deserialize_common.hpp>

#include <msgpack.hpp>
#include <stack>

namespace roerei
{

class msgpack_deserializer
{
private:
	enum type_t
	{
		array,
		map,
		non_container
	};

public:
	msgpack_deserializer();
	~msgpack_deserializer();

	size_t read_array(const std::string& name);
	size_t read_object(const std::string& name);
	void read_null(const std::string& name);

	void read(const std::string& key, uint64_t& x);
	void read(const std::string& key, std::string& x);
	void read(const std::string& key, float& x);

	void feed(const std::string& str);

private:
	const msgpack::object& read(const msgpack::type::object_type t_expected);
	void read_key(const std::string& key);

private:
	struct stack_e
	{
		const msgpack::object* obj_ptr;
		type_t t;
		size_t n, i;

		stack_e(const msgpack::object* obj_ptr, type_t t, size_t n, size_t i);
	};

	msgpack::unpacker pac;
	msgpack::unpacked pac_result;
	std::stack<stack_e> stack;
};

inline msgpack_deserializer::msgpack_deserializer()
	: pac()
	, pac_result()
	, stack()
{}

inline msgpack_deserializer::~msgpack_deserializer()
{}

inline msgpack_deserializer::stack_e::stack_e(const msgpack::object* _obj_ptr, type_t _t, size_t _n, size_t _i)
	: obj_ptr(_obj_ptr)
	, t(_t)
	, n(_n)
	, i(_i)
{}

inline void msgpack_deserializer::feed(const std::string& str)
{
	pac.reserve_buffer(str.size());
	memcpy(pac.buffer(), str.data(), str.size());
	pac.buffer_consumed(str.size());
}

inline std::string convert_msgpack_type(const msgpack::type::object_type t)
{
	switch(t)
	{
	case msgpack::type::ARRAY:
		return "ARRAY";
	case msgpack::type::BOOLEAN:
		return "BOOLEAN";
	case msgpack::type::FLOAT:
		return "FLOAT";
	case msgpack::type::MAP:
		return "MAP";
	case msgpack::type::NEGATIVE_INTEGER:
		return "NEGATIVE_INTEGER";
	case msgpack::type::NIL:
		return "NIL";
	case msgpack::type::POSITIVE_INTEGER:
		return "POSITIVE_INTEGER";
	case msgpack::type::STR:
		return "STR";
	case msgpack::type::BIN:
		return "BIN";
	case msgpack::type::EXT:
		return "EXT";
	}
}

inline const msgpack::object& msgpack_deserializer::read(const msgpack::type::object_type t_expected)
{
	if(stack.empty())
	{
		if(!pac.next(&pac_result))
			throw eob_error();

		stack.emplace(&pac_result.get(), type_t::non_container, 1, 0);
	}

	stack_e& e = stack.top();
	const msgpack::object* result_ptr;

	switch(e.t)
	{
	case type_t::array:
		result_ptr = &e.obj_ptr->via.array.ptr[e.i];
		break;
	case type_t::map:
	{
		msgpack::object_kv& kv = e.obj_ptr->via.map.ptr[e.i/2];
		result_ptr = (e.i % 2 == 0) ? &kv.key : &kv.val;
	}
		break;
	case type_t::non_container:
		result_ptr = e.obj_ptr;
		break;
	}

	if(t_expected != result_ptr->type)
		throw type_error(convert_msgpack_type(t_expected), convert_msgpack_type(result_ptr->type));

	e.i++;

	if(e.i >= e.n)
		stack.pop();

	return *result_ptr;
}

inline void msgpack_deserializer::read_key(const std::string& key)
{
	if(stack.empty() || stack.top().t != type_t::map)
		return; //Do not check if we're not in a map

	const msgpack::object& obj = read(msgpack::type::STR);

	const std::string received(obj.via.str.ptr, obj.via.str.size);
	if(key != received)
		throw key_error(key, received);
}

template<typename R = void, typename S, typename F>
inline R autorollbackonfailure(S& stack, F f)
{
	// Protection against changes in top node; and removal (pop) of top node
	if(stack.empty())
		return f(); // Nothing to rollback in any case

	auto const e = stack.top();
	size_t depth = stack.size();

	auto rollback_f([&]()
	{
		if(depth == stack.size())
			stack.top().i = e.i;
		else if(depth == stack.size() - 1)
			stack.emplace(e);
		else
			throw std::logic_error("Rollback failure");
	});

	try
	{
		return f();
	} catch(type_error e)
	{
		rollback_f();
		throw e;
	} catch(key_error e)
	{
		rollback_f();
		throw e;
	} catch(std::runtime_error e)
	{
		rollback_f();
		throw e;
	}
}

inline size_t msgpack_deserializer::read_array(const std::string& name)
{
	return autorollbackonfailure<size_t>(stack, [&]() {
		read_key(name);

		const msgpack::object& obj = read(msgpack::type::ARRAY);

		if(obj.via.array.size > 0)
			stack.emplace(&obj, type_t::array, obj.via.array.size, 0);

		return obj.via.array.size;
	});
}

inline size_t msgpack_deserializer::read_object(const std::string& name)
{
	read_key(name);

	const msgpack::object& obj = read(msgpack::type::MAP);
	if(obj.via.map.size > 0)
		stack.emplace(&obj, type_t::map, obj.via.map.size*2, 0);

	return obj.via.map.size;
}

inline void msgpack_deserializer::read_null(const std::string& name)
{
	autorollbackonfailure(stack, [&]() {
		read_key(name);
		read(msgpack::type::NIL);
	});
}

inline void msgpack_deserializer::read(const std::string& key, uint64_t& x)
{
	autorollbackonfailure(stack, [&]() {
		read_key(key);
		const msgpack::object& obj = read(msgpack::type::POSITIVE_INTEGER);
		x = obj.via.u64;
	});
}


inline void msgpack_deserializer::read(const std::string& key, std::string& x)
{
	autorollbackonfailure(stack, [&]() {
		read_key(key);

		const msgpack::object& obj = read(msgpack::type::STR);
		x = std::string(obj.via.str.ptr, obj.via.str.size);
	});
}

inline void msgpack_deserializer::read(const std::string& key, float& x)
{
	autorollbackonfailure(stack, [&]() {
		read_key(key);
		const msgpack::object& obj = read(msgpack::type::FLOAT);
		x = obj.via.f64;
	});
}

}
