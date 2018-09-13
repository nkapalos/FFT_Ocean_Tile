#include "OceanTile.h"
#include <math.h>

#define PI 3.141592

void OceanTile::GenerateMesh(ID3D11Device* pDevice, Mesh &rMeshOut)
{
	int indiceCount = 0;
	int halfResolution = m_resolution / 2;
	for (int i = 0; i < m_resolution; i++)
	{
		float horizontalPosition = i * unitWidth;
		
		for (int j = 0; j < m_resolution; j++)
		{
			int currentIdx = i * (m_resolution) + j;
			float verticalPosition = j * unitWidth;
			
			vertices.push_back(v3(horizontalPosition, 0.0f, verticalPosition));
			normals.push_back(v3(0.0f, 1.0f, 0.0f));
			uvs.push_back(v2(i * 1.0f / (m_resolution - 1), j * 1.0f / (m_resolution - 1)));
			
			if (j != m_resolution - 1 && i != m_resolution - 1) 
			{
				indices.push_back(currentIdx + 1);
				indices.push_back(currentIdx);
				indices.push_back(currentIdx + m_resolution + 1);

				indices.push_back(currentIdx + m_resolution + 1);
				indices.push_back(currentIdx);
				indices.push_back(currentIdx + m_resolution);
			}
		}
	}

	std::vector<MeshVertex> verts;

	for (int i = 0; i < vertices.size(); ++i)
	{
		verts.push_back(MeshVertex(vertices[i], 0xFFFFFFFF, normals[i], uvs[i]));
	}
	rMeshOut.init_buffers(pDevice, &verts[0], verts.size(), &indices[0], indices.size());
}

