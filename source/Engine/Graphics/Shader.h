#pragma once
#include <Engine/Core/Types.h>
#include <string>

namespace frostwave
{
	class Shader
	{
	public:
		enum Type
		{
			Vertex = 1 << 0,
			Pixel = 1 << 1,
			None = 1 << 2
		};

		Shader();
		Shader(u32 type, const std::string& pixel, const std::string& vertex = "");
		~Shader();

		void Load(u32 type, const std::string& pixel, const std::string& vertex = "");
		void Bind(u32 mask = 0) const;
		void AddDefine(const std::string& name, const std::string& value);

		bool IsInitialized();
	private:
		struct Data;
		Data* m_Data;
	};
}
namespace fw = frostwave;