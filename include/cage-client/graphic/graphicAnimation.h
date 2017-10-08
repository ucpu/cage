namespace cage
{
	class CAGE_API animationClass
	{
	public:
		void allocate(uint64 duration, uint32 bones, const uint16 *indexes, const uint16 *positionFrames, const uint16 *rotationFrames, void *data);

		real coefficient(uint64 time) const;
		mat4 evaluate(uint16 bone, real coef) const;
		uint64 duration() const;
	};

	CAGE_API holder<animationClass> newAnimation();
}
