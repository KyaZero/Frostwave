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
			Vertex		= 1 << 0,
			Pixel		= 1 << 1,
			Geometry	= 1 << 2,
			None		= 1 << 3
		};

		Shader();
		Shader(u32 type, const std::string& pixel, const std::string& vertex = "", const std::string& geometry = "");
		~Shader();

		void Load(u32 type, const std::string& pixel, const std::string& vertex = "", const std::string& geometry = "");
		void Bind(u32 mask = 0) const;
		//void AddDefine(const std::string& name, const std::string& value);

		bool IsInitialized();
	private:
		struct Data;
		Data* m_Data;
	};
}
namespace fw = frostwave;