namespace cage
{
	template<class T>
	struct pointerRange
	{
	private:
		T *begin_;
		T *end_;

	public:
		pointerRange() : begin_(nullptr), end_(nullptr)
		{}

		pointerRange(T *begin, T *end) : begin_(begin), end_(end)
		{}

		template<class U>
		pointerRange(U &other) : begin_(other.data()), end_(other.data() + other.size())
		{}

		T *begin() const { return begin_; }
		T *end() const { return end_; }
		T *data() const { return begin_; }
		uintPtr size() const { return end_ - begin_; }
	};

	template<class T>
	constexpr T *begin(const holder<pointerRange<T>> &h)
	{
		return h->begin();
	}

	template<class T>
	constexpr T *end(const holder<pointerRange<T>> &h)
	{
		return h->end();
	}
}
