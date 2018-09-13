#include "Framework.h"
#include "ShaderSet.h"
#include "Mesh.h"
#include "Texture.h"
#include "CS_Utils.h"

#include "Configurations.h"
#include "FFTWrapper.h"
#include "OceanTile.h"

#include <d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")

#include <time.h>

//================================================================================
//
//                 External library only supports x86 builds.
//             Different execution configurations can be tested by 
//             uncommenting the #define macros in Configurations.h
//
//================================================================================


//================================================================================
// Constants
//================================================================================
constexpr float kBlendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
constexpr UINT kSampleMask = 0xffffffff;

//================================================================================
// OceanApp extends FrameworkApp
// Framework provided by Dr David Moore.
//================================================================================
class OceanApp : public FrameworkApp
{
public:

	// Constant buffer per frame
	struct PerFrameCBData
	{
		m4x4 m_matProjection;
		m4x4 m_matView;
		v4	 m_vViewPos;
		f32	 m_gridSize;
		f32	 m_choppy;
		f32	 m_heightAdjust;
		f32	 m_reflectivity;
	};

	// Constant buffer per draw
	struct PerDrawCBData
	{
		m4x4 m_matMVP;
		m4x4 m_matModel;
	};

	// Constant buffer for htilde calculation CS
	struct OceanCsCBData
	{
		f32	 m_time;
		f32	 m_gridSize;
		f32	 m_padding1;
		f32	 m_padding2;
	};

	// Constant buffer for normals calculation CS
	struct NormCsCBData
	{
		f32	 m_choppy;
		f32	 m_heightAdjust;
		f32	 m_foamIntensity;
		f32	 m_texSize;
	};

	void on_init(SystemsInterface& systems) override
	{
		// Camera position and rotation initialisation
		systems.pCamera->eye = v3(250.f, 670.f, 1250.f);
		systems.pCamera->look_at(v3(300.f, 0.f, 300.f));

		// Random Seed
		srand(time(NULL));

		//--------------------- Shaders Compilation ---------------------//
		// compile the ocean shader.
		m_oceanShader.init(systems.pD3DDevice
			, ShaderSetDesc::Create_VS_PS_CS("Assets/Shaders/OceanShader.fx", "VS_Ocean", "PS_Ocean", "CS_Ocean")
			, { VertexFormatTraits<MeshVertex>::desc, VertexFormatTraits<MeshVertex>::size }
		);

		// compile the normals calculation shader.
		m_normalsCShader.init(systems.pD3DDevice
			, ShaderSetDesc::Create_CS("Assets/Shaders/NormalsCalc.fx", "CS_Normals")
			, { VertexFormatTraits<MeshVertex>::desc, VertexFormatTraits<MeshVertex>::size }
		);
		
		// compile the UI shader.
		m_UIshader.init(systems.pD3DDevice
			, ShaderSetDesc::Create_VS_PS("Assets/Shaders/UIshader.fx", "VS_UI", "PS_UI")
			, { VertexFormatTraits<MeshVertex>::desc, VertexFormatTraits<MeshVertex>::size }
		);

		// compile the skymap shader.
		m_skyMapShader.init(systems.pD3DDevice
			, ShaderSetDesc::Create_VS_PS("Assets/Shaders/SkyMapShader.fx", "VS_Skymap", "PS_Skymap")
			, { VertexFormatTraits<MeshVertex>::desc, VertexFormatTraits<MeshVertex>::size }
		);


		//--------------------- Meshes Initialisation ---------------------//
		// UI Mesh initialisation
		create_mesh_quad_xy(systems.pD3DDevice, m_UIMesh, 1);

		// SkyMap Mesh initialisation
		create_mesh_sphere(systems.pD3DDevice, m_skyMesh, 10, 10);

		// Tile Mesh generation
		Tile.GenerateMesh(systems.pD3DDevice, m_oceanMesh);


		//--------------------- Constant Buffers Initialisation ---------------------//
		// Create Per Frame Constant Buffer.
		m_pPerFrameCB = create_constant_buffer<PerFrameCBData>(systems.pD3DDevice);
		m_perFrameCBData.m_gridSize = SIZE_OF_GRID;

		// Create Per Draw Constant Buffer.
		m_pPerDrawCB = create_constant_buffer<PerDrawCBData>(systems.pD3DDevice);		

		// Create cs Buffer.
		m_pOceanCsCB = create_constant_buffer<OceanCsCBData>(systems.pD3DDevice);
		m_oceanCsCBData.m_gridSize = SIZE_OF_GRID;

		// Create Normals Calculation CS Buffer.
		m_pNormCsCB = create_constant_buffer<NormCsCBData>(systems.pD3DDevice);
		m_normCsCBData.m_texSize = SIZE_OF_GRID;

		// We need a sampler state to define wrapping and mipmap parameters.
		m_pSamplerState = create_basic_sampler(systems.pD3DDevice, D3D11_TEXTURE_ADDRESS_WRAP);


		//--------------------- Textures Initialisation ---------------------//
		// Initialise heightmap texture
		m_heightmapTexture.init_custom(systems.pD3DDevice, SIZE_OF_GRID, true);

		// Initialise normalmap texture
#ifdef GPGPU_NORM_CD
		m_normalmapTexture.init_custom(systems.pD3DDevice, SIZE_OF_GRID, false);
#endif // GPGPU_NORM_CD
#if defined(CPU_NORM_CD) | defined(CPU_NORM_FFT)
		m_normalmapTexture.init_custom(systems.pD3DDevice, SIZE_OF_GRID, true);
#endif // CPU_NORM_CD | CPU_NORM_FFT

		// Initialise foam texture
		m_foamTexture.init_from_image(systems.pD3DDevice, "Assets/Textures/Ocean_Foam.png", false, true);

		// Initialise skymap texture
		m_skyMapTexture.init_from_dds(systems.pD3DDevice, "Assets/Textures/skymap.dds");


		//--------------------- Initialisation of Philips Spectrum, h0 and h0conjugate. ---------------------//
		m_wrapper.Generate_Heightmap();


		//--------------------- Compute Shader (Ocean) Buffers Initialisation ---------------------//
		uint32_t buffSize = m_wrapper.getHeight() * m_wrapper.getWidth();

		// Stractured buffers to pass their views into the CS, and a reader buffer to get the results.
		CreateStructuredBuffer(systems.pD3DDevice, sizeof(float), buffSize, m_wrapper.getKMag(), &g_pBufKMag);
		CreateStructuredBuffer(systems.pD3DDevice, sizeof(Vec2), buffSize, m_wrapper.getH0Tilde(), &g_pBufH0t);
		CreateStructuredBuffer(systems.pD3DDevice, sizeof(Vec2), buffSize, m_wrapper.getH0TildeConj(), &g_pBufH0tc);
		CreateStructuredBuffer(systems.pD3DDevice, sizeof(Vec2), buffSize, nullptr, &g_pBufHtilde);
		CreateReaderBuffer(systems.pD3DDevice, systems.pD3DContext, g_pBufHtilde, &g_pBufReader);

		// 3 Shader Resource Views for input and 1 Unordered Access View for output.
		CreateBufferSRV(systems.pD3DDevice, g_pBufKMag, &g_pBufKMagSRV);
		CreateBufferSRV(systems.pD3DDevice, g_pBufH0t, &g_pBufH0tSRV);
		CreateBufferSRV(systems.pD3DDevice, g_pBufH0tc, &g_pBufH0tcSRV);

		CreateBufferUAV(systems.pD3DDevice, g_pBufHtilde, &g_pBufHtildeUAV);

				
		// Create various state registers.
		{
			// Backface culling.
			D3D11_RASTERIZER_DESC desc = {};
			desc.FillMode = D3D11_FILL_SOLID;
			desc.CullMode = D3D11_CULL_BACK;
			desc.DepthClipEnable = TRUE;
			systems.pD3DDevice->CreateRasterizerState(&desc, &m_pRasterizerStates[RasterizerStates::kBackFaceCull]);

			// Double sided
			desc.CullMode = D3D11_CULL_NONE;
			systems.pD3DDevice->CreateRasterizerState(&desc, &m_pRasterizerStates[RasterizerStates::kDoubleSided]);
		}

		{
			// Wireframe.
			D3D11_RASTERIZER_DESC desc = {};
			desc.FillMode = D3D11_FILL_WIREFRAME;
			desc.CullMode = D3D11_CULL_NONE;
			desc.DepthClipEnable = TRUE;
			systems.pD3DDevice->CreateRasterizerState(&desc, &m_pRasterizerStates[RasterizerStates::kWireframe]);
		}

		{
			// Z Write Enable
			D3D11_DEPTH_STENCIL_DESC desc = {};
			desc.DepthEnable = TRUE;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			desc.DepthFunc = D3D11_COMPARISON_LESS;
			desc.StencilEnable = FALSE;
			systems.pD3DDevice->CreateDepthStencilState(&desc, &m_pDepthStencilStates[DepthStencilStates::kZWriteEnabled]);

			// Z Write Disable.
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			systems.pD3DDevice->CreateDepthStencilState(&desc, &m_pDepthStencilStates[DepthStencilStates::kZWriteDisabled]);
		
			// Skybox Stencil Buffer
			ZeroMemory(&desc, sizeof(D3D11_DEPTH_STENCIL_DESC));
			desc.DepthEnable = TRUE;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = D3D11_COMPARISON_LESS;

			systems.pD3DDevice->CreateDepthStencilState(&desc, &m_pDepthStencilStates[DepthStencilStates::kSkymapStencil]);
		}

		{
			// Additive
			D3D11_BLEND_DESC desc = {};
			desc.AlphaToCoverageEnable = FALSE;
			desc.IndependentBlendEnable = FALSE;
			desc.RenderTarget[0].BlendEnable = TRUE;
			desc.RenderTarget[0].SrcBlend = desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].DestBlend = desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
			desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			systems.pD3DDevice->CreateBlendState(&desc, &m_pBlendStates[BlendStates::kAdditive]);

			// Transparent
			desc.RenderTarget[0].SrcBlend = desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlend = desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
			systems.pD3DDevice->CreateBlendState(&desc, &m_pBlendStates[BlendStates::kTransparent]);

			// Opaque
			desc.RenderTarget[0].BlendEnable = FALSE;
			systems.pD3DDevice->CreateBlendState(&desc, &m_pBlendStates[BlendStates::kOpaque]);
		}

	}


	void on_update(SystemsInterface& systems) override
	{
		// This function displays some useful debugging values, camera positions etc.
		DemoFeatures::editorHud(systems.pDebugDrawContext);
		
		//---------------------------------------------------------------------------
		// Imgui sliders are provided for artistic intervention and customisation
		//---------------------------------------------------------------------------

		// Imgui window to act as a control interface
		ImGui::Begin("Control Interface");
		ImGui::Columns(1);
		ImGui::SliderFloat("Height Adjustment", &m_heightAdj, 0.0f, 5.0f);
		ImGui::SliderFloat("Lambda (Choppy Look)", &m_lambda, 0.0f, 5.0f);
		ImGui::SliderFloat("Foam Intensity", &m_foamInt, 0.0f, 3.0f);
		ImGui::SliderFloat("Reflectivity", &m_reflectFrag, 0.0f, 1.0f);
		ImGui::Separator();

#ifdef GPGPU_NORM_CD
		ImGui::SliderFloat("Timescale", &m_timescale, 0.0f, 0.1f);
#endif //GPGPU_NORM_CD

		ImGui::Columns(3);
		ImGui::Checkbox("Wireframe", &m_onlyWireframe);
		ImGui::NextColumn();
		ImGui::Checkbox("Skymap", &m_drawSkymap);
		ImGui::NextColumn();
		ImGui::Checkbox("Show Heightmaps", &m_showHeightmaps);
		ImGui::NextColumn();
		ImGui::Checkbox("Pause", &m_pause);
		ImGui::Columns(1);
		ImGui::End();
		
		// Note: Every system update should happen after the ImGui updates
		// since changes in sliders break the application otherwise

		// Update Normal Calc CS Data.
		m_normCsCBData.m_choppy = m_lambda;
		m_normCsCBData.m_heightAdjust = m_heightAdj;
		m_normCsCBData.m_foamIntensity = m_foamInt;

		// Update Per Frame Data.
		m_perFrameCBData.m_matProjection = systems.pCamera->projMatrix.Transpose();
		m_perFrameCBData.m_matView = systems.pCamera->viewMatrix.Transpose();
		m_perFrameCBData.m_vViewPos = v4(systems.pCamera->eye.x, systems.pCamera->eye.y, systems.pCamera->eye.z, 1.0f);
		m_perFrameCBData.m_choppy = m_lambda;
		m_perFrameCBData.m_heightAdjust = m_heightAdj;
		m_perFrameCBData.m_reflectivity = m_reflectFrag;

		// Pause the tile movement
		if (m_pause)
			return;
		
//--------------------------------- GPGPU Execution ---------------------------------//
#ifdef GPGPU_NORM_CD

		//---------------------------------------------------------------------------
		// This order of functions excecution was chosen to properly synchronise
		// the CPU with the GPU. This way we can fully load the command buffer before
		// Present is called. After that we give the GPU plenty of time to execute 
		// all of its commands before the CPU needs these resources again.
		// The cost of this optimisation is the first two frames.
		//---------------------------------------------------------------------------
		m_oceanCsCBData.m_time += m_timescale;			// Increase time
		m_wrapper.Fill_Horizontal_Displacement();		// Fill the input for the horizontal displacement
		m_wrapper.IFFT_Thread();						// Run the IFFTs in parallel
		m_wrapper.Fill_Texture();						// Fill the heightmap texture

		// Update Heightmap texture to use in the GPU
		{
			ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));
			//  Disable GPU access to the texture data.
			systems.pD3DContext->Map(m_heightmapTexture.getTexture(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
			//  Update the texture here.
			memcpy(mappedResource.pData, m_wrapper.getImageOut(), (m_wrapper.getHeight() * m_wrapper.getWidth() * 4 * 4));
			//  Re-enable GPU access to the texture data.
			systems.pD3DContext->Unmap(m_heightmapTexture.getTexture(), 0);
		}

		compute_normals_cs(systems);							// Dispatch Normals calculation compute shader
		read_htilde(systems);									// Map GPU resource and copy to CPU input
		compute_htilde_cs(systems);								// Dispatch Htilde calculation compute shader
#endif //GPGPU_NORM_CD

//--------------------------------- CPU only Executions ---------------------------------//
#if defined(CPU_NORM_CD) | defined(CPU_NORM_FFT)
		
		m_wrapper.Fill_htilde_and_Displacements();				// Fill the input for the IFFT
		m_wrapper.IFFT_Thread();								// Run the IFFTs in parallel
		m_wrapper.Fill_Texture();								// Fill the heightmap texture

#if defined(CPU_NORM_FFT)
		m_wrapper.Fill_Normals_FFT(m_lambda, m_foamInt);						// Fill Normal map using FFT
#elif defined(CPU_NORM_CD)
		m_wrapper.Fill_Normals_Central_Diff(m_lambda, m_heightAdj, m_foamInt);	// Fill Normal map using Central Difference
#endif 

		// Update Heightmap texture
		{
			ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));
			//  Disable GPU access to the texture data.
			systems.pD3DContext->Map(m_heightmapTexture.getTexture(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
			//  Update the texture here.
			memcpy(mappedResource.pData, m_wrapper.getImageOut(), (m_wrapper.getHeight() * m_wrapper.getWidth() * 4 * 4));
			//  Re-enable GPU access to the texture data.
			systems.pD3DContext->Unmap(m_heightmapTexture.getTexture(), 0);
		}
		//Update Normalmap texture
		{
			ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));
			//  Disable GPU access to the texture data.
			systems.pD3DContext->Map(m_normalmapTexture.getTexture(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
			//  Update the texture here.
			memcpy(mappedResource.pData, m_wrapper.getNormalOut(), (m_wrapper.getHeight() * m_wrapper.getWidth() * 4 * 4));
			//  Re-enable GPU access to the texture data.
			systems.pD3DContext->Unmap(m_normalmapTexture.getTexture(), 0);
		}
#endif // CPU_NORM_CD | CPU_NORM_FFT
	}

	void on_render(SystemsInterface& systems) override
	{
		// Push Per Frame Data to GPU
		push_constant_buffer(systems.pD3DContext, m_pPerFrameCB, m_perFrameCBData);
		push_constant_buffer(systems.pD3DContext, m_pOceanCsCB, m_oceanCsCBData);
		push_constant_buffer(systems.pD3DContext, m_pNormCsCB, m_normCsCBData);

		// Draw calls for the scene components
		if (m_drawSkymap)
			draw_skymap(systems);

		draw_ocean(systems, 0, 0, 0);
		
		if (m_showHeightmaps)
			draw_myUI(systems);
	}

	void compute_htilde_cs(SystemsInterface &systems)
	{
		// Bind our set of shaders.
		m_oceanShader.bind(systems.pD3DContext);

		// Initialise the buffers
		ID3D11ShaderResourceView* SRViews[] = { g_pBufKMagSRV, g_pBufH0tSRV, g_pBufH0tcSRV };
		ID3D11UnorderedAccessView* UAViews[] = { g_pBufHtildeUAV };
		ID3D11Buffer* buffers[] = { m_pOceanCsCB };
		const UINT initCount = -1;

		// Bind all buffers (SRV, UAV, CB)
		systems.pD3DContext->CSSetShaderResources(4, 3, SRViews);
		systems.pD3DContext->CSSetUnorderedAccessViews(0, 1, UAViews, &initCount);
		systems.pD3DContext->CSSetConstantBuffers(2, 1, buffers);

		// Dispatch Compute Shader
		systems.pD3DContext->Dispatch(SIZE_OF_GRID / 4, SIZE_OF_GRID / 4, 1);

		// Unbind all buffers to clean up
		ID3D11ShaderResourceView* nullSRV[] = { NULL, NULL, NULL };
		systems.pD3DContext->CSSetShaderResources(4, 3, nullSRV);
		ID3D11UnorderedAccessView* nullUAV[] = { NULL };
		systems.pD3DContext->CSSetUnorderedAccessViews(0, 1, nullUAV, 0);
		ID3D11Buffer* nullBuffers[] = { NULL };
		systems.pD3DContext->CSSetConstantBuffers(2, 1, nullBuffers);

		// Disable Compute Shader
		systems.pD3DContext->CSSetShader(nullptr, nullptr, 0);

		// Prepare resource for reading on the next frame
		systems.pD3DContext->CopyResource(g_pBufReader, g_pBufHtilde);
	}

	void read_htilde(SystemsInterface &systems)
	{
		// Read the shader output by mapping
		Vec2 *p;
		systems.pD3DContext->Map(g_pBufReader, 0, D3D11_MAP_READ, 0, &mappedResource);
		p = (Vec2*)mappedResource.pData;
		
		// Copy result to FFT input
		uint32_t arraySize = SIZE_OF_GRID * SIZE_OF_GRID;
		fftwf_complex * temp_pFFTin = m_wrapper.getFFTin(0);
		for (uint32_t n = 0; n < arraySize; ++n)
		{
			temp_pFFTin[n][0] = p[n].x;
			temp_pFFTin[n][1] = p[n].y;
		}
		systems.pD3DContext->Unmap(g_pBufReader, 0);
	}

	void compute_normals_cs(SystemsInterface &systems)
	{
		// Bind our set of shaders.
		m_normalsCShader.bind(systems.pD3DContext);

		// Buffers and Views preparation
		ID3D11ShaderResourceView* SRViews[] = { m_heightmapTexture.getSRV() };
		ID3D11UnorderedAccessView* UAViews[] = { m_normalmapTexture.getUAV() };
		ID3D11Buffer* buffers[] = { m_pNormCsCB };
		const UINT initCount = -1;

		// Bind all buffers (SRV, UAV, CB)
		systems.pD3DContext->CSSetShaderResources(0, 1, SRViews);
		systems.pD3DContext->CSSetUnorderedAccessViews(0, 1, UAViews, &initCount);
		systems.pD3DContext->CSSetConstantBuffers(0, 1, buffers);

		// Dispatch Compute Shader
		systems.pD3DContext->Dispatch(SIZE_OF_GRID / 4, SIZE_OF_GRID / 4, 1);

		// Unbind all buffers to clean up
		ID3D11ShaderResourceView* nullSRV[] = { NULL };
		systems.pD3DContext->CSSetShaderResources(0, 1, nullSRV);
		ID3D11UnorderedAccessView* nullUAV[] = { NULL };
		systems.pD3DContext->CSSetUnorderedAccessViews(0, 1, nullUAV, 0);
		ID3D11Buffer* nullBuffers[] = { NULL };
		systems.pD3DContext->CSSetConstantBuffers(0, 1, buffers);

		// Disable Compute Shader
		systems.pD3DContext->CSSetShader(nullptr, nullptr, 0);
	}

	void draw_ocean(SystemsInterface &systems, float x, float y, float z)
	{
		// Bind our set of shaders.
		m_oceanShader.bind(systems.pD3DContext);

		// Bind Constant Buffers, to both PS and VS stages
		ID3D11Buffer* buffers[] = { m_pPerFrameCB, m_pPerDrawCB };
		systems.pD3DContext->VSSetConstantBuffers(0, 2, buffers);
		systems.pD3DContext->PSSetConstantBuffers(0, 2, buffers);

		// Bind a sampler state
		ID3D11SamplerState* samplers[] = { m_pSamplerState };
		systems.pD3DContext->PSSetSamplers(0, 1, samplers);

		// Bind SRVs to the appropriate shaders
		ID3D11ShaderResourceView* SRViews[] = { m_heightmapTexture.getSRV(), m_normalmapTexture.getSRV(), m_foamTexture.getSRV() };
		systems.pD3DContext->VSSetShaderResources(0, 3, SRViews);
		systems.pD3DContext->PSSetShaderResources(0, 3, SRViews);

		// Bind the sky texture for reflections and refractions
		m_skyMapTexture.bind(systems.pD3DContext, ShaderStage::kPixel, 3);

		// Set blend depth stencil states
		// Option for Wireframe mode.
		if(m_onlyWireframe)
			systems.pD3DContext->RSSetState(m_pRasterizerStates[RasterizerStates::kWireframe]);
		else
			systems.pD3DContext->RSSetState(m_pRasterizerStates[RasterizerStates::kBackFaceCull]);

		systems.pD3DContext->OMSetDepthStencilState(m_pDepthStencilStates[DepthStencilStates::kZWriteEnabled], 0);
		systems.pD3DContext->OMSetBlendState(m_pBlendStates[BlendStates::kOpaque], kBlendFactor, kSampleMask);

		// Bind the mesh
		m_oceanMesh.bind(systems.pD3DContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Compute MVP matrix.
		m4x4 matModel = m4x4::CreateTranslation(x, y, z);
		m4x4 matMVP = matModel * systems.pCamera->vpMatrix;

		// Update Per Draw Data
		m_perDrawCBData.m_matMVP = matMVP.Transpose();
		m_perDrawCBData.m_matModel = matModel.Transpose();

		// Push CB to GPU
		push_constant_buffer(systems.pD3DContext, m_pPerDrawCB, m_perDrawCBData);

		// Draw the mesh.
		m_oceanMesh.draw(systems.pD3DContext);
	}

	void draw_myUI(SystemsInterface &systems)
	{
		// Bind our set of shaders.
		m_UIshader.bind(systems.pD3DContext);

		// Bind a sampler state
		ID3D11SamplerState* samplers[] = { m_pSamplerState };
		systems.pD3DContext->PSSetSamplers(0, 1, samplers);

		// Bind the heightmap and normalmap textures to the fragment shader
		m_heightmapTexture.bind(systems.pD3DContext, ShaderStage::kPixel, 0);
		m_normalmapTexture.bind(systems.pD3DContext, ShaderStage::kPixel, 1);

		// Set blend depth stencil states
		systems.pD3DContext->RSSetState(m_pRasterizerStates[RasterizerStates::kBackFaceCull]);
		systems.pD3DContext->OMSetDepthStencilState(m_pDepthStencilStates[DepthStencilStates::kZWriteDisabled], 0);
		systems.pD3DContext->OMSetBlendState(m_pBlendStates[BlendStates::kTransparent], kBlendFactor, kSampleMask);

		// Bind the mesh
		m_UIMesh.bind(systems.pD3DContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Draw the mesh.
		m_UIMesh.draw(systems.pD3DContext);
	}

	void draw_skymap(SystemsInterface &systems)
	{
		// Bind our set of shaders.
		m_skyMapShader.bind(systems.pD3DContext);

		// Bind Constant Buffers, to both PS and VS stages
		ID3D11Buffer* buffers[] = { m_pPerFrameCB, m_pPerDrawCB};
		systems.pD3DContext->VSSetConstantBuffers(0, 2, buffers);
		systems.pD3DContext->PSSetConstantBuffers(0, 2, buffers);

		// Bind a sampler state
		ID3D11SamplerState* samplers[] = { m_pSamplerState };
		systems.pD3DContext->PSSetSamplers(0, 1, samplers);

		// Bind the heightmap texture to both vertex and fragment shaders
		m_skyMapTexture.bind(systems.pD3DContext, ShaderStage::kPixel, 0);

		// Set blend depth stencil states
		systems.pD3DContext->RSSetState(m_pRasterizerStates[RasterizerStates::kDoubleSided]);
		systems.pD3DContext->OMSetDepthStencilState(m_pDepthStencilStates[DepthStencilStates::kSkymapStencil], 0);
		systems.pD3DContext->OMSetBlendState(m_pBlendStates[BlendStates::kOpaque], kBlendFactor, kSampleMask);

		// Bind the mesh
		m_skyMesh.bind(systems.pD3DContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Compute MVP matrix.
		m4x4 matModel = m4x4::CreateScale(5.0f, 5.0f, 5.0f);
		matModel *= m4x4::CreateTranslation(systems.pCamera->eye);
		m4x4 matMVP = matModel * systems.pCamera->vpMatrix;

		// Update Per Draw Data
		m_perDrawCBData.m_matMVP = matMVP.Transpose();
		m_perDrawCBData.m_matModel = matModel.Transpose();

		// Push CB to GPU
		push_constant_buffer(systems.pD3DContext, m_pPerDrawCB, m_perDrawCBData);

		// Draw the mesh.
		m_skyMesh.draw(systems.pD3DContext);
	}



	void on_resize(SystemsInterface& ) override
	{
		
	}

private:

	// Constant Buffers
	PerFrameCBData m_perFrameCBData;
	ID3D11Buffer* m_pPerFrameCB = nullptr;

	PerDrawCBData m_perDrawCBData;
	ID3D11Buffer* m_pPerDrawCB = nullptr;

	OceanCsCBData m_oceanCsCBData;
	ID3D11Buffer* m_pOceanCsCB = nullptr;

	NormCsCBData m_normCsCBData;
	ID3D11Buffer* m_pNormCsCB = nullptr;

	// Shader Sets
	ShaderSet m_skyMapShader;
	ShaderSet m_oceanShader;
	ShaderSet m_UIshader;
	ShaderSet m_normalsCShader;
	
	// Meshes
	Mesh m_skyMesh;
	Mesh m_oceanMesh;
	Mesh m_UIMesh;

	// Textures
	Texture m_skyMapTexture;
	Texture m_heightmapTexture;
	Texture m_normalmapTexture;
	Texture m_foamTexture;

	// Singletons
	FFTWrapper & m_wrapper = FFTWrapper::getInstance(SIZE_OF_GRID);
	OceanTile &Tile = OceanTile::getInstance();
	
	// Sampler State
	ID3D11SamplerState* m_pSamplerState = nullptr;

	// Mapped Subresource to use for Mapping/Unmapping
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	// Structured Buffers and their views (SRV and UAV)
	ID3D11Buffer*               g_pBufKMag = nullptr;
	ID3D11Buffer*               g_pBufH0t = nullptr;
	ID3D11Buffer*               g_pBufH0tc = nullptr;
	ID3D11Buffer*               g_pBufHtilde = nullptr;
	ID3D11Buffer*               g_pBufReader = nullptr;

	ID3D11ShaderResourceView*   g_pBufKMagSRV = nullptr;
	ID3D11ShaderResourceView*   g_pBufH0tSRV = nullptr;
	ID3D11ShaderResourceView*   g_pBufH0tcSRV = nullptr;
	ID3D11UnorderedAccessView*	g_pBufHtildeUAV = nullptr;

	//Custom vertex type
	struct Vertex
	{
		DirectX::XMFLOAT4A pos;   // 3D position
		DirectX::XMFLOAT4A uv;    // Texture coordinates
		DirectX::XMFLOAT4A color; // RGBA float
	};

	enum RasterizerStates
	{
		kBackFaceCull,
		kDoubleSided,
		kWireframe,
		kMaxRasterizerStates
	};
	ID3D11RasterizerState* m_pRasterizerStates[kMaxRasterizerStates];

	enum DepthStencilStates
	{
		kZWriteEnabled,
		kZWriteDisabled,
		kSkymapStencil,
		kMaxDepthStencilStates
	};
	ID3D11DepthStencilState* m_pDepthStencilStates[kMaxDepthStencilStates];

	enum BlendStates
	{
		kOpaque,
		kTransparent,
		kAdditive,
		kMaxBlendStates
	};
	ID3D11BlendState* m_pBlendStates[kMaxBlendStates];

	// Model variables
	float m_lambda = 1.3f;
	float m_foamInt = 2.0f;
	float m_timescale = 0.04f;
	float m_heightAdj = 1.2f;
	float m_reflectFrag = 0.6f;

	// Imgui Checkboxes
	bool m_onlyWireframe = false;
	bool m_showHeightmaps = false;
	bool m_drawSkymap = false;
	bool m_pause = false;

};

OceanApp g_app;

FRAMEWORK_IMPLEMENT_MAIN(g_app, "Ocean Tile")
