namespace cage
{
	struct CAGE_API mat3
	{
		// data
		real data[9];

		// constructors
		mat3(); // identity
		explicit mat3(const real other[9]);
		explicit mat3(real a, real b, real c, real d, real e, real f, real g, real h, real i);
		explicit mat3(const vec3 &forward, const vec3 &up, bool keepUp = false);
		explicit mat3(const quat &other);
		explicit mat3(const mat4 &other);

		// compound operators
		mat3 &operator *= (real other) { return *this = *this * other; }
		mat3 &operator += (const mat3 &other) { return *this = *this + other; }
		mat3 &operator *= (const mat3 &other) { return *this = *this * other; }

		// binary operators
		mat3 operator * (real other) const;
		vec3 operator * (const vec3 &other) const;
		mat3 operator + (const mat3 &other) const;
		mat3 operator * (const mat3 &other) const;

		real operator [] (uint32 idx) const { CAGE_ASSERT_RUNTIME(idx < 9, "index out of range", idx); return data[idx]; }
		real &operator [] (uint32 idx) { CAGE_ASSERT_RUNTIME(idx < 9, "index out of range", idx); return data[idx]; }

		// comparison operators
		bool operator == (const mat3 &other) const { for (uint8 i = 0; i < 9; i++) if (data[i] != other.data[i]) return false; return true; }
		bool operator != (const mat3 &other) const { return !(*this == other); }

		// conversion operators
		operator string() const { string res = string() + "(" + data[0]; for (uint8 i = 1; i < 9; i++) res += string() + "," + data[i]; return res + ")"; }

		// methods
		mat3 inverse() const;
		mat3 transpose() const;
		mat3 normalize() const;
		real determinant() const;
		bool valid() const;

		// constants
		static const mat3 Zero;
		static const mat3 Nan;
	};

	struct CAGE_API mat4
	{
		// data
		real data[16];

		// constructors
		mat4(); // identity
		explicit mat4(const real other[16]);
		explicit mat4(real a, real b, real c, real d, real e, real f, real g, real h, real i, real j, real k, real l, real m, real n, real o, real p);
		explicit mat4(real other); // scale matrix
		explicit mat4(const mat3 &other);
		explicit mat4(const vec3 &other); // translation matrix
		explicit mat4(const vec3 &position, const quat &orientation, const vec3 &scale = vec3(1, 1, 1));
		explicit mat4(const quat &other) { *this = mat4(mat3(other)); }
		explicit inline mat4(const transform &other);

		// compound operators
		mat4 &operator *= (real other) { return *this = *this * other; }
		mat4 &operator += (const mat4 &other) { return *this = *this + other; }
		mat4 &operator *= (const mat4 &other) { return *this = *this * other; }

		// binary operators
		mat4 operator * (real other) const;
		vec4 operator * (const vec4 &other) const;
		mat4 operator + (const mat4 &other) const;
		mat4 operator * (const mat4 &other) const;
		real operator [] (uint32 idx) const { CAGE_ASSERT_RUNTIME(idx < 16, "index out of range", idx); return data[idx]; }
		real &operator [] (uint32 idx) { CAGE_ASSERT_RUNTIME(idx < 16, "index out of range", idx); return data[idx]; }

		// comparison operators
		bool operator == (const mat4 &other) const { for (uint8 i = 0; i < 16; i++) if (data[i] != other.data[i]) return false; return true; }
		bool operator != (const mat4 &other) const { return !(*this == other); }

		// conversion operators
		operator string() const { string res = string() + "(" + data[0]; for (uint8 i = 1; i < 16; i++) res += string() + "," + data[i]; return res + ")"; }

		// methods
		mat4 inverse() const;
		mat4 transpose() const;
		mat4 normalize() const;
		real determinant() const;
		bool valid() const;

		// constants
		static const mat4 Zero;
		static const mat4 Nan;
	};

	inline mat3 inverse(const mat3 &th) { return th.inverse(); }
	inline mat3 transpose(const mat3 &th) { return th.transpose(); }
	inline mat3 normalize(const mat3 &th) { return th.normalize(); }
	inline real determinant(const mat3 &th) { return th.determinant(); }
	inline bool valid(const mat3 &th) { return th.valid(); }
	inline mat4 inverse(const mat4 &th) { return th.inverse(); }
	inline mat4 transpose(const mat4 &th) { return th.transpose(); }
	inline mat4 normalize(const mat4 &th) { return th.normalize(); }
	inline real determinant(const mat4 &th) { return th.determinant(); }
	inline bool valid(const mat4 &th) { return th.valid(); }

	CAGE_API mat4 lookAt(const vec3 &eye, const vec3 &target, const vec3 &up);
	CAGE_API mat4 perspectiveProjection(rads fov, real aspectRatio, real near, real far);
	CAGE_API mat4 perspectiveProjection(rads fov, real aspectRatio, real near, real far, real zeroParallaxDistance, real eyeSeparation);
	CAGE_API mat4 perspectiveProjection(real left, real right, real bottom, real top, real near, real far);
	CAGE_API mat4 orthographicProjection(real left, real right, real bottom, real top, real near, real far);
	CAGE_API mat3 modelToNormal(const mat4 &value);

	inline vec3 vec3::operator * (const mat3 &other) const { return other * *this; }
	inline vec4 vec4::operator * (const mat4 &other) const { return other * *this; }

	template<uint32 N> struct matN {};
	template<> struct matN<3> { typedef mat3 type; };
	template<> struct matN<4> { typedef mat4 type; };
}
