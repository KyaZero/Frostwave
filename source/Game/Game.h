#pragma once
#include <Engine/Core/Types.h>
#include <Engine/Graphics/Lights.h>
#include <Engine/Graphics/Model.h>
#include "FreeCamera.h"

namespace frostwave
{
	class Engine;
}

class Game
{
public:
	Game();
	~Game();
	void Init(frostwave::Engine* engine);
	void Update(frostwave::Engine* engine, f32 dt);

private:
	FreeCamera* m_Camera;
	fw::DirectionalLight* m_Light;
	fw::Model* m_Sponza;
};