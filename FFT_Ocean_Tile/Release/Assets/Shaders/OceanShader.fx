///////////////////////////////////////////////////////////////////////////////
// Ocean Shader Effect.
// Shader to handle oceanic movement.
///////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// Vertex Shader Input
////////////////////////////////////////////////////////////////
struct VertexInput
{
	float3 pos   : POSITION;
	float4 color : COLOUR;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

/////////////////////////////////////////////////////////////////
// Fragment Shader Input
/////////////////////////////////////////////////////////////////
struct PixelInput
{
	float4 pos : SV_POSITION;
	float3 normal : NORMAL;
	float3 worldPos : COLOUR;
	float2 uv : TEXCOORD0;
};

/////////////////////////////////////////////////////////////////
// Effect Constant Buffers
/////////////////////////////////////////////////////////////////
cbuffer PerFrameCB : register(b0)
{
	matrix matProjection;
	matrix matView;
	float4 vViewPos;
	float  gridSize;
	float  lambda;
	float  heightAdjust;
	float  reflectivity;
};

cbuffer PerDrawCB : register(b1)
{
	matrix matMVP;
	matrix matModel;
};

cbuffer csCB : register(b2)
{
	float m_time;
	float m_gridSize;
	float m_padding2;
	float m_padding3;
};

/////////////////////////////////////////////////////////////////
// Textures
/////////////////////////////////////////////////////////////////
Texture2D<float4> g_Tex0 : register(t0);
Texture2D<float4> g_TexNormals : register(t1);
Texture2D<float4> g_TexFoam : register(t2);
TextureCube g_TexSky : register(t3);
SamplerState g_Sampler0 : register (s0);

/////////////////////////////////////////////////////////////////
// Vertex Shader
/////////////////////////////////////////////////////////////////

PixelInput VS_Ocean(VertexInput IN)
{
	PixelInput OUT;

	// Constants
	float4 oceanColor = float4(0.2f, 0.36f, 0.6f, 1);

	// Sampling
	uint3 sampleCoord = uint3((gridSize-1) * IN.uv, 0);
	float3 distex = g_Tex0.Load(sampleCoord);
	float3 normals = g_TexNormals.Load(sampleCoord);

	// Calculate Displacement and Normals
	float3 displacement = float3(lambda * (-distex.x), heightAdjust * (distex.y), lambda * (-distex.z));
	IN.pos += displacement;
	IN.normal = normalize(normals);

	// World position
	float4 worldPos = mul(float4(IN.pos, 1.0), matModel);

	// PixelInput components
	OUT.pos = mul(mul(mul(float4(IN.pos, 1.0), matModel), matView), matProjection);
	OUT.uv = IN.uv;
	OUT.normal = IN.normal;
	OUT.worldPos = worldPos.xyz;

	return OUT;
}

/////////////////////////////////////////////////////////////////
// Fragment Shader
/////////////////////////////////////////////////////////////////
float4 PS_Ocean(PixelInput IN) : SV_TARGET
{
	// Constants
	float4 oceanColor = float4(0.14f, 0.27f, 0.47f, 1);

	// Light Presets
	float3 lightDir = normalize(float3(0.2f, -1.0f, 0.3f));
	float ambient = 0.3f;

	// Calculate Reflection
	float3 normal = normalize(IN.normal);
	float3 eyeVector = normalize(IN.worldPos - vViewPos.xyz);
	float3 reflection = normalize(reflect(eyeVector, normal));

	// Sampling
	float4 heightSample = g_Tex0.Sample(g_Sampler0, IN.uv);
	float4 normalSample = g_TexNormals.Sample(g_Sampler0, IN.uv);
	float4 reflectionColour = g_TexSky.Sample(g_Sampler0, reflection);
	float4 foamSample = g_TexFoam.Sample(g_Sampler0, IN.uv * 2);

	// Color + Lighting calculation
	float brightness = max(dot(-lightDir, normal), 0.0f) + ambient;
	float4 lightedColour = oceanColor * brightness;

	// Add Foam by using the folding map, stored into normalSample.w
	float4 foamlessColour = lerp(lightedColour, reflectionColour, reflectivity);
	return foamlessColour + (normalSample.w * foamSample);
}

/////////////////////////////////////////////////////////////////
// Compute Shader Buffers
/////////////////////////////////////////////////////////////////

StructuredBuffer<float> kMag : register(t4);
StructuredBuffer<float2> h0t : register(t5);
StructuredBuffer<float2> h0tc : register(t6);
RWStructuredBuffer<float2> hTilde : register(u0);

// Helper functions
float2 addComplex(float2 a, float2 b)
{
	float2 ret;
	ret.x = a.x + b.x;
	ret.y = a.y + b.y;
	return ret;
}

float2 multComplex(float2 a, float2 b)
{
	float2 ret;
	ret.x = a.x * b.x - a.y * b.y;
	ret.y = a.x * b.y + a.y * b.x;
	return ret;
}

/////////////////////////////////////////////////////////////////
// Compute Shader
/////////////////////////////////////////////////////////////////

[numthreads(4, 4, 1)]
void CS_Ocean(uint3 DTid : SV_DispatchThreadID)
{
	uint Ind = uint(DTid.x * m_gridSize + DTid.y);
	float omegaK = sqrt(9.81f * kMag[Ind]);
	float waveCos = cos(omegaK * m_time);
	float waveSin = sin(omegaK * m_time);
	float2 expPos = { waveCos, waveSin };
	float2 expNeg = { waveCos, -waveSin };

	hTilde[Ind] = addComplex(multComplex(h0t[Ind], expPos), multComplex(h0tc[Ind], expNeg));
}
