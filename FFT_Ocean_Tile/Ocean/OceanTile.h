#pragma once

#include "Framework.h"
#include "ShaderSet.h"
#include "Mesh.h"

#include <vector>

class OceanTile
{
// Singleton Pattern
public:
	static OceanTile& getInstance()
	{
		static OceanTile instance;
		return instance;
	}

private:
	OceanTile() {}

public:
	OceanTile(OceanTile const&) = delete; // Don't Implement
	void operator=(OceanTile const&) = delete;   // Don't Implement

// Members
private:
	int m_resolution = 200;
	float unitWidth = 3.0f;

// Arrays
private:
	std::vector<v3> vertices;
	std::vector<u16> indices;
	std::vector<v3> normals;
	std::vector<v2> uvs;

// Functions
public:
	const int& getResolution() { return m_resolution; }
	void GenerateMesh(ID3D11Device* pDevice, Mesh &rMeshOut);
};

