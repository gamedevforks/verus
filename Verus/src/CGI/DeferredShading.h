#pragma once

namespace verus
{
	namespace CGI
	{
		enum class LightType : int
		{
			none,
			dir,
			omni,
			spot
		};

		class DeferredShading : public Object
		{
#include "../Shaders/DS.inc.hlsl"
#include "../Shaders/DS_Compose.inc.hlsl"

			enum S
			{
				S_LIGHT,
				S_COMPOSE,
				S_MAX
			};

			enum PIPE
			{
				PIPE_INSTANCED_DIR,
				PIPE_INSTANCED_OMNI,
				PIPE_INSTANCED_SPOT,
				PIPE_COMPOSE,
				PIPE_QUAD,
				PIPE_MAX
			};

			enum TEX
			{
				TEX_GBUFFER_0, // Albedo RGB, A = motion blur.
				TEX_GBUFFER_1, // Depth.
				TEX_GBUFFER_2, // RG - Normals in screen space, Emission, Metallicity.
				TEX_GBUFFER_3, // LamScale, LamBias, Specular, Gloss.
				TEX_LIGHT_ACC_DIFF,
				TEX_LIGHT_ACC_SPEC,
				TEX_MAX
			};

			static UB_PerFrame         s_ubPerFrame;
			static UB_PerMesh          s_ubPerMesh;
			static UB_PerObject        s_ubPerObject;
			static UB_ComposePerObject s_ubComposePerObject;

			ShaderPwns<S_MAX>      _shader;
			PipelinePwns<PIPE_MAX> _pipe;
			TexturePwns<TEX_MAX>   _tex;
			UINT64                 _frame = 0;
			int                    _width = 0;
			int                    _height = 0;
			int                    _rp = -1;
			int                    _fb = -1;
			int                    _csidLight = -1;
			int                    _csidCompose = -1;
			int                    _csidQuad[6];
			bool                   _activeGeometryPass = false;
			bool                   _activeLightingPass = false;
			bool                   _async_initPipe = false;

		public:
			DeferredShading();
			~DeferredShading();

			void Init();
			void InitGBuffers(int w, int h);
			void Done();

			static bool IsLoaded();

			void Draw(int gbuffer);

			int GetWidth() const { return _width; }
			int GetHeight() const { return _height; }

			bool IsActiveGeometryPass() const { return _activeGeometryPass; }
			bool IsActiveLightingPass() const { return _activeLightingPass; }

			int GetRenderPassID() const { return _rp; }

			void BeginGeometryPass(bool onlySetRT = false, bool spriteBaking = false);
			void EndGeometryPass(bool resetRT = false);
			bool BeginLightingPass();
			void EndLightingPass();
			void Compose();
			void AntiAliasing();

			static bool IsLightUrl(CSZ url);
			static UB_PerMesh& GetUbPerMesh() { return s_ubPerMesh; }
			void OnNewLightType(LightType type, bool wireframe = false);
			void UpdateUniformBufferPerFrame();
			void BindDescriptorsPerMesh();

			void Load();

			TexturePtr GetGBuffer(int index);
		};
		VERUS_TYPEDEFS(DeferredShading);
	}
}
