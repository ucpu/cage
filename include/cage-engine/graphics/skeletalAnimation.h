namespace cage
{
	class CAGE_API SkeletalAnimation : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const string &name);

		void allocate(uint64 duration, uint32 bones, const uint16 *indexes, const uint16 *positionFrames, const uint16 *rotationFrames, const uint16 *scaleFrames, const void *data);

		mat4 evaluate(uint16 bone, real coef) const;
		uint64 duration() const;
	};

	CAGE_API Holder<SkeletalAnimation> newSkeletalAnimation();

	namespace detail
	{
		CAGE_API real evalCoefficientForSkeletalAnimation(SkeletalAnimation *animation, uint64 emitTime, uint64 animationStart, real animationSpeed, real animationOffset);
	}

	CAGE_API AssetScheme genAssetSchemeSkeletalAnimation();
	static const uint32 AssetSchemeIndexSkeletalAnimation = 14;
}
