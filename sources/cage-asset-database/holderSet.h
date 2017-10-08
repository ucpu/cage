#ifndef guard_holderSet_h_ewkjbguewnjgoijerjigkjiekbgnkljewnglk
#define guard_holderSet_h_ewkjbguewnjgoijerjigkjiekbgnkljewnglk

template<class T> struct holderSet
{
	struct comparatorStruct
	{
		bool operator() (const holder<T> &a, const holder<T> &b) const
		{
			return *a < *b;
		}
	};

	typedef std::set<holder<T>, comparatorStruct> type;
	typedef typename type::iterator iterator;

	iterator find(const string &name) const
	{
		T tmp;
		tmp.name = name;
		holder<T> tmh(&tmp, delegate<void(void*)>());
		return const_cast<holderSet*>(this)->data.find(tmh);
	}

	T *retrieve(const string &name)
	{
		iterator it = find(name);
		if (it == end())
			return nullptr;
		return const_cast<T*> (it->get());
	}

	T *insert(const T &value)
	{
		return const_cast<T*> (data.insert(detail::systemArena().createHolder<T>(value)).first->get());
	}

	iterator erase(const iterator &what)
	{
		return data.erase(what);
	}

	iterator erase(const string &name)
	{
		return erase(find(name));
	}

	void clear()
	{
		data.clear();
	}

	iterator begin()
	{
		return data.begin();
	}

	iterator end()
	{
		return data.end();
	}

	bool exists(const string &name) const
	{
		return find(name) != data.end();
	}

	uint32 size() const
	{
		return numeric_cast<uint32>(data.size());
	}

	void load(fileClass *file)
	{
		uint32 s = 0;
		read(file, s);
		for (uint32 i = 0; i < s; i++)
		{
			T tmp;
			tmp.load(file);
			insert(tmp);
		}
	}

	void save(fileClass *file)
	{
		uint32 s = size();
		write(file, s);
		for (iterator it = begin(), et = end(); it != et; it++)
			(*it)->save(file);
	}

private:
	type data;
};

#endif
