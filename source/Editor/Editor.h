#pragma once
#include <Engine\Core\Types.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Engine.h>

namespace frostwave
{
	class Editor
	{
	public:
		Editor();
		~Editor();

		void Update(Engine* engine, f32 dt, const Texture* renderedScene);

	private:
	};
}