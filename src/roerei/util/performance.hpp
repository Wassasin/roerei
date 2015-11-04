#pragma once

#include <roerei/util/timer.hpp>
#include <roerei/util/guard.hpp>

#include <boost/optional.hpp>

#include <map>
#include <vector>
#include <stack>
#include <memory>
#include <chrono>
#include <string>
#include <functional>
#include <iostream>

namespace roerei
{

namespace test
{

class performance
{
public:
	typedef std::string key_t;
	typedef std::chrono::microseconds time_t;

private:
	struct node_t
	{
		std::vector<time_t> durations;
		std::map<key_t, std::shared_ptr<node_t>> children;
	};

	std::shared_ptr<node_t> root;
	std::stack<std::shared_ptr<node_t>> stack;

	static thread_local boost::optional<performance> singleton;

public:
	performance()
		: root(std::make_shared<node_t>())
		, stack()
	{
		stack.emplace(root);
	}

	static performance& init()
	{
		if(!singleton)
			singleton.reset(performance());

		return *singleton;
	}

	static void clear()
	{
		singleton.reset();
	}

	void operator()(key_t const& key, timer const& t)
	{
		auto node = stack.top()->children.emplace(std::make_pair(key, std::make_shared<node_t>())).first->second;
		node->durations.emplace_back(t.diff_msec());
	}

	void enter(key_t const& key)
	{
		auto next_node = stack.top()->children.emplace(std::make_pair(key, std::make_shared<node_t>())).first->second;
		stack.emplace(next_node);
	}

	void leave(key_t const& key, timer const& t)
	{
		stack.pop();
		(*this)(key, t);
	}

	template<typename F>
	auto operator()(key_t const& key, F f) -> typename std::result_of<F()>::type
	{
		timer t;
		enter(std::move(key));

		auto g(make_guard([&]() {
			leave(key, t);
		}));

		return f();
	}

	static void note_time(key_t const& key, timer const& t)
	{
		if(!singleton)
			throw std::runtime_error("Singleton performance measurements are not initialized");

		(*singleton)(key, t);
	}

	template<typename F>
	static auto measure(key_t const& key, F f) -> typename std::result_of<F()>::type
	{
		if(!singleton)
			throw std::runtime_error("Singleton performance measurements are not initialized");

		return (*singleton)(key, f);
	}

	void print_indentation(std::ostream& os, size_t indent)
	{
		os << std::string(indent, ' ');
	}

	time_t report(node_t const& node, key_t const& key, size_t indent = 0)
	{
		time_t total = time_t::zero();
		for(time_t const& d : node.durations)
			total += d;

		print_indentation(std::cerr, indent);
		if(indent == 0) // Special root case
			std::cerr << key << std::endl;
		else
			std::cerr << key << ": " << total.count() << "µs" << std::endl;

		time_t subtotal = time_t::zero();
		for(auto const& child_tup : node.children)
			subtotal += report(*child_tup.second, child_tup.first, indent+1);

		time_t missing = total - subtotal;
		if(node.children.size() > 0 && missing.count() > 0)
		{
			print_indentation(std::cerr, indent+1);
			std::cerr << "missing: " << missing.count() << "µs" << std::endl;
		}

		return total;
	}

	void report()
	{
		report(*root, "root");
	}
};

#define performance_scope(name) \
	timer _performance_t; \
	std::string _performance_name(name); \
	test::performance::init().enter(_performance_name); \
	auto _performance_guard(make_guard([&]() { test::performance::init().leave(_performance_name, _performance_t); }));

#define performance_func performance_scope(__func__)

#define register_performance decltype(roerei::test::performance::singleton) thread_local roerei::test::performance::singleton;

}

}
