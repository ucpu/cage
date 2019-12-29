#ifndef guard_holderSet_h_ewkjbguewnjgoijerjigkjiekbgnkljewnglk
#define guard_holderSet_h_ewkjbguewnjgoijerjigkjiekbgnkljewnglk

template<class T>
struct holderSet
{
	struct comparatorStruct
	{
		bool operator() (const Holder<T> &a, const Holder<T> &b) const
		{
			return *a < *b;
		}
	};

	typedef std::set<Holder<T>, comparatorStruct> type;
	typedef typename type::iterator iterator;

	iterator find(const string &name) const
	{
		T tmp;
		tmp.name = name;
		Holder<T> tmh(&tmp, nullptr, Delegate<void(void*)>());
		return const_cast<holderSet*>(this)->data.find(tmh);
	}

	T *retrieve(const string &name)
	{
		iterator it = find(name);
		if (it == end())
			return nullptr;
		return const_cast<T*>(it->get());
	}

	T *insert(T &&value)
	{
		return const_cast<T*>(data.insert(detail::systemArena().createHolder<T>(templates::move(value))).first->get());
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

	void load(File *fileName)
	{
		uint32 s = 0;
		read(fileName, s);
		for (uint32 i = 0; i < s; i++)
		{
			T tmp;
			tmp.load(fileName);
			insert(templates::move(tmp));
		}
	}

	void save(File *fileName)
	{
		uint32 s = size();
		write(fileName, s);
		for (iterator it = begin(), et = end(); it != et; it++)
			(*it)->save(fileName);
	}

private:
	type data;
};

#endif
