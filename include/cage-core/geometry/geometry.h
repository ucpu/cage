namespace cage
{
	// point, line, triangle, plane, sphere, aabb

	CAGE_API bool parallel(const vec3 &dir1, const vec3 &dir2);
	CAGE_API bool parallel(const line &a, const line &b);
	CAGE_API bool parallel(const line &a, const triangle &b);
	CAGE_API bool parallel(const line &a, const plane &b);
	inline   bool parallel(const triangle &a, const line &b) { return parallel(b, a); }
	inline   bool parallel(const plane &a, const line &b) { return parallel(b, a); }
	CAGE_API bool parallel(const triangle &a, const triangle &b);
	CAGE_API bool parallel(const triangle &a, const plane &b);
	inline   bool parallel(const plane &a, const triangle &b) { return parallel(b, a); }
	CAGE_API bool parallel(const plane &a, const plane &b);

	CAGE_API bool perpendicular(const vec3 &dir1, const vec3 &dir2);
	CAGE_API bool perpendicular(const line &a, const line &b);
	CAGE_API bool perpendicular(const line &a, const triangle &b);
	CAGE_API bool perpendicular(const line &a, const plane &b);
	inline   bool perpendicular(const triangle &a, const line &b) { return perpendicular(b, a); }
	inline   bool perpendicular(const plane &a, const line &b) { return perpendicular(b, a); }
	CAGE_API bool perpendicular(const triangle &a, const triangle &b);
	CAGE_API bool perpendicular(const triangle &a, const plane &b);
	inline   bool perpendicular(const plane &a, const triangle &b) { return perpendicular(b, a); }
	CAGE_API bool perpendicular(const plane &a, const plane &b);

	//CAGE_API real distance(const vec3 &a, const vec3 &b);
	CAGE_API real distance(const vec3 &a, const line &b);
	CAGE_API real distance(const vec3 &a, const triangle &b);
	CAGE_API real distance(const vec3 &a, const plane &b);
	CAGE_API real distance(const vec3 &a, const sphere &b);
	CAGE_API real distance(const vec3 &a, const aabb &b);
	inline   real distance(const line &a, const vec3 &b) { return distance(b, a); };
	CAGE_API real distance(const line &a, const line &b);
	CAGE_API real distance(const line &a, const triangle &b);
	CAGE_API real distance(const line &a, const plane &b);
	CAGE_API real distance(const line &a, const sphere &b);
	CAGE_API real distance(const line &a, const aabb &b);
	inline   real distance(const triangle &a, const vec3 &b) { return distance(b, a); };
	inline   real distance(const triangle &a, const line &b) { return distance(b, a); };
	CAGE_API real distance(const triangle &a, const triangle &b);
	CAGE_API real distance(const triangle &a, const plane &b);
	CAGE_API real distance(const triangle &a, const sphere &b);
	CAGE_API real distance(const triangle &a, const aabb &b);
	inline   real distance(const plane &a, const vec3 &b) { return distance(b, a); };
	inline   real distance(const plane &a, const line &b) { return distance(b, a); };
	inline   real distance(const plane &a, const triangle &b) { return distance(b, a); };
	CAGE_API real distance(const plane &a, const plane &b);
	CAGE_API real distance(const plane &a, const sphere &b);
	CAGE_API real distance(const plane &a, const aabb &b);
	inline   real distance(const sphere &a, const vec3 &b) { return distance(b, a); };
	inline   real distance(const sphere &a, const line &b) { return distance(b, a); };
	inline   real distance(const sphere &a, const triangle &b) { return distance(b, a); };
	inline   real distance(const sphere &a, const plane &b) { return distance(b, a); };
	CAGE_API real distance(const sphere &a, const sphere &b);
	CAGE_API real distance(const sphere &a, const aabb &b);
	inline   real distance(const aabb &a, const vec3 &b) { return distance(b, a); };
	inline   real distance(const aabb &a, const line &b) { return distance(b, a); };
	inline   real distance(const aabb &a, const triangle &b) { return distance(b, a); };
	inline   real distance(const aabb &a, const plane &b) { return distance(b, a); };
	inline   real distance(const aabb &a, const sphere &b) { return distance(b, a); };
	CAGE_API real distance(const aabb &a, const aabb &b);

	CAGE_API bool intersects(const vec3 &a, const vec3 &b);
	CAGE_API bool intersects(const vec3 &a, const line &b);
	CAGE_API bool intersects(const vec3 &a, const triangle &b);
	CAGE_API bool intersects(const vec3 &a, const plane &b);
	CAGE_API bool intersects(const vec3 &a, const sphere &b);
	CAGE_API bool intersects(const vec3 &a, const aabb &b);
	inline   bool intersects(const line &a, const vec3 &b) { return intersects(b, a); };
	CAGE_API bool intersects(const line &a, const line &b);
	CAGE_API bool intersects(const line &a, const triangle &b);
	CAGE_API bool intersects(const line &a, const plane &b);
	CAGE_API bool intersects(const line &a, const sphere &b);
	CAGE_API bool intersects(const line &a, const aabb &b);
	inline   bool intersects(const triangle &a, const vec3 &b) { return intersects(b, a); };
	inline   bool intersects(const triangle &a, const line &b) { return intersects(b, a); };
	CAGE_API bool intersects(const triangle &a, const triangle &b);
	CAGE_API bool intersects(const triangle &a, const plane &b);
	CAGE_API bool intersects(const triangle &a, const sphere &b);
	CAGE_API bool intersects(const triangle &a, const aabb &b);
	inline   bool intersects(const plane &a, const vec3 &b) { return intersects(b, a); };
	inline   bool intersects(const plane &a, const line &b) { return intersects(b, a); };
	inline   bool intersects(const plane &a, const triangle &b) { return intersects(b, a); };
	CAGE_API bool intersects(const plane &a, const plane &b);
	CAGE_API bool intersects(const plane &a, const sphere &b);
	CAGE_API bool intersects(const plane &a, const aabb &b);
	inline   bool intersects(const sphere &a, const vec3 &b) { return intersects(b, a); };
	inline   bool intersects(const sphere &a, const line &b) { return intersects(b, a); };
	inline   bool intersects(const sphere &a, const triangle &b) { return intersects(b, a); };
	inline   bool intersects(const sphere &a, const plane &b) { return intersects(b, a); };
	CAGE_API bool intersects(const sphere &a, const sphere &b);
	CAGE_API bool intersects(const sphere &a, const aabb &b);
	inline   bool intersects(const aabb &a, const vec3 &b) { return intersects(b, a); };
	inline   bool intersects(const aabb &a, const line &b) { return intersects(b, a); };
	inline   bool intersects(const aabb &a, const triangle &b) { return intersects(b, a); };
	inline   bool intersects(const aabb &a, const plane &b) { return intersects(b, a); };
	inline   bool intersects(const aabb &a, const sphere &b) { return intersects(b, a); };
	CAGE_API bool intersects(const aabb &a, const aabb &b);

	CAGE_API vec3 intersection(const line &a, const triangle &b);
	CAGE_API vec3 intersection(const line &a, const plane &b);
	CAGE_API line intersection(const line &a, const sphere &b);
	CAGE_API line intersection(const line &a, const aabb &b);
	CAGE_API aabb intersection(const aabb &a, const aabb &b);
	inline   vec3 intersection(const triangle &a, const line &b) { return intersection(b, a); }
	inline   vec3 intersection(const plane &a, const line &b) { return intersection(b, a); }
	inline   line intersection(const sphere &a, const line &b) { return intersection(b, a); };
	inline   line intersection(const aabb &a, const line &b) { return intersection(b, a); };

	CAGE_API bool frustumCulling(const vec3 &shape, const mat4 &mvp);
	CAGE_API bool frustumCulling(const line &shape, const mat4 &mvp);
	CAGE_API bool frustumCulling(const triangle &shape, const mat4 &mvp);
	CAGE_API bool frustumCulling(const plane &shape, const mat4 &mvp);
	CAGE_API bool frustumCulling(const sphere &shape, const mat4 &mvp);
	CAGE_API bool frustumCulling(const aabb &shape, const mat4 &mvp);

	CAGE_API vec3 closestPoint(const triangle &trig, const vec3 &point);
	CAGE_API vec3 closestPoint(const plane &pl, const vec3 &point);
}
