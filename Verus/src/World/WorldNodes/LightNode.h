// Copyright (C) 2021-2022, Dmitry Maluev (dmaluev@gmail.com). All rights reserved.
#pragma once

namespace verus::World
{
	class LightData
	{
	public:
		Vector4        _color = Vector4(1, 1, 1, 1000);
		Vector3        _dir = Vector3(0, 0, 1);
		CGI::LightType _lightType = CGI::LightType::omni;
		float          _radius = 10;
		float          _coneIn = cos(Math::ToRadians(20));
		float          _coneOut = cos(Math::ToRadians(30));
	};
	VERUS_TYPEDEFS(LightData);

	// LightNode defines a light source.
	// LightNode's scale is tied to the radius.
	// * has different light parameters, like color, radius, etc.
	class LightNode : public BaseNode
	{
		Matrix4         _matShadow = Matrix4::identity();
		LightData       _data;
		ShadowMapHandle _shadowMapHandle;
		float           _cachedInfluence = 0;
		bool            _shadowMapUpdateRequired = false;
		bool            _async_loadedMesh = false;

	public:
		struct Desc : BaseNode::Desc
		{
			LightData _data;
			bool      _dynamic = false;
		};
		VERUS_TYPEDEFS(Desc);

		LightNode();
		virtual ~LightNode();

		void Init(RcDesc desc);
		void Done();

		virtual void Duplicate(RBaseNode node, HierarchyDuplication hierarchyDuplication) override;

		virtual void Update() override;
		virtual void DrawEditorOverlays(DrawEditorOverlaysFlags flags) override;

		// <Flags>
		virtual void SetShadowFlag(bool shadow = true) override;
		bool CanReserveShadow() const;
		void SetReserveShadowFlag(bool reserveShadow);
		// </Flags>

		virtual float GetPropertyByName(CSZ name) const override;
		virtual void SetPropertyByName(CSZ name, float value) override;

		Vector3 ComputeScale();
		virtual void OnLocalTransformUpdated() override;

		virtual void UpdateBounds() override;

		virtual void OnNodeTransformed(PBaseNode pNode, bool afterEvent) override;

		virtual void Serialize(IO::RSeekableStream stream) override;
		virtual void Deserialize(IO::RStream stream) override;
		void Deserialize_LegacyXXX(IO::RStream stream);
		void DeserializeXML_LegacyXXX(pugi::xml_node node);

		CGI::LightType GetLightType() const;
		void SetLightType(CGI::LightType type);
		RcVector4 GetColor() const;
		void SetColor(RcVector4 color);
		float GetIntensity() const;
		void SetIntensity(float i);
		float GetRadius() const;
		void SetRadius(float r);
		float GetConeIn() const;
		void SetConeIn(float coneIn);
		float GetConeOut() const;
		void SetConeOut(float coneOut);
		Vector4 GetInstData() const;
		float ComputeLampRadius() const;

		// <Shadow>
		ShadowMapHandle GetShadowMapHandle() const { return _shadowMapHandle; }
		void UpdateShadowMapHandle(bool freeCell = false);
		bool CanRequestShadowMapUpdate() const;
		void RequestShadowMapUpdate(bool mustBeStatic = true);
		bool IsShadowMapUpdateRequired() const;
		void OnShadowMapUpdated();
		bool CanBeSmbpStatic() const;
		bool CanBeSmbpDynamic() const;

		void SetupShadowMapCamera(RCamera camera);

		RcMatrix4 GetShadowMatrix() const;
		Matrix4 GetShadowMatrixForDS() const;
		// </Shadow>

		float GetInfluenceAt(Math::RcSphere sphere);
		float GetCachedInfluence() const { return _cachedInfluence; }
	};
	VERUS_TYPEDEFS(LightNode);

	class LightNodePtr : public Ptr<LightNode>
	{
	public:
		void Init(LightNode::RcDesc desc);
		void Duplicate(RBaseNode node, HierarchyDuplication hierarchyDuplication);
	};
	VERUS_TYPEDEFS(LightNodePtr);

	class LightNodePwn : public LightNodePtr
	{
	public:
		~LightNodePwn() { Done(); }
		void Done();
	};
	VERUS_TYPEDEFS(LightNodePwn);
}
