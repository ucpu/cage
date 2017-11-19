namespace cage
{
	struct CAGE_API vec2
	{
		GCHL_GEN_VECTOR_BASE(vec2, 2);

		explicit vec2(real x, real y) { data[0] = x; data[1] = y; }
		explicit vec2(const vec3 &other);
		explicit vec2(const vec4 &other);

		real cross(const vec2 &other) const { return data[0] * other.data[1] - data[1] * other.data[0]; }
	};

	struct CAGE_API vec3
	{
		GCHL_GEN_VECTOR_BASE(vec3, 3);

		explicit vec3(real x, real y, real z) { data[0] = x; data[1] = y; data[2] = z; }
		explicit vec3(const vec2 &other, real z);
		explicit vec3(const vec4 &other);

		vec3 &operator *= (const mat3 &other) { return *this = *this * other; }
		vec3 operator * (const quat &other) const;
		vec3 operator * (const mat3 &other) const;

		vec3 primaryAxis() const
		{
			if (data[0].abs() > data[1].abs() && data[0].abs() > data[2].abs())
				return vec3(data[0].sign(), 0, 0);
			if (data[1].abs() > data[2].abs())
				return vec3(0, data[1].sign(), 0);
			return vec3(0, 0, data[2].sign());
		}

		vec3 cross(const vec3 &other) const
		{
			return vec3(data[1] * other.data[2] - data[2] * other.data[1], data[2] * other.data[0] - data[0] * other.data[2], data[0] * other.data[1] - data[1] * other.data[0]);
		}
	};

	struct CAGE_API vec4
	{
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
}