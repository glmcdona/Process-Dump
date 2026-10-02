#pragma once

#include <algorithm>
#include <condition_variable>
#include <deque>
#include <exception>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

// Producers may wait for module work, but help execute only leaf jobs while waiting.
// This avoids both pool starvation and recursively nesting process lifetimes.
class work_pool
{
	struct group
	{
		size_t pending = 0;
		std::exception_ptr error;
	};
	struct job
	{
		std::function<void()> run;
		group* completion;
	};
	std::mutex mutex_;
	std::condition_variable changed_;
	std::deque<job> producers_, leaves_;
	std::vector<std::thread> workers_;
	group root_;
	bool stopping_ = false;

	void execute(job task)
	{
		std::exception_ptr error;
		try { task.run(); }
		catch (...) { error = std::current_exception(); }
		{
			std::lock_guard<std::mutex> lock(mutex_);
			if (error && !task.completion->error)
				task.completion->error = error;
			--task.completion->pending;
		}
		changed_.notify_all();
	}

	void worker()
	{
		for (;;)
		{
			job task;
			{
				std::unique_lock<std::mutex> lock(mutex_);
				changed_.wait(lock, [&] { return stopping_ || !leaves_.empty() || !producers_.empty(); });
				if (stopping_ && leaves_.empty() && producers_.empty())
					return;
				auto& queue = leaves_.empty() ? producers_ : leaves_;
				task = std::move(queue.front());
				queue.pop_front();
			}
			execute(std::move(task));
		}
	}

	void wait(group& completion, bool help)
	{
		std::unique_lock<std::mutex> lock(mutex_);
		while (completion.pending != 0)
		{
			if (help && !leaves_.empty())
			{
				auto task = std::move(leaves_.front());
				leaves_.pop_front();
				lock.unlock();
				execute(std::move(task));
				lock.lock();
			}
			else
				changed_.wait(lock);
		}
		if (completion.error)
			std::rethrow_exception(completion.error);
	}

public:
	explicit work_pool(int threads)
	{
		if (threads < 1)
			throw std::invalid_argument("worker count must be positive");
		try
		{
			for (int i = 0; i < threads; ++i)
				workers_.emplace_back([this] { worker(); });
		}
		catch (...)
		{
			{
				std::lock_guard<std::mutex> lock(mutex_);
				stopping_ = true;
			}
			changed_.notify_all();
			for (auto& thread : workers_) thread.join();
			throw;
		}
	}

	~work_pool()
	{
		{
			std::lock_guard<std::mutex> lock(mutex_);
			stopping_ = true;
		}
		changed_.notify_all();
		for (auto& thread : workers_) thread.join();
	}

	void submit(std::function<void()> task)
	{
		{
			std::lock_guard<std::mutex> lock(mutex_);
			producers_.push_back({std::move(task), &root_});
			++root_.pending;
		}
		changed_.notify_one();
	}

	void wait() { wait(root_, false); }

	template<typename T, typename F>
	void parallel_for(const std::vector<T>& items, F action)
	{
		// Bound queued metadata and captured references even for huge process maps.
		for (size_t first = 0; first < items.size(); first += 256)
		{
			group completion;
			try
			{
				std::lock_guard<std::mutex> lock(mutex_);
				for (size_t i = first; i < (std::min)(items.size(), first + 256); ++i)
				{
					leaves_.push_back({[&, i] { action(items[i]); }, &completion});
					++completion.pending;
				}
			}
			catch (...)
			{
				const auto error = std::current_exception();
				changed_.notify_all();
				wait(completion, true);
				std::rethrow_exception(error);
			}
			changed_.notify_all();
			wait(completion, true);
		}
	}
};
