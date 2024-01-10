#ifndef guard_flatBag_sezik4edrt5
#define guard_flatBag_sezik4edrt5

#include <algorithm> // lower_bound, binary_search
#include <vector>

#include <cage-core/core.h>

namespace cage
{
	template<class Value>
	struct FlatBag
	{
	public:
		using key_type = Value;
		using value_type = Value;
		using const_iterator = typename std::vector<Value>::const_iterator;
		using size_type = uintPtr;

		constexpr void insert(const Value &value)
		{
			CAGE_ASSERT(count(value) == 0);
			data_.push_back(value);
			unsorted++;
			if (unsorted > 100)
			{
				std::sort(data_.begin(), data_.end());
				sorted = data_.size();
				unsorted = 0;
			}
		}

		constexpr uintPtr erase(const Value &value)
		{
			CAGE_ASSERT(sorted + unsorted == data_.size());
			if (unsorted > 0)
			{
				auto it = std::find(data_.begin() + sorted, data_.end(), value);
				if (it != data_.end())
				{
					std::swap(*it, data_.back());
					data_.pop_back();
					unsorted--;
					return 1;
				}
			}
			if (sorted > 0)
			{
				auto it = std::lower_bound(data_.begin(), data_.begin() + sorted, value);
				if (it == data_.begin() + sorted)
					return 0;
				if (*it != value)
					return 0;
				std::copy_n(it + 1, data_.end() - it - 1, it);
				data_.pop_back();
				sorted--;
				return 1;
			}
			return 0;
		}

		constexpr void clear()
		{
			data_.clear();
			sorted = unsorted = 0;
		}

		constexpr void reserve(uintPtr s) { data_.reserve(s); }

		constexpr const_iterator find(const Value &value) const
		{
			CAGE_ASSERT(sorted + unsorted == data_.size());
			CAGE_ASSERT(std::is_sorted(data_.begin(), data_.begin() + sorted));
			auto it = std::find(data_.begin() + sorted, data_.end(), value);
			if (it != data_.end())
				return it;
			it = std::lower_bound(data_.begin(), data_.begin() + sorted, value);
			if (it == data_.begin() + sorted)
				return data_.end();
			if (*it == value)
				return it;
			return data_.end();
		}

		constexpr uintPtr count(const Value &value) const { return find(value) == end() ? 0 : 1; }

		constexpr uintPtr size() const noexcept { return data_.size(); }

		constexpr bool empty() const noexcept { return data_.empty(); }

		constexpr const Value *data() const noexcept { return data_.data(); }

		constexpr const_iterator begin() const noexcept { return data_.begin(); }

		constexpr const_iterator end() const noexcept { return data_.end(); }

		constexpr const std::vector<Value> &unsafeData() const noexcept { return data_; }

		constexpr std::vector<Value> &unsafeData() noexcept { return data_; }

	private:
		std::vector<Value> data_;
		uintPtr sorted = 0;
		uintPtr unsorted = 0;
	};
}

#endif // guard_flatBag_sezik4edrt5
