namespace cage
{
	class CAGE_API uniformBuffer : private immovable
	{
#ifdef CAGE_DEBUG
		detail::stringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const string &name);

		uint32 getId() const;
		void bind() const;
		void bind(uint32 bindingPoint) const;
		void bind(uint32 bindingPoint, uint32 offset, uint32 size) const;

		void writeWhole(void *data, uint32 size, uint32 usage = 0);
		void writeRange(void *data, uint32 offset, uint32 size);
	};

	CAGE_API holder<uniformBuffer> newUniformBuffer();
}
