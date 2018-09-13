///////////////////////////////////////////////////////////////////////////////
// UI Shader Effect.
// Shader to handle UI Display.
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
	float2 uv : TEXCOORD0;
	float3 normal : NORMAL;
	float3 viewDir : TEXCOORD1;
};


/////////////////////////////////////////////////////////////////
// Textures
/////////////////////////////////////////////////////////////////
Texture2D<float4> g_TexDispl : register(t0);
Texture2D<float4> g_TexNormal : register(t1);
SamplerState g_Sampler0 : register (s0);


/////////////////////////////////////////////////////////////////
// Vertex Shader
/////////////////////////////////////////////////////////////////

PixelInput VS_UI(VertexInput IN)
{
	PixelInput OUT;

	OUT.pos = float4(IN.pos, 1.0);
	OUT.uv = IN.uv;
	OUT.normal = IN.normal;

	return OUT;
}

/////////////////////////////////////////////////////////////////
// Fragment Shader
/////////////////////////////////////////////////////////////////
float4 PS_UI(PixelInput IN) : SV_TARGET
{	
	// Texture heightmap variables
	int size = 80;
	int size2 = 256;
	int locationX1 = 10;
	int locationX2 = 110;
	int locationX3 = 210;
	int locationX4 = 310;
	int locationY1 = 280;
	int locationY2 = 380;
	int locationY3 = 480;
	int locationY4 = 580;

	if (IN.pos.y > locationY1 && IN.pos.y < locationY1 + size)
	{
		// Horizontal Disposition (X-Axis)
		if (IN.pos.x > locationX1 && IN.pos.x < locationX1 + size)
		{
			float2 coords = float2((IN.pos.x - locationX1) / size, (IN.pos.y - locationY1) / size);
			float4 smpl = g_TexDispl.Sample(g_Sampler0, coords);
			return float4(smpl.xxx, 1);
		}

		// Heightmap (Y-Axis Disposition)
		if (IN.pos.x > locationX2 && IN.pos.x < locationX2 + size)
		{
			float2 coords = float2((IN.pos.x - locationX2) / size, (IN.pos.y - locationY1) / size);
			float4 smpl = g_TexDispl.Sample(g_Sampler0, coords);
			return float4(smpl.yyy, 1);
		}

		// Horizontal Disposition (Z-Axis)
		if (IN.pos.x > locationX3 && IN.pos.x < locationX3 + size)
		{
			float2 coords = float2((IN.pos.x - locationX3) / size, (IN.pos.y - locationY1) / size);
			float4 smpl = g_TexDispl.Sample(g_Sampler0, coords);
			return float4(smpl.zzz, 1);
		}
	}

	if (IN.pos.y > locationY2 && IN.pos.y < locationY2 + size)
	{
		// Normal (X-Axis)
		if (IN.pos.x > locationX1 && IN.pos.x < locationX1 + size)
		{
			float2 coords = float2((IN.pos.x - locationX1) / size, (IN.pos.y - locationY2) / size);
			float4 smpl = g_TexNormal.Sample(g_Sampler0, coords);
			return float4(smpl.xxx, 1);
		}

		// Normal (Y-Axis)
		if (IN.pos.x > locationX2 && IN.pos.x < locationX2 + size)
		{
			float2 coords = float2((IN.pos.x - locationX2) / size, (IN.pos.y - locationY2) / size);
			float4 smpl = g_TexNormal.Sample(g_Sampler0, coords);
			return float4(smpl.yyy, 1);
		}

		// Normal (Z-Axis)
		if (IN.pos.x > locationX3 && IN.pos.x < locationX3 + size)
		{
			float2 coords = float2((IN.pos.x - locationX3) / size, (IN.pos.y - locationY2) / size);
			float4 smpl = g_TexNormal.Sample(g_Sampler0, coords);
			return float4(smpl.zzz, 1);
		}

		// Jacobian
		if (IN.pos.x > locationX4 && IN.pos.x < locationX4 + size)
		{
			float2 coords = float2((IN.pos.x - locationX4) / size, (IN.pos.y - locationY2) / size);
			float4 smpl = g_TexNormal.Sample(g_Sampler0, coords);
			return float4(smpl.www, 1);
		}
	}

	if (IN.pos.y > locationY3 && IN.pos.y < locationY3 + size2)
	{
		// Displacements - All channels in RGB
		if (IN.pos.x > locationX1 && IN.pos.x < locationX1 + size2)
		{
			float2 coords = float2((IN.pos.x - locationX1) / size2, (IN.pos.y - locationY3) / size2);
			float4 smpl = g_TexDispl.Sample(g_Sampler0, coords);
			return float4(smpl.xyz, 1);
		}

		// Normals - All channels in RGB
		if (IN.pos.x > locationX4 && IN.pos.x < locationX4 + size2)
		{
			float2 coords = float2((IN.pos.x - locationX4) / size2, (IN.pos.y - locationY3) / size2);
			float4 smpl = g_TexNormal.Sample(g_Sampler0, coords);
			return float4(smpl.xzy, 1);
		}
	}

	return float4(0,0,0,0);
}

