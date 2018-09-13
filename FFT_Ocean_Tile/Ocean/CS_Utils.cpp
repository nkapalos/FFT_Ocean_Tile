#pragma once
#include "CS_Utils.h"

//--------------------------------------------------------------------------------------
// Create Structured Buffer
//--------------------------------------------------------------------------------------
HRESULT CreateStructuredBuffer(ID3D11Device* pDevice, UINT uElementSize, UINT uCount, void* pInitData, ID3D11Buffer** ppBufOut)
{
	*ppBufOut = nullptr;

	D3D11_BUFFER_DESC desc = {};
	desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	desc.ByteWidth = uElementSize * uCount;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = uElementSize;

	if (pInitData)
	{
		D3D11_SUBRESOURCE_DATA InitData;
		InitData.pSysMem = pInitData;
		return pDevice->CreateBuffer(&desc, &InitData, ppBufOut);
	}
	else
		return pDevice->CreateBuffer(&desc, nullptr, ppBufOut);
}

//--------------------------------------------------------------------------------------
// Create Shader Resource View for Structured Buffers
//--------------------------------------------------------------------------------------
HRESULT CreateBufferSRV(ID3D11Device* pDevice, ID3D11Buffer* pBuffer, ID3D11ShaderResourceView** ppSRVOut)
{
	D3D11_BUFFER_DESC descBuf = {};
	pBuffer->GetDesc(&descBuf);

	D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	desc.BufferEx.FirstElement = 0;

	if (descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED)
	{
		// This is a Structured Buffer
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.BufferEx.NumElements = descBuf.ByteWidth / descBuf.StructureByteStride;
	}
	else
	{
		return E_INVALIDARG;
	}

	return pDevice->CreateShaderResourceView(pBuffer, &desc, ppSRVOut);
}

//--------------------------------------------------------------------------------------
// Create Unordered Access View for Structured Buffers
//-------------------------------------------------------------------------------------- 
HRESULT CreateBufferUAV(ID3D11Device* pDevice, ID3D11Buffer* pBuffer, ID3D11UnorderedAccessView** ppUAVOut)
{
	D3D11_BUFFER_DESC descBuf = {};
	pBuffer->GetDesc(&descBuf);

	D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
	desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	desc.Buffer.FirstElement = 0;

	if (descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED)
	{
		// This is a Structured Buffer
		desc.Format = DXGI_FORMAT_UNKNOWN;      // Format must be must be DXGI_FORMAT_UNKNOWN, when creating a View of a Structured Buffer
		desc.Buffer.NumElements = descBuf.ByteWidth / descBuf.StructureByteStride;
	}
	else
	{
		return E_INVALIDARG;
	}

	return pDevice->CreateUnorderedAccessView(pBuffer, &desc, ppUAVOut);
}

//--------------------------------------------------------------------------------------
// Create a CPU accessible buffer
//-------------------------------------------------------------------------------------- 
HRESULT CreateReaderBuffer(ID3D11Device* pDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3D11Buffer* pBuffer, ID3D11Buffer** ppBufOut)
{
	*ppBufOut = nullptr;

	D3D11_BUFFER_DESC desc = {};
	pBuffer->GetDesc(&desc);

	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.Usage = D3D11_USAGE_STAGING;
	desc.BindFlags = 0;
	desc.MiscFlags = 0;
	return pDevice->CreateBuffer(&desc, nullptr, ppBufOut);
		
}

