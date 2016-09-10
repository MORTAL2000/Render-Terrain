cbuffer PerFrameData : register(b0)
{
	float4x4 viewproj;
	float4x4 shadowtexmatrices[4];
	float4 eye;
	float4 frustum[6];
}

cbuffer TerrainData : register(b1)
{
	float scale;
	float width;
	float depth;
	float base;
}

Texture2D<float> heightmap : register(t0);
Texture2D<float4> displacementmap : register(t2);

SamplerState hmsampler : register(s0);
SamplerState detailsampler : register(s1);
SamplerState displacementsampler : register(s3);

struct DS_OUTPUT
{
	float4 pos : SV_POSITION;
	float4 shadowpos[4] : TEXCOORD0;
	float3 worldpos : POSITION;
};

// Output control point
struct HS_CONTROL_POINT_OUTPUT
{
	float3 worldpos : POSITION;
};

// Output patch constant data.
struct HS_CONSTANT_DATA_OUTPUT
{
	float EdgeTessFactor[4]			: SV_TessFactor; // e.g. would be [4] for a quad domain
	float InsideTessFactor[2]		: SV_InsideTessFactor; // e.g. would be Inside[2] for a quad domain
	uint skirt : SKIRT;
};

#define NUM_CONTROL_POINTS 4

float3 estimateNormal(float2 texcoord) {
	float2 b = texcoord + float2(0.0f, -0.3f / depth);
	float2 c = texcoord + float2(0.3f / width, -0.3f / depth);
	float2 d = texcoord + float2(0.3f / width, 0.0f);
	float2 e = texcoord + float2(0.3f / width, 0.3f / depth);
	float2 f = texcoord + float2(0.0f, 0.3f / depth);
	float2 g = texcoord + float2(-0.3f / width, 0.3f / depth);
	float2 h = texcoord + float2(-0.3f / width, 0.0f);
	float2 i = texcoord + float2(-0.3f / width, -0.3f / depth);

	float zb = heightmap.SampleLevel(hmsampler, b, 0) * scale;
	float zc = heightmap.SampleLevel(hmsampler, c, 0) * scale;
	float zd = heightmap.SampleLevel(hmsampler, d, 0) * scale;
	float ze = heightmap.SampleLevel(hmsampler, e, 0) * scale;
	float zf = heightmap.SampleLevel(hmsampler, f, 0) * scale;
	float zg = heightmap.SampleLevel(hmsampler, g, 0) * scale;
	float zh = heightmap.SampleLevel(hmsampler, h, 0) * scale;
	float zi = heightmap.SampleLevel(hmsampler, i, 0) * scale;

	float x = zg + 2 * zh + zi - zc - 2 * zd - ze;
	float y = 2 * zb + zc + zi - ze - 2 * zf - zg;
	float z = 8.0f;

	return normalize(float3(x, y, z));
}

// takes x, y, and z texture coordinates and the normal and performs tri-planar mapping to
// calculate the displacement value from the displacement map.
float triplanar_displacement(float3 tex, float3 norm) {
	float3 blending = abs(norm);
	blending = normalize(max(blending, 0.00001)); // force weights to sum to 1.0
//	blending = pow(max(blending, 0.0), 0.5f);
	float b = blending.x + blending.y + blending.z;
	blending /= float3(b, b, b);

	float x = displacementmap.SampleLevel(displacementsampler, tex.yz, 0.0f).w;
	float y = displacementmap.SampleLevel(displacementsampler, tex.xz, 0.0f).w;
	float z = displacementmap.SampleLevel(displacementsampler, tex.xy, 0.0f).w;
	float h = x * blending.x + y * blending.y + z * blending.z;
	h =  h - 0.5f;

	return h;
}

[domain("quad")]
DS_OUTPUT main(
	HS_CONSTANT_DATA_OUTPUT input,
	float2 domain : SV_DomainLocation,
	const OutputPatch<HS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> patch)
{
	DS_OUTPUT output;

	output.worldpos = lerp(lerp(patch[0].worldpos, patch[1].worldpos, domain.x), lerp(patch[2].worldpos, patch[3].worldpos, domain.x), domain.y);
	
	float h;
	if (input.skirt < 5) {
		if (input.skirt > 0 && domain.y == 1) {
			h = heightmap.SampleLevel(hmsampler, output.worldpos / width, 0.0f);
			output.worldpos.z = h * scale;
		}
	} else {
		h = heightmap.SampleLevel(hmsampler, output.worldpos / width, 0.0f);
		output.worldpos.z = h * scale;
	}
	
	float3 norm = estimateNormal(output.worldpos / width);
	output.worldpos += norm * 0.5f * (2.0f * displacementmap.SampleLevel(displacementsampler, output.worldpos / 32, 0.0f).w - 1.0f);
	//output.worldpos += norm * triplanar_displacement(output.worldpos / 32, norm);

	// generate coordinates transformed into view/projection space.
	output.pos = float4(output.worldpos, 1.0f);
	output.pos = mul(output.pos, viewproj);

	[unroll]
	for (int i = 0; i < 4; ++i) {
		// generate projective tex-coords to project shadow map onto scene.
		output.shadowpos[i] = float4(output.worldpos, 1.0f);

		output.shadowpos[i] = mul(output.shadowpos[i], shadowtexmatrices[i]);
	}
	return output;
}
