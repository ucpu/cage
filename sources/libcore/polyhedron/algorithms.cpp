#include "polyhedron.h"

namespace cage
{
	void Polyhedron::convertToIndexed()
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_THROW_CRITICAL(NotImplemented, "convertToIndexed");
	}

	void Polyhedron::convertToExpanded()
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_THROW_CRITICAL(NotImplemented, "convertToExpanded");
	}

	void Polyhedron::generateTexture(const PolyhedronTextureGenerationConfig &config)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_THROW_CRITICAL(NotImplemented, "generateTexture");
	}

	void Polyhedron::generateNormals(const PolyhedronNormalsGenerationConfig &config)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_THROW_CRITICAL(NotImplemented, "generateNormals");
	}

	void Polyhedron::generateTangents(const PolyhedronTangentsGenerationConfig &config)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_THROW_CRITICAL(NotImplemented, "generateTangents");
	}

	void Polyhedron::applyTransform(const transform &t)
	{
		// todo optimized code
		applyTransform(mat4(t));
	}

	void Polyhedron::applyTransform(const mat4 &t)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_THROW_CRITICAL(NotImplemented, "applyTransform");
	}

	void Polyhedron::clip(const aabb &box)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_THROW_CRITICAL(NotImplemented, "clip");
	}

	void Polyhedron::clip(const plane &pln)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_THROW_CRITICAL(NotImplemented, "clip");
	}

	Holder<Polyhedron> Polyhedron::cut(const plane &pln)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_THROW_CRITICAL(NotImplemented, "cut");
	}

	void Polyhedron::discardDisconnected()
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_THROW_CRITICAL(NotImplemented, "discardDisconnected");
	}

	Holder<PointerRange<Holder<Polyhedron>>> Polyhedron::separateDisconnected()
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_THROW_CRITICAL(NotImplemented, "separateDisconnected");
	}
}
