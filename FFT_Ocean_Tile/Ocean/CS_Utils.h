#pragma once
#include <stdio.h>
#include <crtdbg.h>
#include <d3dcommon.h>
#include <d3d11.h>
#include <d3dcompiler.h>

//--------------------------------------------------------------------------------------
// Create Structured Buffer
//--------------------------------------------------------------------------------------
HRESULT CreateStructuredBuffer(ID3D11Device* pDevice, UINT uElementSize, UINT uCount, void* pInitData, ID3D11Buffer** ppBufOut);

//--------------------------------------------------------------------------------------
// Create Shader Resource View for Structured Buffers
//--------------------------------------------------------------------------------------
HRESULT CreateBufferSRV(ID3D11Device* pDevice, ID3D11Buffer* pBuffer, ID3D11ShaderResourceView** ppSRVOut);

//--------------------------------------------------------------------------------------
// Create Unordered Access View for Structured Buffers
//-------------------------------------------------------------------------------------- 
HRESULT CreateBufferUAV(ID3D11Device* pDevice, ID3D11Buffer* pBuffer, ID3D11UnorderedAccessView** ppUAVOut);

//--------------------------------------------------------------------------------------
// Create a CPU accessible buffer.
//-------------------------------------------------------------------------------------- 
HRESULT CreateReaderBuffer(ID3D11Device* pDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3D11Buffer* pBuffer, ID3D11Buffer** ppBufOut);
