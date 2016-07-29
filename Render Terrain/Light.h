#pragma once

#include <DirectXMath.h>
#include <Windows.h>

using namespace DirectX;

struct LightSource {
	LightSource() { ZeroMemory(this, sizeof(this)); }

	XMFLOAT4	pos;
	XMFLOAT4	intensityAmbient;
	XMFLOAT4	intensityDiffuse;
	XMFLOAT4	intensitySpecular;
	XMFLOAT3	attenuation;
	float		range;
	XMFLOAT3	direction;
	float		spotlightConeExponent;
};

class Light {
public:
	Light() {}
	Light(XMFLOAT4 p, XMFLOAT4 amb, XMFLOAT4 dif, XMFLOAT4 spec, XMFLOAT3 att, float r, XMFLOAT3 dir, float sexp);
	~Light() {}

	LightSource GetLight() { return mlsLightData; }
	void SetLightDirection(XMFLOAT3 dir) { mlsLightData.direction = dir; }

protected:
	LightSource mlsLightData;
};

