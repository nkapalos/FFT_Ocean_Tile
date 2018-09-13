#pragma once

// ========================================================
// Shader stage enum
// ========================================================
namespace ShaderStage
{
	enum ShaderStageEnum
	{
		kVertex,
		kHull,
		kDomain,
		kGeometry,
		kPixel,
		kCompute,

		kMaxStages
	};
}

// ========================================================
// ShaderSet
// ========================================================

// Describes the entry points for a given set of shaders
// Fill in the filename then multiple entry points.
struct ShaderSetDesc
{
	const char* filename;
	const char* entryPoints[ShaderStage::kMaxStages];

	static ShaderSetDesc Create_VS_PS(const char* fName, const char* vsEntry, const char* psEntry)
	{
		ShaderSetDesc desc = {};
		desc.filename = fName;
		desc.entryPoints[ShaderStage::kVertex] = vsEntry;
		desc.entryPoints[ShaderStage::kPixel] = psEntry;
		return desc;
	}

	//NK Function to enable Compute Shader creation along VS and PS
	static ShaderSetDesc Create_VS_PS_CS(const char* fName, const char* vsEntry, const char* psEntry, const char* csEntry)
	{
		ShaderSetDesc desc = {};
		desc.filename = fName;
		desc.entryPoints[ShaderStage::kVertex] = vsEntry;
		desc.entryPoints[ShaderStage::kPixel] = psEntry;
		desc.entryPoints[ShaderStage::kCompute] = csEntry;
		return desc;
	}

	//NK Function to only enable Compute Shader creation
	static ShaderSetDesc Create_CS(const char* fName, const char* csEntry)
	{
		ShaderSetDesc desc = {};
		desc.filename = fName;
		desc.entryPoints[ShaderStage::kCompute] = csEntry;
		return desc;
	}

};

struct ShaderSet
{
	using InputLayoutDesc = std::tuple<const D3D11_INPUT_ELEMENT_DESC *, int>;
	ShaderSet();
	void init(ID3D11Device* device, const ShaderSetDesc& desc, const InputLayoutDesc & layout);

	void bind(ID3D11DeviceContext* pContext) const;

	ComPtr<ID3D11InputLayout>  inputLayout;
	ComPtr<ID3D11VertexShader> vs;
	ComPtr<ID3D11HullShader> hs;
	ComPtr<ID3D11DomainShader> ds;
	ComPtr<ID3D11GeometryShader> gs;
	ComPtr<ID3D11PixelShader>  ps;
	ComPtr<ID3D11ComputeShader>  cs;
};


//================================================================================
// Helpers
//================================================================================


// template to create a constant buffer from CPU structure.
template<typename ConstantBufferType>
ID3D11Buffer* create_constant_buffer(ID3D11Device* pDevice)
{
	ID3D11Buffer* pBuffer = nullptr;

	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth = sizeof(ConstantBufferType);
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT hr = pDevice->CreateBuffer(&desc, NULL, &pBuffer);
	ASSERT(!FAILED(hr) && pBuffer);

	return pBuffer;
}

// template to update entire constant buffer from CPU structure.
template<typename ConstantBufferType>
void push_constant_buffer(ID3D11DeviceContext* pContext, ID3D11Buffer* pBuffer, const ConstantBufferType& rData)
{
	D3D11_MAPPED_SUBRESOURCE subresource;
	if (!FAILED(pContext->Map(pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource)))
	{
		memcpy(subresource.pData, &rData, sizeof(ConstantBufferType));
		pContext->Unmap(pBuffer, 0);
	}
}

// template to create a structure buffer from CPU structure.
template<typename StructureElementType>
ID3D11Buffer* create_structured_buffer(ID3D11Device* pDevice, u32 elements)
{
	ID3D11Buffer* pBuffer = nullptr;

	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth = sizeof(StructureElementType) * elements;
	desc.StructureByteStride = sizeof(StructureElementType);
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

	HRESULT hr = pDevice->CreateBuffer(&desc, NULL, &pBuffer);
	//ASSERT(!FAILED(hr) && pBuffer);

	return pBuffer;
}

inline ID3D11ShaderResourceView* create_structured_buffer_view(ID3D11Device* pDevice, ID3D11Buffer* pBuffer)
{
	ID3D11ShaderResourceView* pView = nullptr;
	HRESULT hr = pDevice->CreateShaderResourceView(pBuffer, NULL, &pView);
	//ASSERT(!FAILED(hr) && pView);
	return pView;
}

// helper to create a sampler state
inline ID3D11SamplerState* create_basic_sampler(ID3D11Device* pDevice, D3D11_TEXTURE_ADDRESS_MODE mode)
{
	ID3D11SamplerState* pSampler = nullptr;

	D3D11_SAMPLER_DESC desc = {};
	desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	desc.AddressU = mode;
	desc.AddressV = mode;
	desc.AddressW = mode;
	desc.MinLOD = 0.f;
	desc.MaxLOD = D3D11_FLOAT32_MAX;

	HRESULT hr = pDevice->CreateSamplerState(&desc, &pSampler);
	ASSERT(!FAILED(hr) && pSampler);

	return pSampler;
}