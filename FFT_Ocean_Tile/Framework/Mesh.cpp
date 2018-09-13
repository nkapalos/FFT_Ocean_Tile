
#include "Mesh.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobjloader/tiny_obj_loader.h"

Mesh::Mesh()
	: m_pVertexBuffer(nullptr)
	, m_pIndexBuffer(nullptr)
{

}

Mesh::~Mesh()
{
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pIndexBuffer);
}

void Mesh::init_buffers(ID3D11Device* pDevice, const MeshVertex* pVertices, const u32 kNumVerts, const u16* pIndices, const u32 kNumIndices)
{
	ASSERT(!m_pVertexBuffer && !m_pIndexBuffer);

	m_pVertexBuffer = nullptr;
	// Create a vertex buffer
	{
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = sizeof(MeshVertex) * kNumVerts;
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA data = {};
		data.pSysMem = pVertices;

		HRESULT hr = pDevice->CreateBuffer(&desc, &data, &m_pVertexBuffer);
		ASSERT(!FAILED(hr));
	}

	// Create an index buffer
	m_pIndexBuffer = nullptr;
	if (pIndices)
	{
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = sizeof(u16) * kNumIndices;
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

		D3D11_SUBRESOURCE_DATA data = {};
		data.pSysMem = pIndices;

		HRESULT hr = pDevice->CreateBuffer(&desc, &data, &m_pIndexBuffer);
		ASSERT(!FAILED(hr));
	}

	m_vertices = kNumVerts;
	m_indices = kNumIndices;
}

void Mesh::bind(ID3D11DeviceContext* pContext, const D3D_PRIMITIVE_TOPOLOGY topology)
{
	pContext->IASetPrimitiveTopology(topology);

	ID3D11Buffer* buffers[] = { m_pVertexBuffer };
	UINT strides[] = { sizeof(MeshVertex) };
	UINT offsets[] = { 0 };
	pContext->IASetVertexBuffers(0, 1, buffers, strides, offsets);

	if (m_pIndexBuffer)
	{
		pContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	}
}

void Mesh::draw(ID3D11DeviceContext* pContext)
{
	if (m_pIndexBuffer)
	{
		pContext->DrawIndexed(m_indices, 0, 0);
	}
	else
	{
		pContext->Draw(m_vertices, 0);
	}
}

void create_mesh_sphere(ID3D11Device* pDevice, Mesh& rMeshOut, const int& LatLines, const int& LongLines)
{
	int NumSphereVertices = ((LatLines - 2) * LongLines) + 2;
	int NumSphereFaces = ((LatLines - 3)*(LongLines) * 2) + (LongLines * 2);

	float sphereYaw = 0.0f;
	float spherePitch = 0.0f;

	std::vector<v3> vertices(NumSphereVertices);
	std::vector<u16> indices(NumSphereFaces * 3);

	DirectX::XMMATRIX Rotationx;
	DirectX::XMMATRIX Rotationy;

	v4 currVertPos = v4(0.0f, 0.0f, 1.0f, 0.0f);

	vertices[0] = v3( 0.0f, 0.0f, 1.0f);

	for (DWORD i = 0; i < LatLines - 2; ++i)
	{
		spherePitch = (i + 1) * (3.14 / (LatLines - 1));
		Rotationx = DirectX::XMMatrixRotationX(spherePitch);
		for (DWORD j = 0; j < LongLines; ++j)
		{
			sphereYaw = j * (6.28 / (LongLines));
			Rotationy = DirectX::XMMatrixRotationZ(sphereYaw);
			currVertPos = DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), (Rotationx * Rotationy));
			currVertPos = DirectX::XMVector3Normalize(currVertPos);
			vertices[i*LongLines + j + 1] = v3(currVertPos.x , currVertPos.y, currVertPos.z);
		}
	}

	vertices[NumSphereVertices - 1] = v3(0.0f, 0.0f, -1.0f);

	int k = 0;
	for (u16 l = 0; l < LongLines - 1; ++l)
	{
		indices[k] = 0;
		indices[k + 1] = l + 1;
		indices[k + 2] = l + 2;
		k += 3;
	}

	indices[k] = 0;
	indices[k + 1] = LongLines;
	indices[k + 2] = 1;
	k += 3;

	for (u16 i = 0; i < LatLines - 3; ++i)
	{
		for (u16 j = 0; j < LongLines - 1; ++j)
		{
			indices[k] = i*LongLines + j + 1;
			indices[k + 1] = i*LongLines + j + 2;
			indices[k + 2] = (i + 1)*LongLines + j + 1;

			indices[k + 3] = (i + 1)*LongLines + j + 1;
			indices[k + 4] = i*LongLines + j + 2;
			indices[k + 5] = (i + 1)*LongLines + j + 2;

			k += 6; // next quad
		}

		indices[k] = (i*LongLines) + LongLines;
		indices[k + 1] = (i*LongLines) + 1;
		indices[k + 2] = ((i + 1)*LongLines) + LongLines;

		indices[k + 3] = ((i + 1)*LongLines) + LongLines;
		indices[k + 4] = (i*LongLines) + 1;
		indices[k + 5] = ((i + 1)*LongLines) + 1;

		k += 6;
	}

	for (u16 l = 0; l < LongLines - 1; ++l)
	{
		indices[k] = NumSphereVertices - 1;
		indices[k + 1] = (NumSphereVertices - 1) - (l + 1);
		indices[k + 2] = (NumSphereVertices - 1) - (l + 2);
		k += 3;
	}

	indices[k] = NumSphereVertices - 1;
	indices[k + 1] = (NumSphereVertices - 1) - LongLines;
	indices[k + 2] = NumSphereVertices - 2;


	std::vector<MeshVertex> verts;

	for (int i = 0; i < vertices.size(); ++i)
	{
		verts.push_back(MeshVertex(vertices[i], 0xFFFFFFFF, v3(0,1,0), v2(0, 0)));
	}
	rMeshOut.init_buffers(pDevice, &verts[0], verts.size(), &indices[0], indices.size());

}

void create_mesh_cube(ID3D11Device* pDevice, Mesh& rMeshOut, const f32 kHalfSize)
{
	// define the vertices
	const f32 s = kHalfSize;
	const u32 c = 0xFF808080;

	const u32 colours[6] = {
		0xFF800000,	  // front
		0xFF008000,	  // right
		0xFF000080,	  // back
		0xFF808000,	  // left
		0xFF800080,	  // top
		0xFF008080	  // bottom
	};

	const v3 normals[6] =
	{
		v3(0, 0, 1),	// front
		v3(1, 0, 0),	// right
		v3(0, 0, -1),	// back
		v3(-1, 0, 0),	// left
		v3(0, 1, 0),	// top
		v3(0, -1, 0)	// bottom
	};

	const v2 texCoords[4] = {
		v2(0, 0),
		v2(1, 0),
		v2(1, 1),
		v2(0, 1)
	};

	const MeshVertex verts[] = {
		//front
		MeshVertex(v3(-s, -s, s), colours[0], normals[0], texCoords[0]),
		MeshVertex(v3(s, -s, s), colours[0], normals[0], texCoords[1]),
		MeshVertex(v3(s, s, s), colours[0], normals[0], texCoords[2]),
		MeshVertex(v3(-s, s, s), colours[0], normals[0], texCoords[3]),

		//right
		MeshVertex(v3(s, s, s), colours[1], normals[1], texCoords[0]),
		MeshVertex(v3(s, s, -s), colours[1], normals[1], texCoords[1]),
		MeshVertex(v3(s, -s, -s), colours[1], normals[1], texCoords[2]),
		MeshVertex(v3(s, -s, s), colours[1], normals[1], texCoords[3]),

		//back
		MeshVertex(v3(-s, -s, -s), colours[2], normals[2], texCoords[0]),
		MeshVertex(v3(s, -s, -s), colours[2], normals[2], texCoords[1]),
		MeshVertex(v3(s, s, -s), colours[2], normals[2], texCoords[2]),
		MeshVertex(v3(-s, s, -s), colours[2], normals[2], texCoords[3]),

		//left
		MeshVertex(v3(-s, -s, -s), colours[3], normals[3], texCoords[0]),
		MeshVertex(v3(-s, -s, s), colours[3], normals[3], texCoords[1]),
		MeshVertex(v3(-s, s, s), colours[3], normals[3], texCoords[2]),
		MeshVertex(v3(-s, s, -s), colours[3], normals[3], texCoords[3]),

		//top
		MeshVertex(v3(s, s, s), colours[4], normals[4], texCoords[0]),
		MeshVertex(v3(-s, s, s), colours[4], normals[4], texCoords[1]),
		MeshVertex(v3(-s, s, -s), colours[4], normals[4], texCoords[2]),
		MeshVertex(v3(s, s, -s), colours[4], normals[4], texCoords[3]),

		//bottom
		MeshVertex(v3(-s, -s, -s), colours[5], normals[5], texCoords[0]),
		MeshVertex(v3(s, -s, -s), colours[5], normals[5], texCoords[1]),
		MeshVertex(v3(s, -s, s), colours[5], normals[5], texCoords[2]),
		MeshVertex(v3(-s, -s, s), colours[5], normals[5], texCoords[3]),

	};

	const u32 kVertices = sizeof(verts) / sizeof(verts[0]);

	// and indices
	const u16 indices[] = {
		0,  1,  2,  0,  2,  3,   // front
		4,  5,  6,  4,  6,  7,   // right
		8,  9,  10, 8,  10, 11,  // back
		12, 13, 14, 12, 14, 15,  // left
		16, 17, 18, 16, 18, 19,  // top
		20, 21, 22, 20, 22, 23   // bottom
	};

	const u32 kIndices = sizeof(indices) / sizeof(indices[0]);

	rMeshOut.init_buffers(pDevice, verts, kVertices, indices, kIndices);
}

void create_mesh_quad_xy(ID3D11Device* pDevice, Mesh& rMeshOut, const f32 kHalfSize)
{
	// define the vertices
	const f32 s = kHalfSize;
	const u32 c = 0xFF808080;

	const u32 colours[] = {
		0xFFFFFFFF	  // front
	};

	const v3 normals[] =
	{
		v3(0, 0, -1),
		v3(0, 0, 1)
	};

	const v2 texCoords[] = {
		v2(0, 0),
		v2(1, 0),
		v2(1, 1),
		v2(0, 1)
	};

	const MeshVertex verts[] = {
		// front
		MeshVertex(v3(-s, -s, 0.f), colours[0], normals[0], texCoords[0]),
		MeshVertex(v3(s, -s, 0.f), colours[0], normals[0], texCoords[1]),
		MeshVertex(v3(s, s, 0.f), colours[0], normals[0], texCoords[2]),
		MeshVertex(v3(-s, s, 0.f), colours[0], normals[0], texCoords[3]),

		// back
		MeshVertex(v3(-s, -s, 0.f), colours[0], normals[1], texCoords[0]),
		MeshVertex(v3(s, -s, 0.f), colours[0], normals[1], texCoords[1]),
		MeshVertex(v3(s, s, 0.f), colours[0], normals[1], texCoords[2]),
		MeshVertex(v3(-s, s, 0.f), colours[0], normals[1], texCoords[3]),
	};

	const u32 kVertices = sizeof(verts) / sizeof(verts[0]);

	// and indices
	const u16 indices[] = {
		0,  1,  2,  0,  2,  3,
		4,  6,  5,  4,  7,  6
	};

	const u32 kIndices = sizeof(indices) / sizeof(indices[0]);

	rMeshOut.init_buffers(pDevice, verts, kVertices, indices, kIndices);
}

void create_point(ID3D11Device* pDevice, Mesh& rMeshOut)
{
	const u32 colours[] = {
		0xFFFFFFFF	  // front
	};

	const v3 normals[] =
	{
		v3(0, 0, -1)
	};

	const v2 texCoords[] = {
		v2(0, 0)
	};

	const MeshVertex verts[] = {
		MeshVertex(v3(0.f, 0.f, 0.f), colours[0], normals[0], texCoords[0])
	};

	const u32 kVertices = sizeof(verts) / sizeof(verts[0]);

	const u32 kIndices = 0;

	rMeshOut.init_buffers(pDevice, verts, kVertices, nullptr, kIndices);

}


void create_mesh_from_obj(ID3D11Device* pDevice, Mesh& rMeshOut, const char* pFilename, const f32 kScale)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::vector<MeshVertex> meshVertices;	

	std::string err;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, pFilename);

	if (!err.empty()) { // `err` may contain warning message.
		debugF("load_obj_mesh( %s ) : %s", pFilename, err.c_str());
	}

	if (!ret) {
		panicF("Error Loading OBJ %s", pFilename);
	}

	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {

		meshVertices.clear();

		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) 
		{
			u32 fv = shapes[s].mesh.num_face_vertices[f];

			// Loop over vertices in the face.
			for (u32 v = 0; v < fv; v++) {

				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
				tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
				tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
				tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
				tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
				tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
				tinyobj::real_t tx = attrib.texcoords[2 * idx.texcoord_index + 0];
				tinyobj::real_t ty = attrib.texcoords[2 * idx.texcoord_index + 1];

				// Optional: vertex colors
				// tinyobj::real_t red = attrib.colors[3*idx.vertex_index+0];
				// tinyobj::real_t green = attrib.colors[3*idx.vertex_index+1];
				// tinyobj::real_t blue = attrib.colors[3*idx.vertex_index+2];

				v3 pos = v3(vx, vy, vz) * kScale;
				v3 normal(nx, ny, nz);
				normal.Normalize();
				v2 uv(tx,ty);

				meshVertices.push_back(MeshVertex(pos, 0xFFFFFFFF, normal, uv));
			}
			index_offset += fv;

			// per-face material
			shapes[s].mesh.material_ids[f];
		}
	}
	rMeshOut.init_buffers(pDevice, &meshVertices[0], meshVertices.size(), nullptr, 0);
}
