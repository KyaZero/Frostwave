#include <iostream>
#include <Engine/Engine.h>
#include <Engine/Graphics/Texture.h>
#include <Editor/Editor.h>
#include <Engine/Graphics/Framework.h>
#include <Game/Game.h>

int WinMain()
{
	{
		fw::Engine engine(50MB);
		fw::Editor editor;
		Game game;
		auto editorTick = [&](f32 dt, const fw::Texture* renderedScene) { editor.Update(&engine, dt, renderedScene); };
		auto gameTick = [&](f32 dt) { game.Update(&engine, dt); };
		auto gameInit = [&] {game.Init(&engine); };
		engine.Init(gameTick, gameInit, editorTick);
	}


	fw::Framework::ReportLiveObjects();
	//system("pause");
	return 0;
}