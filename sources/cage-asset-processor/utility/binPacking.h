namespace cage
{
	class binPackingClass
	{
	public:
		void add(uint32 id, uint32 width, uint32 height);
		bool solve(uint32 width, uint32 height);
		void get(uint32 index, uint32 &id, uint32 &x, uint32 &y);
	};

	holder<binPackingClass> newBinPacking();
}
