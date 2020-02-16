#pragma once
#include <filesystem>
#include <vector>
#include <algorithm>
#include <fstream>

namespace fs = std::filesystem;

namespace frostwave
{
	class FileWatcher
	{
	public:
		static FileWatcher* Get() { static FileWatcher* instance = new FileWatcher; return instance; }
		void Register(void* ptr, const std::string& path, std::function<void(const std::string&)> callback)
		{
			if (!fs::exists(path)) return;

			for (auto&& watch : m_Watchers)
			{
				if (watch.path == path)
				{
					for (auto& call : watch.callbacks)
					{
						if (call.address == ptr) return;
					}
					Callback cb;
					cb.func = callback;
					cb.address = ptr;
					watch.callbacks.emplace_back(Callback{ ptr, callback });
					return;
				}
			}

			FileInfo info = { };
			info.path = path;
			info.time = fs::last_write_time(path).time_since_epoch().count();
			info.callbacks.emplace_back(Callback{ ptr, callback });

			m_Watchers.push_back(info);
		}
		void Unregister(void* ptr, const std::string& path)
		{
			for (i32 i = 0; i < (i32)m_Watchers.size(); i++)
			{
				auto& watch = m_Watchers[i];
				if (watch.path == path)
				{
					for (i32 j = 0; j < (i32)watch.callbacks.size(); j++)
					{
						auto callback = watch.callbacks[j];
						if (callback.address == ptr || ptr == nullptr)
						{
							watch.callbacks[j] = watch.callbacks.back();
							watch.callbacks.pop_back();
						}
					}

					if (watch.callbacks.size() > 0) continue;
					m_Watchers[i] = m_Watchers.back();
					m_Watchers.pop_back();
					return;
				}
			}
		}
		void Update([[maybe_unused]] const f32 aDeltaTime)
		{
			std::vector<std::pair<std::string, std::function<void(const std::string&)>>> toUpdate;
			for(auto& watch : m_Watchers)
			{
				if (watch.path.length() == 0) continue;

				if (fs::exists(watch.path))
				{
					auto newTime = fs::last_write_time(watch.path).time_since_epoch().count();
					if (watch.time < newTime)
					{
						for (auto&& callback : watch.callbacks)
							callback.func(watch.path);

						watch.time = newTime;
					}
				}
				else
				{
					WARNING_LOG("File '%s' doesnt exist, removing watch..", watch.path.c_str());
					Unregister(nullptr, watch.path);
				}
			}
		}
	private:
		FileWatcher() : m_LastIndex(0) { m_Watchers.reserve(80); }
		~FileWatcher() { }

		struct Callback
		{
			void* address = nullptr;
			std::function<void(const std::string&)> func;
		};

		struct FileInfo
		{
			std::string path;
			i64 time;
			std::vector<Callback> callbacks;
		};
		std::vector<FileInfo> m_Watchers;
		u64 m_LastIndex;
	};
}
namespace fw = frostwave;