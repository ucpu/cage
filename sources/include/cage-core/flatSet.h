#ifndef guard_flatSet_BDA291015B42
#define guard_flatSet_BDA291015B42

#include "core.h"

#include <vector>
#include <algorithm> // lower_bound, binary_search
#include <utility> // pair
#include <functional> // less

namespace cage
{
	template<class Value, class Compare = std::less<Value>>
	struct FlatSet
	{
	public:
		using key_type = Value;
		using value_type = Value;
		using const_iterator = typename std::vector<Value>::const_iterator;
		using const_reverse_iterator = typename std::vector<Value>::const_reverse_iterator;
		using size_type = typename std::vector<Value>::size_type;

		FlatSet() = default;
		FlatSet(const FlatSet &other) = default;
		FlatSet(FlatSet &&other) = default;
		
		template<class InputIt>
		FlatSet(InputIt first, InputIt last)
		{
			insert(first, last);
		}

		FlatSet(std::initializer_list<Value> init)
		{
			insert(init.begin(), init.end());
		}

		FlatSet &operator = (const FlatSet &other) = default;
		FlatSet &operator = (FlatSet &&other) = default;

		FlatSet &operator = (std::initializer_list<Value> ilist)
		{
			clear();
			insert(ilist.begin(), ilist.end());
			return *this;
		}

		std::pair<const_iterator, bool> insert(const Value &value)
		{
			auto it = std::lower_bound<const_iterator, Value, Compare>(data_.begin(), data_.end(), value, Compare());
			if (it != data_.end() && equals(*it, value))
				return { it, false };
			return { data_.insert(it, value), true };
		}

		std::pair<const_iterator, bool> insert(Value &&value)
		{
			auto it = std::lower_bound<const_iterator, Value, Compare>(data_.begin(), data_.end(), value, Compare());
			if (it != data_.end() && equals(*it, value))
				return { it, false };
			return { data_.insert(it, std::move(value)), true };
		}

		template<class InputIt>
		void insert(InputIt first, InputIt last)
		{
			while (first != last)
				insert(*first++);
		}

		void insert(std::initializer_list<Value> ilist)
		{
			insert(ilist.begin(), ilist.end());
		}

		const_iterator erase(const_iterator pos)
		{
			return data_.erase(pos);
		}

		const_iterator erase(const_iterator first, const_iterator last)
		{
			return data_.erase(first, last);
		}

		uintPtr erase(const Value &value)
		{
			auto it = std::lower_bound<const_iterator, Value, Compare>(data_.begin(), data_.end(), value, Compare());
			if (it != data_.end() && equals(*it, value))
			{
				data_.erase(it);
				return 1;
			}
			return 0;
		}

		void clear()
		{
			data_.clear();
		}

		void reserve(uintPtr s)
		{
			data_.reserve(s);
		}

		const_iterator find(const Value &value) const
		{
			auto it = std::lower_bound<const_iterator, Value, Compare>(data_.begin(), data_.end(), value, Compare());
			if (it != data_.end() && equals(*it, value))
				return it;
			return data_.end();
		}

		uintPtr count(const Value &value) const
		{
			return std::binary_search<const_iterator, Value, Compare>(data_.begin(), data_.end(), value, Compare());
		}

		uintPtr size() const noexcept
		{
			return data_.size();
		}

		bool empty() const noexcept
		{
			return data_.empty();
		}

		const Value *data() const noexcept
		{
			return data_.data();
		}

		const_iterator begin() const noexcept
		{
			return data_.begin();
		}

		const_iterator end() const noexcept
		{
			return data_.end();
		}

		const_reverse_iterator rbegin() const noexcept
		{
			return data_.rbegin();
		}

		const_reverse_iterator rend() const noexcept
		{
			return data_.rend();
		}

	private:
		std::vector<Value> data_;

		static bool equals (const Value &a, const Value &b)
		{
			return !Compare()(a, b) && !Compare()(b, a);
		}

		friend bool operator == (const FlatSet &a, const FlatSet &b)
		{
			return a.data_ == b.data_;
		}
	};
}

#endif // guard_flatSet_BDA291015B42
