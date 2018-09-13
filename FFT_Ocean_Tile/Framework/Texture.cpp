#include "Texture.h"
#include "DirectXTK/DDSTextureLoader.h"
#include "DirectXTK/WICTextureLoader.h"

Texture::Texture()
	: m_pTexture(nullptr)
	, m_pTextureSRV(nullptr)
	, m_pTextureUAV(nullptr)
{

}

Texture::~Texture()
{
	SAFE_RELEASE(m_pTextureUAV);
	SAFE_RELEASE(m_pTextureSRV);
	SAFE_RELEASE(m_pTexture);
}

void Texture::init_custom(ID3D11Device* pDevice, const int& texSize, const bool& isDynamic)
{

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = texSize;
	desc.Height = texSize;
	desc.MipLevels = desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.SampleDesc.Count = 1;
	
	// Dynamic Textures cannot have UAVs
	if (isDynamic)
	{
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	}
	else
	{
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.CPUAccessFlags = 0;
		desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	}
	desc.MiscFlags = 0;

	ID3D11Texture2D* tex;
	HRESULT hr = pDevice->CreateTexture2D(&desc, nullptr, &tex);

	m_pTexture = tex;

	if (SUCCEEDED(hr))
	{
		// Create Shader Resource View 
		D3D11_SHADER_RESOURCE_VIEW_DESC SRVdesc = {};
		SRVdesc.Format = desc.Format;
		SRVdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRVdesc.Texture2D.MipLevels = 1;

		hr = pDevice->CreateShaderResourceView(tex, &SRVdesc, &m_pTextureSRV);

		// Create Unordered Access View
		D3D11_UNORDERED_ACCESS_VIEW_DESC UAVdesc = {};
		UAVdesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		UAVdesc.Format = desc.Format;
		UAVdesc.Texture2D.MipSlice = 0;

		hr = pDevice->CreateUnorderedAccessView(m_pTexture, &UAVdesc, &m_pTextureUAV);
		bool testHR = (SUCCEEDED(hr));
	}
}
	

void Texture::init_from_dds(ID3D11Device* pDevice, const char* pFilename)
{
	wchar_t fileNameW[MAX_PATH];
	size_t numChars;
	mbstowcs_s(&numChars, fileNameW, MAX_PATH, pFilename, MAX_PATH);

	HRESULT hr = DirectX::CreateDDSTextureFromFile(pDevice, fileNameW, &m_pTexture, &m_pTextureSRV);
	if (FAILED(hr))
	{
		panicF("Could not load texture : %s ", pFilename);
	}
}

void Texture::init_from_image(ID3D11Device* pDevice, const char* pFilename, bool bGenerateMips, bool bIsDynamic)
{
	wchar_t fileNameW[MAX_PATH];
	size_t numChars;
	mbstowcs_s(&numChars, fileNameW, MAX_PATH, pFilename, MAX_PATH);

	HRESULT hr;

	if(bIsDynamic)
		hr = DirectX::CreateWICTextureFromFileEx(pDevice, fileNameW, 0, D3D11_USAGE_DYNAMIC, D3D11_BIND_SHADER_RESOURCE, D3D11_CPU_ACCESS_WRITE, 0, 0, &m_pTexture, &m_pTextureSRV);
	else
		hr = DirectX::CreateWICTextureFromFile(pDevice, fileNameW, &m_pTexture, &m_pTextureSRV);

	if (FAILED(hr))
	{
		panicF("Could not load texture : %s ", pFilename);
	}
}

void Texture::bind(ID3D11DeviceContext* pDeviceContext, ShaderStage::ShaderStageEnum stage, u32 slot)
{
	// This is not very efficient.
	// We could bind multiple in a single call.
	// An opportunity for you to change ;)

	switch (stage)
	{
	case ShaderStage::kVertex:
		pDeviceContext->VSSetShaderResources(slot, 1, &m_pTextureSRV);
		break;
	case ShaderStage::kHull:
		pDeviceContext->HSSetShaderResources(slot, 1, &m_pTextureSRV);
		break;
	case ShaderStage::kDomain:
		pDeviceContext->DSSetShaderResources(slot, 1, &m_pTextureSRV);
		break;
	case ShaderStage::kGeometry:
		pDeviceContext->GSSetShaderResources(slot, 1, &m_pTextureSRV);
		break;
	case ShaderStage::kPixel:
		pDeviceContext->PSSetShaderResources(slot, 1, &m_pTextureSRV);
		break;
	case ShaderStage::kCompute:
		pDeviceContext->CSSetShaderResources(slot, 1, &m_pTextureSRV);
		break;
	}
}
