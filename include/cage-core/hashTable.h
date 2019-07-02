#ifndef guard_hashTable_h_16a79cb4_223b_4dcc_ac3e_4e801bc98e97_
#define guard_hashTable_h_16a79cb4_223b_4dcc_ac3e_4e801bc98e97_

namespace cage
{
	namespace detail
	{
		CAGE_API uint32 hash(uint32 key);
	}

	namespace privat
	{
		struct CAGE_API hashTableLineStruct
		{
			void *second;
			uint32 first;
		};

		class CAGE_API hashTablePriv : private immovable
		{
		public:
			void add(uint32 name, void *value);
			void remove(uint32 name);
			void *get(uint32 name, bool allowNull) const;
			bool exists(uint32 name) const;
			const hashTableLineStruct *begin() const;
			const hashTableLineStruct *end() const;
			void next(const hashTableLineStruct *&it) const;
			uint32 count() const;
			void clear();
		};

		CAGE_API holder<hashTablePriv> newHashTable(uint32 initItems, uint32 maxItems, float maxFillRate);
	}

	template<class T>
	struct hashTablePair
	{
		T *second;
		uint32 first;
	};

	template<class T>
	struct hashTableIt
	{
		hashTableIt(const privat::hashTablePriv *table, const privat::hashTableLineStruct *it) : table(table), it(it) {}
		const hashTablePair<T> &operator *() const { return *(hashTablePair<T>*)it; }
		const hashTablePair<T> *operator ->() const { return (hashTablePair<T>*)it; }
		void operator ++() { table->next(it); }
		void operator ++(int) { table->next(it); }
		bool operator == (const hashTableIt &other) const { return it == other.it; }
		bool operator != (const hashTableIt &other) const { return !(*this == other); }

	private:
		const privat::hashTablePriv *const table;
		const privat::hashTableLineStruct *it;
	};

	template<class T>
	class hashTable : private immovable
	{
	public:
		hashTable(holder<privat::hashTablePriv> table) : table(templates::move(table)) {}
		void add(uint32 name, T *value) { return table->add(name, value); }
		void remove(uint32 name) { return table->remove(name); }
		T *get(uint32 name, bool allowNull) const { return (T*)table->get(name, allowNull); }
		bool exists(uint32 name) const { return table->exists(name); } 
		hashTableIt<T> begin() const { return hashTableIt<T>(table.get(), table->begin()); }
		hashTableIt<T> end() const { return hashTableIt<T>(table.get(), table->end()); }
		uint32 count() const { return table->count(); }
		void clear() { return table->clear(); }

	private:
		holder<privat::hashTablePriv> table;
	};

	template<class T>
	holder<hashTable<T>> newHashTable(uint32 initItems, uint32 maxItems, float maxFillRate = 0.6)
	{
		return detail::systemArena().createHolder<hashTable<T>>(privat::newHashTable(initItems, maxItems, maxFillRate));
	}
}

#endif // guard_hashTable_h_16a79cb4_223b_4dcc_ac3e_4e801bc98e97_
