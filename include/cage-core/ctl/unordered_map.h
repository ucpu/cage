
#include <robin_hood.h>

namespace cage
{
	template<typename Key, typename T>
	class unordered_map : protected robin_hood::unordered_map<Key, T>
	{
		using base = robin_hood::unordered_map<Key, T>;
	public:
		using key_type = Key;
		using value_type = T;
		using size_type = typename base::size_type;
		using iterator = typename base::iterator;
		using const_iterator = typename base::const_iterator;

		T &at(const Key &key)
		{
			try
			{
				return base::at(key);
			}
			catch (const std::exception &e)
			{
				CAGE_THROW_ERROR(cage::exception, e.what());
			}
		}

		const T &at(const Key &key) const
		{
			try
			{
				return base::at(key);
			}
			catch (const std::exception &e)
			{
				CAGE_THROW_ERROR(cage::exception, e.what());
			}
		}

		T &operator[](const Key &key)
		{
			try
			{
				return base::operator[](key);
			}
			catch (const std::exception &e)
			{
				CAGE_THROW_ERROR(cage::exception, e.what());
			}
		}

		T &operator[](Key &&key)
		{
			try
			{
				return base::operator[](templates::forward<Key>(key));
			}
			catch (const std::exception &e)
			{
				CAGE_THROW_ERROR(cage::exception, e.what());
			}
		}

		size_type count(const Key &key) const
		{
			try
			{
				return base::count(key);
			}
			catch (const std::exception &e)
			{
				CAGE_THROW_ERROR(cage::exception, e.what());
			}
		}

		iterator erase(const_iterator pos)
		{
			try
			{
				return base::erase(pos);
			}
			catch (const std::exception &e)
			{
				CAGE_THROW_ERROR(cage::exception, e.what());
			}
		}

		iterator erase(const_iterator first, const_iterator last)
		{
			try
			{
				return base::erase(first, last);
			}
			catch (const std::exception &e)
			{
				CAGE_THROW_ERROR(cage::exception, e.what());
			}
		}

		size_type erase(const key_type &key)
		{
			try
			{
				return base::erase(key);
			}
			catch (const std::exception &e)
			{
				CAGE_THROW_ERROR(cage::exception, e.what());
			}
		}

		template<class... Args>
		std::pair<iterator, bool> emplace(Args &&... args)
		{
			try
			{
				return base::emplace(templates::forward<Args>(args)...);
			}
			catch (const std::exception &e)
			{
				CAGE_THROW_ERROR(cage::exception, e.what());
			}
		}

		iterator find(const Key &key)
		{
			try
			{
				return base::find(key);
			}
			catch (const std::exception &e)
			{
				CAGE_THROW_ERROR(cage::exception, e.what());
			}
		}

		const_iterator find(const Key &key) const
		{
			try
			{
				return base::find(key);
			}
			catch (const std::exception &e)
			{
				CAGE_THROW_ERROR(cage::exception, e.what());
			}
		}

		std::pair<iterator, bool> insert(const value_type &value);
		template<class P>
		std::pair<iterator, bool> insert(P &&value);
		template<class InputIt>
		void insert(InputIt first, InputIt last);
		void insert(std::initializer_list<value_type> ilist);

		using base::clear;
		using base::empty;
		using base::size;
		using base::begin;
		using base::cbegin;
		using base::end;
		using base::cend;
	};
}
