#pragma once

#include "CommonHeader.h"
#include "VertexFormats.h"


using MeshVertex = Vertex_Pos3fColour4ubNormal3fTex2f; // vertex type

//================================================================================
// Mesh Class
// Wraps an index and vertex buffer.
// Provides methods for loading a simple model.
//================================================================================
class Mesh
{
public:
	Mesh();
	~Mesh();

	void init_buffers(ID3D11Device* pDevice, const MeshVertex* pVertices, const u32 kNumVerts, const u16* pIndices, const u32 kNumIndices);
	void bind(ID3D11DeviceContext* pContext, const D3D_PRIMITIVE_TOPOLOGY topology);
	void draw(ID3D11DeviceContext* pContext);

	// Accessors.
	const ID3D11Buffer* vertex_buffer() const { return m_pVertexBuffer; }
	const ID3D11Buffer* index_buffer() const { return m_pIndexBuffer; }

	u32 vertices() const { return m_vertices; }
	u32 indices() const { return m_indices; }

private:
	ID3D11Buffer* m_pVertexBuffer;
	ID3D11Buffer* m_pIndexBuffer;
	u32 m_vertices;
	u32 m_indices;
};

//================================================================================
// Helpers for creating mesh data
//================================================================================

void create_mesh_sphere(ID3D11Device* pDevice, Mesh& rMeshOut, const int& LatLines, const int& LongLines);

void create_mesh_cube(ID3D11Device* pDevice, Mesh& rMeshOut, const f32 kHalfSize);

void create_mesh_quad_xy(ID3D11Device* pDevice, Mesh& rMeshOut, const f32 kHalfSize);

void create_mesh_from_obj(ID3D11Device* pDevice, Mesh& rMeshOut, const char* pFilename, const f32 kScale);

void create_point(ID3D11Device* pDevice, Mesh& rMeshOut);
