#ifndef PIECES_H
#define PIECES_H

#include <vector>
#include <cmath>
#include <glm/glm.hpp>
#include "constants.h"

struct Mesh
{
	std::vector<float> positions;
	std::vector<float> normals;
	std::vector<float> texCoords;
	std::vector<float> tangents;
	int vertexCount = 0;
};

inline void addVertex(Mesh &mesh, const glm::vec3 &pos, const glm::vec3 &normal)
{
	mesh.positions.push_back(pos.x); mesh.positions.push_back(pos.y); mesh.positions.push_back(pos.z);
	mesh.normals.push_back(normal.x); mesh.normals.push_back(normal.y); mesh.normals.push_back(normal.z);
}

inline void addVertex(Mesh &mesh, const glm::vec3 &pos, const glm::vec3 &normal, const glm::vec2 &uv, const glm::vec3 &tangent)
{
	mesh.positions.push_back(pos.x); mesh.positions.push_back(pos.y); mesh.positions.push_back(pos.z);
	mesh.normals.push_back(normal.x); mesh.normals.push_back(normal.y); mesh.normals.push_back(normal.z);
	mesh.texCoords.push_back(uv.x); mesh.texCoords.push_back(uv.y);
	mesh.tangents.push_back(tangent.x); mesh.tangents.push_back(tangent.y); mesh.tangents.push_back(tangent.z);
}

inline Mesh makeLathe(const std::vector<glm::vec2> &profile, int segments)
{
	Mesh mesh;
	int pointCount = (int)profile.size();
	if (pointCount < 2)
		return mesh;

	std::vector<glm::vec2> profileNormals(pointCount);
	for (int i = 0; i < pointCount; i++)
	{
		glm::vec2 tangent2D;
		if (i == 0)
			tangent2D = profile[1] - profile[0];
		else if (i == pointCount - 1)
			tangent2D = profile[pointCount - 1] - profile[pointCount - 2];
		else
			tangent2D = profile[i + 1] - profile[i - 1];
		glm::vec2 normal2D(tangent2D.y, -tangent2D.x);
		float length = std::sqrt(normal2D.x * normal2D.x + normal2D.y * normal2D.y);
		if (length > 1e-6f)
			normal2D /= length;
		else
			normal2D = glm::vec2(1.0f, 0.0f);
		profileNormals[i] = normal2D;
	}

	for (int i = 0; i < pointCount - 1; i++)
	{
		for (int j = 0; j < segments; j++)
		{
			float angle0 = 2.0f * PI * j / segments;
			float angle1 = 2.0f * PI * (j + 1) / segments;
			float cos0 = cosf(angle0), sin0 = sinf(angle0), cos1 = cosf(angle1), sin1 = sinf(angle1);

			glm::vec3 pos00(profile[i].x * cos0, profile[i].y, profile[i].x * sin0);
			glm::vec3 pos10(profile[i + 1].x * cos0, profile[i + 1].y, profile[i + 1].x * sin0);
			glm::vec3 pos11(profile[i + 1].x * cos1, profile[i + 1].y, profile[i + 1].x * sin1);
			glm::vec3 pos01(profile[i].x * cos1, profile[i].y, profile[i].x * sin1);

			glm::vec3 nrm00(profileNormals[i].x * cos0, profileNormals[i].y, profileNormals[i].x * sin0);
			glm::vec3 nrm10(profileNormals[i + 1].x * cos0, profileNormals[i + 1].y, profileNormals[i + 1].x * sin0);
			glm::vec3 nrm11(profileNormals[i + 1].x * cos1, profileNormals[i + 1].y, profileNormals[i + 1].x * sin1);
			glm::vec3 nrm01(profileNormals[i].x * cos1, profileNormals[i].y, profileNormals[i].x * sin1);

			float u0 = (float)j / segments, u1 = (float)(j + 1) / segments;
			float v0 = (float)i / (pointCount - 1), v1 = (float)(i + 1) / (pointCount - 1);
			glm::vec3 tan0(-sin0, 0.0f, cos0), tan1(-sin1, 0.0f, cos1);

			addVertex(mesh, pos00, nrm00, glm::vec2(u0, v0), tan0);
			addVertex(mesh, pos10, nrm10, glm::vec2(u0, v1), tan0);
			addVertex(mesh, pos11, nrm11, glm::vec2(u1, v1), tan1);
			addVertex(mesh, pos00, nrm00, glm::vec2(u0, v0), tan0);
			addVertex(mesh, pos11, nrm11, glm::vec2(u1, v1), tan1);
			addVertex(mesh, pos01, nrm01, glm::vec2(u1, v0), tan1);
		}
	}
	mesh.vertexCount = (int)mesh.positions.size() / 3;
	return mesh;
}

inline Mesh makeSphere(float radius, int stacks, int sectors)
{
	std::vector<glm::vec2> profile;
	for (int i = 0; i <= stacks; i++)
	{
		float angle = -PI / 2.0f + PI * (float)i / stacks;
		profile.push_back(glm::vec2(radius * cosf(angle), radius * sinf(angle)));
	}
	return makeLathe(profile, sectors);
}

inline Mesh makeBox(float sizeX, float sizeY, float sizeZ)
{
	Mesh mesh;
	float halfX = sizeX * 0.5f, halfY = sizeY * 0.5f, halfZ = sizeZ * 0.5f;
	glm::vec3 nPX(1, 0, 0), nNX(-1, 0, 0), nPY(0, 1, 0), nNY(0, -1, 0), nPZ(0, 0, 1), nNZ(0, 0, -1);
	glm::vec3 tPX(0, 0, 1), tNX(0, 0, -1), tPY(1, 0, 0), tNY(1, 0, 0), tPZ(1, 0, 0), tNZ(-1, 0, 0);
	addVertex(mesh, glm::vec3(halfX, -halfY, -halfZ), nPX, glm::vec2(0, 0), tPX);
	addVertex(mesh, glm::vec3(halfX, -halfY, halfZ), nPX, glm::vec2(1, 0), tPX);
	addVertex(mesh, glm::vec3(halfX, halfY, halfZ), nPX, glm::vec2(1, 1), tPX);
	addVertex(mesh, glm::vec3(halfX, -halfY, -halfZ), nPX, glm::vec2(0, 0), tPX);
	addVertex(mesh, glm::vec3(halfX, halfY, halfZ), nPX, glm::vec2(1, 1), tPX);
	addVertex(mesh, glm::vec3(halfX, halfY, -halfZ), nPX, glm::vec2(0, 1), tPX);
	addVertex(mesh, glm::vec3(-halfX, -halfY, halfZ), nNX, glm::vec2(0, 0), tNX);
	addVertex(mesh, glm::vec3(-halfX, -halfY, -halfZ), nNX, glm::vec2(1, 0), tNX);
	addVertex(mesh, glm::vec3(-halfX, halfY, -halfZ), nNX, glm::vec2(1, 1), tNX);
	addVertex(mesh, glm::vec3(-halfX, -halfY, halfZ), nNX, glm::vec2(0, 0), tNX);
	addVertex(mesh, glm::vec3(-halfX, halfY, -halfZ), nNX, glm::vec2(1, 1), tNX);
	addVertex(mesh, glm::vec3(-halfX, halfY, halfZ), nNX, glm::vec2(0, 1), tNX);
	addVertex(mesh, glm::vec3(-halfX, halfY, halfZ), nPY, glm::vec2(0, 1), tPY);
	addVertex(mesh, glm::vec3(halfX, halfY, halfZ), nPY, glm::vec2(1, 1), tPY);
	addVertex(mesh, glm::vec3(halfX, halfY, -halfZ), nPY, glm::vec2(1, 0), tPY);
	addVertex(mesh, glm::vec3(-halfX, halfY, halfZ), nPY, glm::vec2(0, 1), tPY);
	addVertex(mesh, glm::vec3(halfX, halfY, -halfZ), nPY, glm::vec2(1, 0), tPY);
	addVertex(mesh, glm::vec3(-halfX, halfY, -halfZ), nPY, glm::vec2(0, 0), tPY);
	addVertex(mesh, glm::vec3(-halfX, -halfY, -halfZ), nNY, glm::vec2(0, 1), tNY);
	addVertex(mesh, glm::vec3(halfX, -halfY, -halfZ), nNY, glm::vec2(1, 1), tNY);
	addVertex(mesh, glm::vec3(halfX, -halfY, halfZ), nNY, glm::vec2(1, 0), tNY);
	addVertex(mesh, glm::vec3(-halfX, -halfY, -halfZ), nNY, glm::vec2(0, 1), tNY);
	addVertex(mesh, glm::vec3(halfX, -halfY, halfZ), nNY, glm::vec2(1, 0), tNY);
	addVertex(mesh, glm::vec3(-halfX, -halfY, halfZ), nNY, glm::vec2(0, 0), tNY);
	addVertex(mesh, glm::vec3(-halfX, -halfY, halfZ), nPZ, glm::vec2(0, 0), tPZ);
	addVertex(mesh, glm::vec3(halfX, -halfY, halfZ), nPZ, glm::vec2(1, 0), tPZ);
	addVertex(mesh, glm::vec3(halfX, halfY, halfZ), nPZ, glm::vec2(1, 1), tPZ);
	addVertex(mesh, glm::vec3(-halfX, -halfY, halfZ), nPZ, glm::vec2(0, 0), tPZ);
	addVertex(mesh, glm::vec3(halfX, halfY, halfZ), nPZ, glm::vec2(1, 1), tPZ);
	addVertex(mesh, glm::vec3(-halfX, halfY, halfZ), nPZ, glm::vec2(0, 1), tPZ);
	addVertex(mesh, glm::vec3(halfX, -halfY, -halfZ), nNZ, glm::vec2(0, 0), tNZ);
	addVertex(mesh, glm::vec3(-halfX, -halfY, -halfZ), nNZ, glm::vec2(1, 0), tNZ);
	addVertex(mesh, glm::vec3(-halfX, halfY, -halfZ), nNZ, glm::vec2(1, 1), tNZ);
	addVertex(mesh, glm::vec3(halfX, -halfY, -halfZ), nNZ, glm::vec2(0, 0), tNZ);
	addVertex(mesh, glm::vec3(-halfX, halfY, -halfZ), nNZ, glm::vec2(1, 1), tNZ);
	addVertex(mesh, glm::vec3(halfX, halfY, -halfZ), nNZ, glm::vec2(0, 1), tNZ);
	mesh.vertexCount = (int)mesh.positions.size() / 3;
	return mesh;
}

#define LATHE_SEGMENTS 28

inline Mesh makePawn()
{
	std::vector<glm::vec2> p = {
		{0.00f, 0.00f}, {0.33f, 0.00f}, {0.33f, 0.05f}, {0.29f, 0.09f}, {0.20f, 0.13f},
		{0.13f, 0.20f}, {0.12f, 0.34f}, {0.17f, 0.40f}, {0.10f, 0.46f},
		{0.19f, 0.55f}, {0.21f, 0.66f}, {0.16f, 0.77f}, {0.00f, 0.88f}};
	return makeLathe(p, LATHE_SEGMENTS);
}

inline Mesh makeBishop()
{
	std::vector<glm::vec2> p = {
		{0.00f, 0.00f}, {0.34f, 0.00f}, {0.34f, 0.05f}, {0.30f, 0.09f}, {0.21f, 0.14f},
		{0.14f, 0.22f}, {0.12f, 0.45f}, {0.18f, 0.52f}, {0.10f, 0.58f},
		{0.17f, 0.66f}, {0.20f, 0.80f}, {0.18f, 0.95f}, {0.12f, 1.05f},
		{0.13f, 1.10f}, {0.07f, 1.16f}, {0.00f, 1.20f}};
	return makeLathe(p, LATHE_SEGMENTS);
}

inline Mesh makeRook()
{
	std::vector<glm::vec2> p = {
		{0.00f, 0.00f}, {0.36f, 0.00f}, {0.36f, 0.06f}, {0.31f, 0.10f}, {0.24f, 0.16f},
		{0.23f, 0.60f}, {0.30f, 0.66f}, {0.30f, 0.74f}, {0.00f, 0.74f}};
	return makeLathe(p, LATHE_SEGMENTS);
}

inline Mesh makeQueen()
{
	std::vector<glm::vec2> p = {
		{0.00f, 0.00f}, {0.37f, 0.00f}, {0.37f, 0.06f}, {0.32f, 0.10f}, {0.22f, 0.16f},
		{0.15f, 0.26f}, {0.13f, 0.55f}, {0.19f, 0.62f}, {0.11f, 0.68f},
		{0.18f, 0.80f}, {0.23f, 1.00f}, {0.25f, 1.15f}, {0.22f, 1.25f},
		{0.27f, 1.32f}, {0.30f, 1.40f}, {0.26f, 1.45f}, {0.00f, 1.46f}};
	return makeLathe(p, LATHE_SEGMENTS);
}

inline Mesh makeKing()
{
	std::vector<glm::vec2> p = {
		{0.00f, 0.00f}, {0.38f, 0.00f}, {0.38f, 0.06f}, {0.33f, 0.10f}, {0.23f, 0.16f},
		{0.16f, 0.26f}, {0.14f, 0.60f}, {0.20f, 0.67f}, {0.12f, 0.73f},
		{0.19f, 0.86f}, {0.24f, 1.06f}, {0.26f, 1.22f}, {0.22f, 1.33f},
		{0.27f, 1.40f}, {0.30f, 1.48f}, {0.24f, 1.53f}, {0.20f, 1.57f}, {0.00f, 1.58f}};
	return makeLathe(p, LATHE_SEGMENTS);
}

inline Mesh makeKnightBase()
{
	std::vector<glm::vec2> p = {
		{0.00f, 0.00f}, {0.34f, 0.00f}, {0.34f, 0.05f}, {0.30f, 0.09f},
		{0.22f, 0.14f}, {0.17f, 0.20f}, {0.16f, 0.30f}};
	return makeLathe(p, LATHE_SEGMENTS);
}

#endif
