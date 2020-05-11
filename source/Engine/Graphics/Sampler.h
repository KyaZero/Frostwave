#pragma once
#include <Engine/Core/Math/Vec4.h>

namespace frostwave
{
	class Sampler
	{
	public:
		enum class Filter
		{
			Linear,
			Point,
			Anisotropic
		};

		enum class Address
		{
			Clamp,
			Wrap,
			Mirror,
			Border
		};
		Sampler();
		Sampler(Filter filter, Address address, Vec4f border);
		~Sampler();

		void Init(Filter filter, Address address, Vec4f border);
		void Bind(u32 slot);
		void Unbind(u32 slot);

	private:
		struct Data;
		Data* m_Data;
	};
}
namespace fw = frostwave;