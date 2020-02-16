#pragma once
#include <Engine/Core/Types.h>
#include <Engine/Logging/Logger.h>
#include <Engine/Memory/Size.h>
#include <vector>
#include <string>
#include <iostream>
#include <type_traits>
#include <typeinfo>

template <class, class = void>
struct is_defined : std::false_type { };

template <class T>
struct is_defined<T, std::enable_if_t<std::is_object<T>::value && !std::is_pointer<T>::value && (sizeof(T) > 0)>> : std::true_type{ };

namespace frostwave
{
	struct AllocResult
	{
		void* mem;
		Size size;
	};

	class Allocator
	{
	public:
		Allocator(const Size size);
		~Allocator();

		static void Create(Size maxSize);
		static void Destroy();

		static Allocator* Get();

	#ifdef _DEBUG 
		struct MemoryStats
		{
			Size max;
			Size current;
		};
		MemoryStats GetStats();

		AllocResult Allocate(const Size size, std::string type);
	#else
		AllocResult Allocate(const Size size);
	#endif

		void Free(void* memory);
		bool IsAllocated(void* memory);

		class NewResult
		{
		public:
			NewResult(u64 amount) : m_Amount(amount) { }

			template<typename T>
			operator T () const
			{
				if constexpr (!std::is_pointer_v<T>)
				{
					static_assert(false, "Allocate needs to be called with a pointer type as the return value!");
				}

#ifdef _DEBUG
				AllocResult res = Allocator::Get()->Allocate(Size::Bytes(sizeof(std::remove_pointer_t<T>) * m_Amount), typeid(std::remove_pointer_t<T>).name());
#else
				AllocResult res = Allocator::Get()->Allocate(Size::Bytes(sizeof(std::remove_pointer_t<T>) * m_Amount));
#endif

#ifdef _DEBUG
				if (Logger::Valid())
				{
					f64 used = (f64)(Allocator::Get()->m_UsedSize / 1024.0 / 1024.0);
					f64 max = (f64)(Allocator::Get()->m_Size / 1024.0 / 1024.0);
					VERBOSE_LOG("Allocated %llux of type %s - size: %llu bytes - %f%% used", m_Amount, typeid(std::remove_pointer_t<T>).name(), res.size.AsBytes(), used/max*100.0);
				}
#endif

				if constexpr (std::is_trivially_constructible_v<T>)
				{
					T t = new(res.mem) std::remove_pointer_t<T>;
					return t;
				}
				else
				{
					return (T)res.mem;
				}

			}
		private:
			u64 m_Amount;
		};

	private:
		static Allocator* s_Instance;

		void* m_Memory;
		Size m_Size, m_UsedSize;

		struct Block
		{
			u64 offset;
			Size size;
		#ifdef _DEBUG
			std::string type;
			void* ptr;
		#endif
		};
		std::vector<Block> m_Blocks;
	};

	template <typename T, typename... Ts>
	inline static T* Allocate(Ts&&... args) {
#ifdef _DEBUG 
		AllocResult result = Allocator::Get()->Allocate(Size(sizeof(T)), typeid(T).name());
#else
		AllocResult result = Allocator::Get()->Allocate(Size(sizeof(T)));
#endif
		T* ret = new (result.mem) T(std::forward<Ts>(args)...);
		return ret;
	}

	//The NewResult type automatically deduces into the correct type
	inline static Allocator::NewResult Allocate(u64 amount = 1)
	{
		return Allocator::NewResult(amount);
	}

	template<typename T>
	inline static bool IsAllocated(T* memory)
	{
		return Allocator::Get()->IsAllocated((void*)memory);
	}

	template<typename T>
	inline static void Free(T* memory)
	{
#ifdef _DEBUG
		VERBOSE_LOG("Freed memory of type %s!", typeid(T).name());
#endif
		if constexpr (std::is_trivially_destructible_v<T> || std::is_destructible_v<T>)
		{
			memory->~T();
		}
		return Allocator::Get()->Free((void*)memory);
	}
}
namespace fw = frostwave;