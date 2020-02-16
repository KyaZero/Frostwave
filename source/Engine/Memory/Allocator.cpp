#include "Allocator.h"
#include <malloc.h>
#include <cstring>
#include <cassert>

frostwave::Allocator* frostwave::Allocator::s_Instance = nullptr;

frostwave::Allocator::Allocator(const Size size) : m_Memory(nullptr), m_Size(size), m_UsedSize(0_B)
{
	m_Memory = malloc(size);
}

frostwave::Allocator::~Allocator()
{
	if (m_Blocks.size() > 0)
	{
		assert(false && "Trying to kill allocator with active subregions!");
		i64 bytes = 0;
		for (i32 i = 0; i < m_Blocks.size(); ++i)
		{
			bytes += m_Blocks[i].size;
		#ifdef _DEBUG
			INFO_LOG("Active region of type '%s'", m_Blocks[i].type.c_str());
		#endif
		}
		
		INFO_LOG("Trying to kill allocator with %llu active subregions (%lluB/%lluB, %f%% full)", m_Blocks.size(), bytes, m_Size.AsBytes(), ((f32)bytes / (f32)m_Size.AsBytes())*(f64)100.0f);
		return;
	}
	free(m_Memory);
	m_Size = 0_B;
}

void frostwave::Allocator::Create(Size maxSize)
{
	if (s_Instance)
	{
		FATAL_LOG("Already called Create on Allocator.");
	}

	s_Instance = new Allocator(maxSize);
}

void frostwave::Allocator::Destroy()
{
	delete s_Instance;
	s_Instance = nullptr;
}

frostwave::Allocator* frostwave::Allocator::Get()
{
	return s_Instance;
}

#ifdef _DEBUG
frostwave::Allocator::MemoryStats frostwave::Allocator::GetStats()
{
	return MemoryStats{m_Size, m_UsedSize};
}

frostwave::AllocResult frostwave::Allocator::Allocate(const Size size, std::string type)
#else
frostwave::AllocResult frostwave::Allocator::Allocate(const Size size)
#endif
{
	u64 current = 0;
	u64 next = m_Size;

	i32 index = 0;
	for (; index < m_Blocks.size(); ++index)
	{
		next = m_Blocks[index].offset;

		if (next - current >= size) break;

		current = next + m_Blocks[index].size;
	}

	if (current + size > m_Size)
	{
		FATAL_LOG("Failed to get memory cause ran out!");
		return AllocResult{ nullptr, 0_B };
	}

	void* memory = (i8*)m_Memory + current;
#ifdef _DEBUG
	m_Blocks.insert(m_Blocks.begin() + index, { current, size, type, memory });
#else
	m_Blocks.insert(m_Blocks.begin() + index, { current, size });
#endif

	m_UsedSize = Size(m_UsedSize.AsBytes() + size.AsBytes());

	return AllocResult{ memory, size };
}

void frostwave::Allocator::Free(void* memory)
{
	for (i32 i = 0; i < m_Blocks.size(); ++i)
	{
		if ((u64)((i8*)memory - (i8*)m_Memory) == m_Blocks[i].offset)
		{
			m_UsedSize = Size(m_UsedSize.AsBytes() - m_Blocks[i].size.AsBytes());
			m_Blocks.erase(m_Blocks.begin() + i);
			return;
		}
	}

	FATAL_LOG("Tried to free memory not owned by this allocator!");
}

bool frostwave::Allocator::IsAllocated(void* memory)
{
	for (i32 i = 0; i < m_Blocks.size(); ++i)
	{
		if ((u64)((i8*)memory - (i8*)m_Memory) == m_Blocks[i].offset)
		{
			return true;
		}
	}
	return false;
}
