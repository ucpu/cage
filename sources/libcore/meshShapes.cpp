#include <cage-core/meshShapes.h>

#include <unordered_map>

namespace cage
{
	Holder<Mesh> newMeshSphereUv(real radius, uint32 segments, uint32 rings)
	{
		Holder<Mesh> mesh = newMesh();

		const vec2 uvScale = 1 / vec2(segments - 1, rings - 1);
		for (uint32 r = 0; r < rings; r++)
		{
			const rads b = degs(180 * real(r) / (rings - 1) - 90);
			for (uint32 s = 0; s < segments; s++)
			{
				const rads a = degs(360 * real(s) / (segments - 1));
				const real cosb = cos(b);
				const vec3 pos = vec3(cos(a) * cosb, sin(a) * cosb, sin(b));
				CAGE_ASSERT(abs(distance(pos, normalize(pos))) < 1e-4);
				mesh->addVertex(pos * radius, pos, vec2(s, r) * uvScale);
			}
		}

		for (uint32 r = 0; r < rings - 1; r++)
		{
			for (uint32 s = 0; s < segments - 1; s++)
			{
				mesh->addTriangle(r * segments + s, (r + 1) * segments + s + 1, (r + 1) * segments + s);
				mesh->addTriangle(r * segments + s, r * segments + s + 1, (r + 1) * segments + s + 1);
			}
		}

		return mesh;
	}

	namespace
	{
		Holder<Mesh> newMeshIcosahedronRaw()
		{
			Holder<Mesh> mesh = newMesh();

			// https://www.danielsieger.com/blog/2021/01/03/generating-platonic-solids.html

			const real phi = (1.0f + sqrt(5.0f)) * 0.5f; // golden ratio
			const real a = 1.0f;
			const real b = 1.0f / phi;

			mesh->addVertex(normalize(vec3(0, +b, -a)));
			mesh->addVertex(normalize(vec3(+b, +a, 0)));
			mesh->addVertex(normalize(vec3(-b, +a, 0)));
			mesh->addVertex(normalize(vec3(0, +b, +a)));
			mesh->addVertex(normalize(vec3(0, -b, +a)));
			mesh->addVertex(normalize(vec3(-a, 0, +b)));
			mesh->addVertex(normalize(vec3(0, -b, -a)));
			mesh->addVertex(normalize(vec3(+a, 0, -b)));
			mesh->addVertex(normalize(vec3(+a, 0, +b)));
			mesh->addVertex(normalize(vec3(-a, 0, -b)));
			mesh->addVertex(normalize(vec3(+b, -a, 0)));
			mesh->addVertex(normalize(vec3(-b, -a, 0)));

			mesh->addTriangle(2, 1, 0);
			mesh->addTriangle(1, 2, 3);
			mesh->addTriangle(5, 4, 3);
			mesh->addTriangle(4, 8, 3);
			mesh->addTriangle(7, 6, 0);
			mesh->addTriangle(6, 9, 0);
			mesh->addTriangle(11, 10, 4);
			mesh->addTriangle(10, 11, 6);
			mesh->addTriangle(9, 5, 2);
			mesh->addTriangle(5, 9, 11);
			mesh->addTriangle(8, 7, 1);
			mesh->addTriangle(7, 8, 10);
			mesh->addTriangle(2, 5, 3);
			mesh->addTriangle(8, 1, 3);
			mesh->addTriangle(9, 2, 0);
			mesh->addTriangle(1, 7, 0);
			mesh->addTriangle(11, 9, 6);
			mesh->addTriangle(7, 10, 6);
			mesh->addTriangle(5, 11, 4);
			mesh->addTriangle(10, 8, 4);

			return mesh;
		}

		real averageEdgesLength(const Mesh *mesh)
		{
			real sum = 0;
			uint32 cnt = 0;
			const uint32 facesCount = mesh->facesCount();
			const auto indices = mesh->indices();
			const auto positions = mesh->positions();
			for (uint32 i = 0; i < facesCount; i++)
			{
				const uint32 a = indices[i * 3 + 0];
				const uint32 b = indices[i * 3 + 1];
				const uint32 c = indices[i * 3 + 2];
				sum += distance(positions[a], positions[b]);
				sum += distance(positions[b], positions[c]);
				sum += distance(positions[c], positions[a]);
				cnt += 3;
			}
			return sum / cnt;
		}
	}

	Holder<Mesh> newMeshSphereRegular(real radius, real edgeLength)
	{
		Holder<Mesh> mesh = newMeshIcosahedronRaw();

		while (true)
		{
			if (averageEdgesLength(+mesh) * radius < edgeLength)
				break;

			// subdivide the mesh
			Holder<Mesh> tmp = newMesh();
			tmp->positions(mesh->positions());
			const auto originalPositions = mesh->positions();
			struct Hasher {
				std::size_t operator () (const std::pair<uint32, uint32> &p) const
				{
					const auto h = std::hash<uint32>();
					return h(p.first) ^ p.second + h(p.second);
				}
			};
			std::unordered_map<std::pair<uint32, uint32>, uint32, Hasher> mapping;
			mapping.reserve(mesh->verticesCount() * 2);
			const auto &split = [&](uint32 a, uint32 b) -> uint32 {
				uint32 &r = mapping[std::pair(min(a, b), max(a, b))];
				if (r == 0)
				{
					r = tmp->verticesCount();
					tmp->addVertex(normalize(originalPositions[a] + originalPositions[b]));
				}
				return r;
			};
			const auto originalIndices = mesh->indices();
			const uint32 originalFacesCount = mesh->facesCount();
			for (uint32 i = 0; i < originalFacesCount; i++)
			{
				const uint32 a = originalIndices[i * 3 + 0];
				const uint32 b = originalIndices[i * 3 + 1];
				const uint32 c = originalIndices[i * 3 + 2];
				const uint32 d = split(a, b);
				const uint32 e = split(b, c);
				const uint32 f = split(c, a);
				tmp->addTriangle(a, d, f);
				tmp->addTriangle(d, e, f);
				tmp->addTriangle(b, e, d);
				tmp->addTriangle(c, f, e);
			}
			std::swap(mesh, tmp);
		}

		// copy positions into normals
		mesh->normals(mesh->positions());

		// scale positions by radius
		for (vec3 &p : mesh->positions())
			p *= radius;

		return mesh;
	}

	Holder<Mesh> newMeshIcosahedron(real radius)
	{
		Holder<Mesh> mesh = newMeshIcosahedronRaw();

		// copy positions into normals
		mesh->normals(mesh->positions());

		// scale positions by radius
		for (vec3 &p : mesh->positions())
			p *= radius;

		return mesh;
	}
}
