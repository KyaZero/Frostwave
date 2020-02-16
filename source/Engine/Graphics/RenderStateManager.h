#pragma once
#pragma once

namespace frostwave
{
	class RenderStateManager
	{
	public:
		enum class BlendStates
		{
			Disable,
			AlphaBlend,
			Additive,
			Count
		};

		enum class DepthStencilStates
		{
			Default,
			LessEquals,
			ReadOnly,
			Count,
		};

		enum class RasterizerStates
		{
			Default,
			NoCull,
			FrontFace,
			Wireframe,
			BackCull,
			Count
		};

		RenderStateManager();
		~RenderStateManager();

		bool Init();

		void SetBlendState(BlendStates aBlendState);
		void SetDepthStencilState(DepthStencilStates aDepthState);
		void SetRasterizerState(RasterizerStates aRasterizerState);
		void SetAllDefault();

	private:
		bool CreateBlendStates();
		bool CreateDepthStencilStates();
		bool CreateRasterizerStates();

		struct Data;
		Data* m_Data;
	};
}