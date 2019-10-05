#ifndef guard_hashTable_h_16a79cb4_223b_4dcc_ac3e_4e801bc98e97_
#define guard_hashTable_h_16a79cb4_223b_4dcc_ac3e_4e801bc98e97_

namespace cage
{
	namespace detail
	{
		CAGE_API uint32 hash(uint32 key);
	}

	struct CAGE_API hashTableCreateConfig
	{
		uint32 initItems;
		hashTableCreateConfig();
	};

	namespace privat
	{
		struct hashTablePairPriv
		{
			uint32 first;
			void *second;
		};

		struct CAGE_API hashTableItPriv
		{
			void *_ptr1, *_ptr2;
			const hashTablePairPriv &operator *() const;
			const hashTablePairPriv *operator ->() const;
			void operator ++();
			void operator ++(int) { operator++(); };
			bool operator == (const hashTableItPriv &other) const;
			bool operator != (const hashTableItPriv &other) const { return !(operator==(other)); }
		};

		class CAGE_API hashTablePriv : private immovable
		{
		public:
			void add(uint32 name, void *value);
			void remove(uint32 name);
			void *tryGet(uint32 name) const;
			void *get(uint32 name) const;
			bool exists(uint32 name) const;
			hashTableItPriv begin() const;
			hashTableItPriv end() const;
			uint32 count() const;
			void clear();
		};

		CAGE_API holder<hashTablePriv> newHashTable(const hashTableCreateConfig &config);
	}

	template<class T>
	struct hashTablePair
	{
		uint32 first;
		T *second;
	};

	template<class T>
	struct hashTableIt : public privat::hashTableItPriv
	{
		const hashTablePair<T> &operator *() const { return (hashTablePair<T>&)((hashTableItPriv*)this)->operator*(); }
		const hashTablePair<T> *operator ->() const { return (hashTablePair<T>*)((hashTableItPriv*)this)->operator->(); }
	};

	template<class T>
	class hashTable : private immovable
	{
	public:
		hashTable(holder<privat::hashTablePriv> table) : table(templates::move(table)) {}
		void add(uint32 name, T *value) { return table->add(name, value); }
		void remove(uint32 name) { return table->remove(name); }
		T *tryGet(uint32 name) const { return (T*)table->tryGet(name); } // nullptr if not found
		T *get(uint32 name) const { return (T*)table->get(name); } // throws if not found
		bool exists(uint32 name) const { return table->exists(name); }
		hashTableIt<T> begin() const { auto it = table->begin(); return *(hashTableIt<T>*)&it; }
		hashTableIt<T> end() const { auto it = table->end(); return *(hashTableIt<T>*)&it; }
		uint32 count() const { return table->count(); }
		void clear() { return table->clear(); }

	private:
		holder<privat::hashTablePriv> table;
	};

	template<class T>
	holder<hashTable<T>> newHashTable(const hashTableCreateConfig &config)
	{
		return detail::systemArena().createHolder<hashTable<T>>(privat::newHashTable(config));
	}
}

#endif // guard_hashTable_h_16a79cb4_223b_4dcc_ac3e_4e801bc98e97_
