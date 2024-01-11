#ifndef guard_flatBag_sezik4edrt5
#define guard_flatBag_sezik4edrt5

#include <vector>

#include <unordered_dense.h>

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
			if (indices.count(value))
				return;
			indices[value] = data_.size();
			data_.push_back(value);
		}

		constexpr uintPtr erase(const Value &value)
		{
			auto it = indices.find(value);
			if (it == indices.end())
				return 0;
			const uintPtr off = it->second;
			indices.erase(it);
			if (off + 1 != data_.size())
			{
				indices[data_.back()] = off;
				std::swap(data_[off], data_.back());
			}
			data_.pop_back();
			return 1;
		}

		constexpr void clear()
		{
			indices.clear();
			data_.clear();
		}

		constexpr void reserve(uintPtr s)
		{
			indices.reserve(s);
			data_.reserve(s);
		}

		constexpr const_iterator find(const Value &value) const
		{
			auto it = indices.find(value);
			if (it == indices.end())
				return data_.end();
			return data_.begin() + it->second;
		}

		constexpr uintPtr count(const Value &value) const { return find(value) == end() ? 0 : 1; }

		constexpr uintPtr size() const noexcept { return data_.size(); }

		constexpr bool empty() const noexcept { return data_.empty(); }

		constexpr const Value *data() const noexcept { return data_.data(); }

		constexpr const_iterator begin() const noexcept { return data_.begin(); }

		constexpr const_iterator end() const noexcept { return data_.end(); }

		constexpr std::vector<Value> &unsafeData() noexcept { return data_; }

	private:
		ankerl::unordered_dense::map<Value, uintPtr> indices;
		std::vector<Value> data_;
	};
}

#endif // guard_flatBag_sezik4edrt5
