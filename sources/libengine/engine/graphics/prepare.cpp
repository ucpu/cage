#include <cage-core/swapBufferGuard.h>

#include <cage-engine/provisionalGraphics.h>
#include <cage-engine/provisionalHandles.h>
#include <cage-engine/screenSpaceEffects.h>
#include <cage-engine/shaderConventions.h>
#include <cage-engine/renderQueue.h>
#include <cage-engine/window.h>
#include <cage-engine/opengl.h> // all the constants

#include <cage-engine/renderObject.h>
#include <cage-engine/texture.h>
#include <cage-engine/model.h>
#include <cage-engine/font.h>

#include "graphics.h"

namespace cage
{
	namespace
	{
		struct Mat3x4
		{
			vec4 data[3];

			Mat3x4() : Mat3x4(mat4())
			{}

			explicit Mat3x4(const mat3 &in)
			{
				for (uint32 a = 0; a < 3; a++)
					for (uint32 b = 0; b < 3; b++)
						data[b][a] = in[a * 3 + b];
			}

			explicit Mat3x4(const mat4 &in)
			{
				CAGE_ASSERT(in[3] == 0 && in[7] == 0 && in[11] == 0 && in[15] == 1);
				for (uint32 a = 0; a < 4; a++)
					for (uint32 b = 0; b < 3; b++)
						data[b][a] = in[a * 4 + b];
			}
		};

		struct UniModel
		{
			mat4 mvpMat;
			mat4 mvpPrevMat;
			Mat3x4 normalMat; // [2][3] is 1 if lighting is enabled and 0 otherwise
			Mat3x4 mMat;
			vec4 color; // color rgb is linear, and NOT alpha-premultiplied
			vec4 aniTexFrames;
		};

		struct UniLight
		{
			mat4 shadowMat;
			mat4 mvpMat;
			vec4 color; // + spotAngle
			vec4 position;
			vec4 direction; // + normalOffsetScale
			vec4 attenuation; // + spotExponent
		};

		struct UniViewport
		{
			mat4 vpInv;
			vec4 eyePos;
			vec4 eyeDir;
			vec4 viewport;
			vec4 ambientLight;
			vec4 ambientDirectionalLight;
		};

		struct Preparator
		{
			const EmitBuffer &emit;
			const uint64 prepareTime;

			Preparator(const EmitBuffer &emit, uint64 time) : emit(emit), prepareTime(time)
			{}

			void prepare()
			{
				// todo
			}
		};
	}

	void Graphics::prepare(uint64 time)
	{
		renderQueue->resetQueue();

		if (auto lock = emitBuffersGuard->read())
		{
			Preparator prep(emitBuffers[lock.index()], time);
			prep.prepare();
		}
	}
}
