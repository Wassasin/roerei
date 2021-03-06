#pragma once

#include <vector>
#include <future>
#include <sstream>

namespace roerei
{
	class multitask
	{
		class jobset_t
		{
			static size_t jobset_count;

			std::vector<std::packaged_task<void()>> tasks;
			std::packaged_task<void()> continuation;

			std::unique_ptr<std::mutex> mutex;
			size_t jobset_ident;
			size_t tasks_i;
			size_t tasks_done;

			bool all_started() /* Thread unsafe */
			{
				return tasks_i >= tasks.size();
			}

		public:
			jobset_t() = default;
			jobset_t(jobset_t&&) = default;

			jobset_t(std::vector<std::packaged_task<void()>>&& _tasks, std::packaged_task<void()>&& _continuation)
				: tasks(std::move(_tasks))
				, continuation(std::move(_continuation))
				, mutex(new std::mutex())
				, jobset_ident(jobset_count++)
				, tasks_i(0)
				, tasks_done(0)
			{}

			bool run() // Returns false if done; i.e. returns true if still able to run something
			{
				size_t i;
				{
					std::lock_guard<std::mutex> _l(*mutex);
					if(all_started())
						return false;

					i = tasks_i++;
				}

				auto& t = tasks[i];
				auto future = t.get_future();

				{
					std::stringstream ss;
					ss << "Started #" << jobset_ident << "-" << i << " (out of " << tasks.size() << ")";
					std::cerr << ss.str() << std::endl;
				}

				t(); // Execute packaged task

				{
					std::stringstream ss;
					ss << "Finished #" << jobset_ident << "-" << i << " (out of " << tasks.size() << ")";
					std::cerr << ss.str() << std::endl;
				}

				future.get(); // Yield exception

				{
					std::lock_guard<std::mutex> _l(*mutex);
					if(++tasks_done < tasks.size())
						return !all_started();
				}

				{
					std::stringstream ss;
					ss << "Starting continuation #" << jobset_ident;
					std::cerr << ss.str() << std::endl;
				}

				continuation(); // Execute continuation

				{
					std::stringstream ss;
					ss << "Finished continuation #" << jobset_ident;
					std::cerr << ss.str() << std::endl;
				}

				return false;
			}
		};

		std::vector<jobset_t> jobsets;

		void handlet()
		{
			for(auto& jobset : jobsets)
			{
				while(jobset.run())
				{}
			}
		}

	public:
		multitask()
			: jobsets()
		{}

		void add(jobset_t&& _jobset)
		{
			jobsets.emplace_back(std::move(_jobset));
		}

		void run(size_t n = 1, bool blocking = true)
		{
			std::cerr << "Starting " << jobsets.size() << " jobsets" << std::endl;

			std::vector<std::thread> threads;
			threads.reserve(n);
			for(size_t i = 0; i < n; ++i)
				threads.emplace_back([&]() { handlet(); });

			if(blocking)
				for(auto& t : threads)
					t.join();
			else
				for(auto& t : threads)
					t.detach();
		}

		void run_synced()
		{
			handlet();
		}
	};
}
