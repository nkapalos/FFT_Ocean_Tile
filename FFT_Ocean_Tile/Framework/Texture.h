#pragma once

#include "CommonHeader.h"
#include "ShaderSet.h"

class Texture
{

public:

	Texture();
	~Texture();

	// Getters
	inline ID3D11Resource* getTexture() { return m_pTexture; }
	inline ID3D11ShaderResourceView* getSRV() { return m_pTextureSRV; }
	inline ID3D11UnorderedAccessView* getUAV() { return m_pTextureUAV; }

	// Custom initialisation that can also set the Unordered Access View
	// for compute shader output scenarios
	void init_custom(ID3D11Device* pDevice, const int& texSize, const bool& isDynamic);

	// Initialize from a DDS file.
	void init_from_dds(ID3D11Device* pDevice, const char* pFilename);

	// Initialize from a non-dds image files such as JPEG, or PNG
	void init_from_image(ID3D11Device* pDevice, const char* pFilename, bool bGenerateMips, bool bIsDynamic);

	// bind to the pipeline on a particular shader and slot
	void bind(ID3D11DeviceContext* pDeviceContext, ShaderStage::ShaderStageEnum stage, u32 slot);

private:
	ID3D11Resource* m_pTexture;
	ID3D11ShaderResourceView* m_pTextureSRV;
	ID3D11UnorderedAccessView* m_pTextureUAV;

};

