#pragma once
#include <Engine/Logging/Logger.h>
#include <d3d11.h>

namespace frostwave
{
	inline bool ErrorCheck(HRESULT res)
	{
		if (FAILED(res))
		{
			FATAL_LOG("Failure");
			return false;
		}
		return true;
	}

	template<typename T>
	inline void SafeRelease(T** resource)
	{
		(*resource)->Release();
		*resource = nullptr;
	}
}
namespace fw = frostwave;