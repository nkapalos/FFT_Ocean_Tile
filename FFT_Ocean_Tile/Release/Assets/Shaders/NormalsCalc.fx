///////////////////////////////////////////////////////////////////////////////
// Normals and Jacobian Calculation.
// Compute Shader to handle Normals and Jacobian calculations.
// Generate appropriate texture to store the results.
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////
// Constant Buffer
/////////////////////////////////////////////////////////////////
cbuffer NormCalcCB : register(b0)
{
	float  lambda;
	float  heightAdjust;
	float  foamInt;
	float  texSize;
};

/////////////////////////////////////////////////////////////////
// Textures
/////////////////////////////////////////////////////////////////
Texture2D<float4> texHeightmap : register(t0);
RWTexture2D<float4> texNormals : register(u0);

/////////////////////////////////////////////////////////////////
// Compute Shader
/////////////////////////////////////////////////////////////////

[numthreads(4, 4, 1)]
void CS_Normals(uint3 DTid : SV_DispatchThreadID)
{
	// Indices of neighbouring texels
	uint2 xNext, zNext;
	
	if (DTid.x == texSize - 1)
		xNext = uint2(0, DTid.y);
	else
		xNext = uint2(DTid.x + 1, DTid.y);

	if (DTid.y == texSize - 1)
		zNext = uint2(DTid.x, 0);
	else
		zNext = uint2(DTid.x, DTid.y + 1);


	float s11, s21, s12;
	float Jxx, Jyy, Jxy, Jyx, jacobian;
	int jacobianVal;
	float3 va, vb, normals;

	// Normals and Jacobian calculation
	s11 = heightAdjust * texHeightmap[DTid.xy].y;

	s21 = heightAdjust * texHeightmap[xNext].y;
	Jxx = 1 + lambda * (-texHeightmap[xNext].x + texHeightmap[DTid.xy].x) * foamInt;
	Jxy = lambda * (-texHeightmap[xNext].z + texHeightmap[DTid.xy].z) * foamInt;

	s12 = heightAdjust * texHeightmap[zNext].y;
	Jyy = 1 + lambda * (-texHeightmap[zNext].z + texHeightmap[DTid.xy].z) * foamInt;
	Jyx = lambda * (-texHeightmap[zNext].x + texHeightmap[DTid.xy].x) * foamInt;

	va = normalize(float3(2.0f, 0.0f, s21 - s11));
	vb = normalize(float3(0.0f, 2.0f, s12 - s11));
	normals = cross(va, vb);
	jacobian = (Jxx * Jyy) - (Jxy * Jyx);
	
	if (jacobian < 0)
		jacobianVal = 1.0f;
	else
		jacobianVal = 0.0f;

	texNormals[DTid.xy] = float4(normals.xzy, jacobianVal);
}
