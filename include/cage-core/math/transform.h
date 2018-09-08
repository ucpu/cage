namespace cage
{
	struct CAGE_API transform
	{
		// data
		quat orientation;
		vec3 position;
		real scale;

		// constructors
		transform();
		explicit transform(const vec3 &position, const quat &orientation = quat(), real scale = 1);

		// compound operators
		transform &operator *= (const transform &other) { return *this = *this * other; }
		transform &operator *= (const quat &other) { return *this = *this * other; }
		transform &operator += (const vec3 &other) { return *this = *this + other; }

		// binary operators
		transform operator * (const transform &other) const;
		transform operator * (const quat &other) const;
		transform operator + (const vec3 &other) const;

		// comparison operators
		bool operator == (const transform &other) const { return orientation == other.orientation && position == other.position && scale == other.scale; }
		bool operator != (const transform &other) const { return !(*this == other); }

		// conversion operators
		operator string() const { return string() + "(" + position + ", " + orientation + ", " + scale + ")"; }

		// methods
		transform inverse() const;
		bool valid() const { return orientation.valid() && position.valid() && scale.valid(); }
	};

	mat4::mat4(const transform &other) { *this = mat4(other.position, other.orientation, vec3(other.scale, other.scale, other.scale)); }

	inline transform inverse(const transform &th) { return th.inverse(); }
	inline bool valid(const transform &th) { return th.valid(); }
	CAGE_API transform interpolate(const transform &min, const transform &max, real value);
}
