#pragma once

#ifdef _DEBUG
namespace frostwave
{
	class DebugVisualizer
	{
	public:
		void Draw();
	};
}
namespace fw = frostwave;
#endif