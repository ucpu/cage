#include <cage-core/math.h>
#include <cage-core/serialization.h>

namespace cage
{
	namespace
	{
		Real radicalInverse_VdC(uint32 bits)
		{
			bits = (bits << 16u) | (bits >> 16u);
			bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
			bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
			bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
			bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
			return Real(bits) * 2.3283064365386963e-10; // / 0x100000000
		}

		Vec2 hammersley(uint32 i, uint32 N)
		{
			return Vec2(Real(i) / Real(N), radicalInverse_VdC(i));
		}

		Vec3 hemisphereSample(Vec2 xi)
		{
			Rads phi = Rads(2 * Real::Pi() * xi[0]);
			Real cosTheta = 1 - xi[1];
			Real sinTheta = sqrt(1 - cosTheta * cosTheta);
			return Vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
		}

		Vec3 pushForward(Vec3 v)
		{
			// prevents sampling near original plane, to reduce fake occlusion
			return normalize(v + Vec3(0, 0, 0.2));
		}

		struct Sequence
		{
			Vec4 data[256];
			uint32 size = 0;

			void generate()
			{
				CAGE_ASSERT(size <= array_size(data));
				for (uint32 i = 0; i < size; i++)
					data[i] = Vec4(pushForward(hemisphereSample(hammersley(i, size))), 0);
			}
		};
	}

	namespace privat
	{
		PointerRange<const char> pointsForSsaoShader(uint32 count)
		{
			static Sequence buffer;
			if (buffer.size != count)
			{
				buffer.size = count;
				buffer.generate();
			}
			return bufferCast<const char, const Vec4>(buffer.data);
		}
	}
}
