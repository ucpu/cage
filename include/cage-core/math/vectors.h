#define GCHL_GEN_QUATERNION_BASE(NAME,N) \
explicit NAME(const string &str); \
NAME operator - () const { return inverse(); } \
NAME operator + () const { return *this; } \
real &operator [] (uint32 idx) { CAGE_ASSERT_RUNTIME (idx < N, "index out of range", idx, N); return data[idx]; } \
real operator [] (uint32 idx) const { CAGE_ASSERT_RUNTIME (idx < N, "index out of range", idx, N); return data[idx]; } \
bool operator == (const NAME &other) const { for (uint32 i = 0; i < N; i++) if (data[i] != other.data[i]) return false; return true; } \
bool operator != (const NAME &other) const { return !(*this == other); } \
operator string() const { string r = "("; for (uint32 i = 0; i < N - 1; i++) r += string(data[i]) + ", "; return r + string(data[N - 1]) + ")"; } \
NAME inverse() const { return *this * -1; } \
NAME normalize() const { return *this / length(); } \
real length() const { return squaredLength().sqrt(); } \
real squaredLength() const { return dot(*this); } \
real dot(const NAME &other) const { real sum = 0; for (uint32 i = 0; i < N; i++) sum += data[i] * other.data[i]; return sum; } \
bool valid() const { for (uint32 i = 0; i < N; i++) if (!data[i].valid()) return false; return true; } \
NAME &operator += (const NAME &other) { return *this = *this + other; } \
NAME &operator += (real other) { return *this = *this + other; } \
NAME &operator -= (const NAME &other) { return *this = *this - other; } \
NAME &operator -= (real other) { return *this = *this - other; } \
NAME &operator *= (real other) { return *this = *this * other; } \
NAME &operator /= (real other) { return *this = *this / other; } \
NAME operator + (const NAME &other) const { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = data[i] + other.data[i]; return r; } \
NAME operator + (real other) const { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = data[i] + other; return r; } \
NAME operator - (const NAME &other) const { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = data[i] - other.data[i]; return r; } \
NAME operator - (real other) const { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = data[i] - other; return r; } \
NAME operator * (real other) const { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = data[i] * other; return r; } \
NAME operator / (real other) const { NAME r; real d = real(1) / other; for (uint32 i = 0; i < N; i++) r.data[i] = data[i] * d; return r; } \
static const NAME Nan;

#define GCHL_GEN_VECTOR_BASE(NAME, N) \
GCHL_GEN_QUATERNION_BASE(NAME, N) \
NAME() { for (uint32 i = 0; i < N; i++) data[i] = 0; } \
NAME min(const NAME &other) const { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = data[i] < other.data[i] ? data[i] : other.data[i]; return r; } \
NAME max(const NAME &other) const { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = data[i] > other.data[i] ? data[i] : other.data[i]; return r; } \
NAME min(real other) const { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = data[i] < other ? data[i] : other; return r; } \
NAME max(real other) const { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = data[i] > other ? data[i] : other; return r; } \
NAME abs() const { return this->max(-*this); } \
NAME clamp(const NAME &minimum, const NAME &maximum) const { CAGE_ASSERT_RUNTIME(minimum.min(maximum) == minimum); return min(maximum).max(minimum); } \
NAME clamp(real minimum, real maximum) const { CAGE_ASSERT_RUNTIME (minimum <= maximum, minimum, maximum); return min(maximum).max(minimum); } \
real distance(const NAME &other) const { return squaredDistance(other).sqrt(); } \
real squaredDistance(const NAME &other) const { return (*this - other).squaredLength(); } \
NAME &operator *= (const NAME &other) { return *this = *this * other; } \
NAME &operator /= (const NAME &other) { return *this = *this / other; } \
NAME operator * (const NAME &other) const { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = data[i] * other.data[i]; return r; } \
NAME operator / (const NAME &other) const { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = data[i] / other.data[i]; return r; } \
static const uint32 Dimension = N;

#define GCHL_GEN_QUATERNION_FUNCTIONS(NAME, N) \
inline NAME inverse(const NAME &th) { return th.inverse(); } \
inline NAME normalize(const NAME &th) { return th.normalize(); } \
inline real length(const NAME &th) { return th.length(); } \
inline real squaredLength(const NAME &th) { return th.squaredLength(); } \
inline real dot(const NAME &th, const NAME &other) { return th.dot(other); } \
inline bool valid(const NAME &th) { return th.valid(); } \
inline NAME operator + (real th, const NAME &other) { return other + th; } \
inline NAME operator - (real th, const NAME &other) { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = th - other.data[i]; return r; } \
inline NAME operator * (real th, const NAME &other) { return other * th; } \
inline NAME operator / (real th, const NAME &other) { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = th / other.data[i]; return r; }

#define GCHL_GEN_VECTOR_FUNCTIONS(NAME, N) \
GCHL_GEN_QUATERNION_FUNCTIONS(NAME, N) \
inline NAME min(const NAME &th, const NAME &other) { return th.min(other); } \
inline NAME max(const NAME &th, const NAME &other) { return th.max(other); } \
inline NAME min(const NAME &th, real other) { return th.min(other); } \
inline NAME max(const NAME &th, real other) { return th.max(other); } \
inline NAME min(real other, const NAME &th) { return th.min(other); } \
inline NAME max(real other, const NAME &th) { return th.max(other); } \
inline NAME abs(const NAME &th) { return th.abs(); } \
inline NAME clamp(const NAME &th, const NAME &minimum, const NAME &maximum) { return th.clamp(minimum, maximum); } \
inline NAME clamp(const NAME &th, real minimum, real maximum) { return th.clamp(minimum, maximum); } \
inline real distance(const NAME &th, const NAME &other) { return th.distance(other); } \
inline real squaredDistance(const NAME &th, const NAME &other) { return th.squaredDistance(other); }

namespace cage
{
	struct CAGE_API vec2
	{
		union
		{
			struct
			{
				real x;
				real y;
			};
			real data[2];
		};

		GCHL_GEN_VECTOR_BASE(vec2, 2);

		explicit vec2(real x, real y) { data[0] = x; data[1] = y; }
		explicit vec2(const vec3 &other);
		explicit vec2(const vec4 &other);

		real cross(const vec2 &other) const { return data[0] * other.data[1] - data[1] * other.data[0]; }
	};

	struct CAGE_API vec3
	{
		union
		{
			struct
			{
				real x;
				real y;
				real z;
			};
			real data[3];
		};

		GCHL_GEN_VECTOR_BASE(vec3, 3);

		explicit vec3(real x, real y, real z) { data[0] = x; data[1] = y; data[2] = z; }
		explicit vec3(const vec2 &other, real z);
		explicit vec3(const vec4 &other);

		vec3 &operator *= (const mat3 &other) { return *this = *this * other; }
		vec3 operator * (const quat &other) const;
		vec3 operator * (const mat3 &other) const;

		vec3 primaryAxis() const;

		vec3 cross(const vec3 &other) const { return vec3(data[1] * other.data[2] - data[2] * other.data[1], data[2] * other.data[0] - data[0] * other.data[2], data[0] * other.data[1] - data[1] * other.data[0]); }
	};

	struct CAGE_API vec4
	{
		union
		{
			struct
			{
				real x;
				real y;
				real z;
				real w;
			};
			real data[4];
		};

		GCHL_GEN_VECTOR_BASE(vec4, 4);

		explicit vec4(real x, real y, real z, real w) { data[0] = x; data[1] = y; data[2] = z; data[3] = w; }
		explicit vec4(const vec2 &v, real z, real w);
		explicit vec4(const vec2 &v, const vec2 &w);
		explicit vec4(const vec3 &v, real w);

		vec4 &operator *= (const mat4 &other) { return *this = *this * other; }
		vec4 operator * (const mat4 &other) const;
	};

	struct CAGE_API quat
	{
		union
		{
			struct
			{
				real x;
				real y;
				real z;
				real w;
			};
			real data[4];
		};

		GCHL_GEN_QUATERNION_BASE(quat, 4);

		quat();
		explicit quat(real x, real y, real z, real w);
		explicit quat(rads pitch, rads yaw, rads roll);
		explicit quat(const vec3 &axis, rads angle);
		explicit quat(const mat3 &rot);
		explicit quat(const vec3 &forward, const vec3 &up, bool keepUp = false);

		quat &operator *= (const quat &other) { return *this = *this * other; }
		vec3 operator * (const vec3 &other) const;
		quat operator * (const quat &other) const;
		transform operator * (const transform &other) const;

		quat conjugate() const { return quat(-data[0], -data[1], -data[2], data[3]); }
		void toAxisAngle(vec3 &axis, rads &angle) const;
	};

	GCHL_GEN_VECTOR_FUNCTIONS(vec2, 2);
	GCHL_GEN_VECTOR_FUNCTIONS(vec3, 3);
	GCHL_GEN_VECTOR_FUNCTIONS(vec4, 4);
	GCHL_GEN_QUATERNION_FUNCTIONS(quat, 4);

	inline real cross(const vec2 &th, const vec2 &other) { return th.cross(other); }
	inline vec3 cross(const vec3 &th, const vec3 &other) { return th.cross(other); }
	inline vec3 primaryAxis(vec3 &th) { return th.primaryAxis(); }
	inline quat conjugate(const quat &th) { return th.conjugate(); }
	CAGE_API quat lerp(const quat &min, const quat &max, real value);
	CAGE_API quat slerp(const quat &min, const quat &max, real value);
	CAGE_API quat rotate(const quat &from, const quat &toward, rads maxTurn);

	inline vec3 vec3::operator * (const quat &other) const { return other * *this; }

#define GCHL_GENERATE(TYPE) inline TYPE interpolate (const TYPE &min, const TYPE &max, real value) { return (TYPE)(min + (max - min) * value.value); }
	CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, vec2, vec3, vec4));
#undef GCHL_GENERATE
	inline quat interpolate(const quat &min, const quat &max, real value) { return slerp(min, max, value); }

	inline vec2::vec2(const vec3 &other) { data[0] = other.data[0]; data[1] = other.data[1]; }
	inline vec2::vec2(const vec4 &other) { data[0] = other.data[0]; data[1] = other.data[1]; }
	inline vec3::vec3(const vec2 &other, real z) { data[0] = other.data[0]; data[1] = other.data[1]; data[2] = z; }
	inline vec3::vec3(const vec4 &other) { data[0] = other.data[0]; data[1] = other.data[1]; data[2] = other.data[2]; }
	inline vec4::vec4(const vec2 &v, real z, real w) { data[0] = v.data[0]; data[1] = v.data[1]; data[2] = z; data[3] = w; }
	inline vec4::vec4(const vec2 &v, const vec2 &w) { data[0] = v.data[0]; data[1] = v.data[1]; data[2] = w.data[0]; data[3] = w.data[1]; }
	inline vec4::vec4(const vec3 &v, real w) { data[0] = v.data[0]; data[1] = v.data[1]; data[2] = v.data[2]; data[3] = w; }

	CAGE_API vec2 randomDirection2();
	CAGE_API vec3 randomDirection3();
	CAGE_API quat randomDirectionQuat();

	template<uint32 N> struct vecN {};
	template<> struct vecN<1> { typedef real type; };
	template<> struct vecN<2> { typedef vec2 type; };
	template<> struct vecN<3> { typedef vec3 type; };
	template<> struct vecN<4> { typedef vec4 type; };
}

#undef GCHL_GEN_QUATERNION_BASE
#undef GCHL_GEN_VECTOR_BASE
#undef GCHL_GEN_QUATERNION_FUNCTIONS
#undef GCHL_GEN_VECTOR_FUNCTIONS
