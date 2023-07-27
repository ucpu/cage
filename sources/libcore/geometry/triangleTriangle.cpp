#include <math.h>

// taken from https://github.com/erich666/jgt-code/blob/master/Volume_08/Number_1/Guigue2003/tri_tri_intersect.c
// and modified

namespace
{
	int tri_tri_overlap_test_3d(float p1[3], float q1[3], float r1[3], float p2[3], float q2[3], float r2[3]);

	int tri_tri_intersection_test_3d(float p1[3], float q1[3], float r1[3], float p2[3], float q2[3], float r2[3], int *coplanar, float source[3], float target[3]);

	int coplanar_tri_tri3d(float p1[3], float q1[3], float r1[3], float p2[3], float q2[3], float r2[3], float N1[3]);

	int tri_tri_overlap_test_2d(float p1[2], float q1[2], float r1[2], float p2[2], float q2[2], float r2[2]);

#define CROSS(dest, v1, v2) \
	dest[0] = v1[1] * v2[2] - v1[2] * v2[1]; \
	dest[1] = v1[2] * v2[0] - v1[0] * v2[2]; \
	dest[2] = v1[0] * v2[1] - v1[1] * v2[0];

#define DOT(v1, v2) (v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2])

#define SUB(dest, v1, v2) \
	dest[0] = v1[0] - v2[0]; \
	dest[1] = v1[1] - v2[1]; \
	dest[2] = v1[2] - v2[2];

#define SCALAR(dest, alpha, v) \
	dest[0] = alpha * v[0]; \
	dest[1] = alpha * v[1]; \
	dest[2] = alpha * v[2];

#define CHECK_MIN_MAX(p1, q1, r1, p2, q2, r2) \
	{ \
		SUB(v1, p2, q1) \
		SUB(v2, p1, q1) \
		CROSS(N1, v1, v2) \
		SUB(v1, q2, q1) \
		if (DOT(v1, N1) > 0.0f) \
			return 0; \
		SUB(v1, p2, p1) \
		SUB(v2, r1, p1) \
		CROSS(N1, v1, v2) \
		SUB(v1, r2, p1) \
		if (DOT(v1, N1) > 0.0f) \
			return 0; \
		else \
			return 1; \
	}

#define TRI_TRI_3D(p1, q1, r1, p2, q2, r2, dp2, dq2, dr2) \
	{ \
		if (dp2 > 0.0f) \
		{ \
			if (dq2 > 0.0f) \
				CHECK_MIN_MAX(p1, r1, q1, r2, p2, q2) \
			else if (dr2 > 0.0f) \
				CHECK_MIN_MAX(p1, r1, q1, q2, r2, p2) \
			else \
				CHECK_MIN_MAX(p1, q1, r1, p2, q2, r2) \
		} \
		else if (dp2 < 0.0f) \
		{ \
			if (dq2 < 0.0f) \
				CHECK_MIN_MAX(p1, q1, r1, r2, p2, q2) \
			else if (dr2 < 0.0f) \
				CHECK_MIN_MAX(p1, q1, r1, q2, r2, p2) \
			else \
				CHECK_MIN_MAX(p1, r1, q1, p2, q2, r2) \
		} \
		else \
		{ \
			if (dq2 < 0.0f) \
			{ \
				if (dr2 >= 0.0f) \
					CHECK_MIN_MAX(p1, r1, q1, q2, r2, p2) \
				else \
					CHECK_MIN_MAX(p1, q1, r1, p2, q2, r2) \
			} \
			else if (dq2 > 0.0f) \
			{ \
				if (dr2 > 0.0f) \
					CHECK_MIN_MAX(p1, r1, q1, p2, q2, r2) \
				else \
					CHECK_MIN_MAX(p1, q1, r1, q2, r2, p2) \
			} \
			else \
			{ \
				if (dr2 > 0.0f) \
					CHECK_MIN_MAX(p1, q1, r1, r2, p2, q2) \
				else if (dr2 < 0.0f) \
					CHECK_MIN_MAX(p1, r1, q1, r2, p2, q2) \
				else \
					return coplanar_tri_tri3d(p1, q1, r1, p2, q2, r2, N1); \
			} \
		} \
	}

	int tri_tri_overlap_test_3d(float p1[3], float q1[3], float r1[3],

		float p2[3], float q2[3], float r2[3])
	{
		float dp1, dq1, dr1, dp2, dq2, dr2;
		float v1[3], v2[3];
		float N1[3], N2[3];

		SUB(v1, p2, r2)
		SUB(v2, q2, r2)
		CROSS(N2, v1, v2)

		SUB(v1, p1, r2)
		dp1 = DOT(v1, N2);
		SUB(v1, q1, r2)
		dq1 = DOT(v1, N2);
		SUB(v1, r1, r2)
		dr1 = DOT(v1, N2);

		if (((dp1 * dq1) > 0.0f) && ((dp1 * dr1) > 0.0f))
			return 0;

		SUB(v1, q1, p1)
		SUB(v2, r1, p1)
		CROSS(N1, v1, v2)

		SUB(v1, p2, r1)
		dp2 = DOT(v1, N1);
		SUB(v1, q2, r1)
		dq2 = DOT(v1, N1);
		SUB(v1, r2, r1)
		dr2 = DOT(v1, N1);

		if (((dp2 * dq2) > 0.0f) && ((dp2 * dr2) > 0.0f))
			return 0;

		if (dp1 > 0.0f)
		{
			if (dq1 > 0.0f)
				TRI_TRI_3D(r1, p1, q1, p2, r2, q2, dp2, dr2, dq2)
			else if (dr1 > 0.0f)
				TRI_TRI_3D(q1, r1, p1, p2, r2, q2, dp2, dr2, dq2)
			else
				TRI_TRI_3D(p1, q1, r1, p2, q2, r2, dp2, dq2, dr2)
		}
		else if (dp1 < 0.0f)
		{
			if (dq1 < 0.0f)
				TRI_TRI_3D(r1, p1, q1, p2, q2, r2, dp2, dq2, dr2)
			else if (dr1 < 0.0f)
				TRI_TRI_3D(q1, r1, p1, p2, q2, r2, dp2, dq2, dr2)
			else
				TRI_TRI_3D(p1, q1, r1, p2, r2, q2, dp2, dr2, dq2)
		}
		else
		{
			if (dq1 < 0.0f)
			{
				if (dr1 >= 0.0f)
					TRI_TRI_3D(q1, r1, p1, p2, r2, q2, dp2, dr2, dq2)
				else
					TRI_TRI_3D(p1, q1, r1, p2, q2, r2, dp2, dq2, dr2)
			}
			else if (dq1 > 0.0f)
			{
				if (dr1 > 0.0f)
					TRI_TRI_3D(p1, q1, r1, p2, r2, q2, dp2, dr2, dq2)
				else
					TRI_TRI_3D(q1, r1, p1, p2, q2, r2, dp2, dq2, dr2)
			}
			else
			{
				if (dr1 > 0.0f)
					TRI_TRI_3D(r1, p1, q1, p2, q2, r2, dp2, dq2, dr2)
				else if (dr1 < 0.0f)
					TRI_TRI_3D(r1, p1, q1, p2, r2, q2, dp2, dr2, dq2)
				else
					return coplanar_tri_tri3d(p1, q1, r1, p2, q2, r2, N1);
			}
		}
	};

	int coplanar_tri_tri3d(float p1[3], float q1[3], float r1[3], float p2[3], float q2[3], float r2[3], float normal_1[3])
	{

		float P1[2], Q1[2], R1[2];
		float P2[2], Q2[2], R2[2];

		float n_x, n_y, n_z;

		n_x = ((normal_1[0] < 0) ? -normal_1[0] : normal_1[0]);
		n_y = ((normal_1[1] < 0) ? -normal_1[1] : normal_1[1]);
		n_z = ((normal_1[2] < 0) ? -normal_1[2] : normal_1[2]);

		if ((n_x > n_z) && (n_x >= n_y))
		{
			P1[0] = q1[2];
			P1[1] = q1[1];
			Q1[0] = p1[2];
			Q1[1] = p1[1];
			R1[0] = r1[2];
			R1[1] = r1[1];

			P2[0] = q2[2];
			P2[1] = q2[1];
			Q2[0] = p2[2];
			Q2[1] = p2[1];
			R2[0] = r2[2];
			R2[1] = r2[1];
		}
		else if ((n_y > n_z) && (n_y >= n_x))
		{
			P1[0] = q1[0];
			P1[1] = q1[2];
			Q1[0] = p1[0];
			Q1[1] = p1[2];
			R1[0] = r1[0];
			R1[1] = r1[2];

			P2[0] = q2[0];
			P2[1] = q2[2];
			Q2[0] = p2[0];
			Q2[1] = p2[2];
			R2[0] = r2[0];
			R2[1] = r2[2];
		}
		else
		{
			P1[0] = p1[0];
			P1[1] = p1[1];
			Q1[0] = q1[0];
			Q1[1] = q1[1];
			R1[0] = r1[0];
			R1[1] = r1[1];

			P2[0] = p2[0];
			P2[1] = p2[1];
			Q2[0] = q2[0];
			Q2[1] = q2[1];
			R2[0] = r2[0];
			R2[1] = r2[1];
		}

		return tri_tri_overlap_test_2d(P1, Q1, R1, P2, Q2, R2);
	};

#define CONSTRUCT_INTERSECTION(p1, q1, r1, p2, q2, r2) \
	{ \
		SUB(v1, q1, p1) \
		SUB(v2, r2, p1) \
		CROSS(N, v1, v2) \
		SUB(v, p2, p1) \
		if (DOT(v, N) > 0.0f) \
		{ \
			SUB(v1, r1, p1) \
			CROSS(N, v1, v2) \
			if (DOT(v, N) <= 0.0f) \
			{ \
				SUB(v2, q2, p1) \
				CROSS(N, v1, v2) \
				if (DOT(v, N) > 0.0f) \
				{ \
					SUB(v1, p1, p2) \
					SUB(v2, p1, r1) \
					alpha = DOT(v1, N2) / DOT(v2, N2); \
					SCALAR(v1, alpha, v2) \
					SUB(source, p1, v1) \
					SUB(v1, p2, p1) \
					SUB(v2, p2, r2) \
					alpha = DOT(v1, N1) / DOT(v2, N1); \
					SCALAR(v1, alpha, v2) \
					SUB(target, p2, v1) \
					return 1; \
				} \
				else \
				{ \
					SUB(v1, p2, p1) \
					SUB(v2, p2, q2) \
					alpha = DOT(v1, N1) / DOT(v2, N1); \
					SCALAR(v1, alpha, v2) \
					SUB(source, p2, v1) \
					SUB(v1, p2, p1) \
					SUB(v2, p2, r2) \
					alpha = DOT(v1, N1) / DOT(v2, N1); \
					SCALAR(v1, alpha, v2) \
					SUB(target, p2, v1) \
					return 1; \
				} \
			} \
			else \
			{ \
				return 0; \
			} \
		} \
		else \
		{ \
			SUB(v2, q2, p1) \
			CROSS(N, v1, v2) \
			if (DOT(v, N) < 0.0f) \
			{ \
				return 0; \
			} \
			else \
			{ \
				SUB(v1, r1, p1) \
				CROSS(N, v1, v2) \
				if (DOT(v, N) >= 0.0f) \
				{ \
					SUB(v1, p1, p2) \
					SUB(v2, p1, r1) \
					alpha = DOT(v1, N2) / DOT(v2, N2); \
					SCALAR(v1, alpha, v2) \
					SUB(source, p1, v1) \
					SUB(v1, p1, p2) \
					SUB(v2, p1, q1) \
					alpha = DOT(v1, N2) / DOT(v2, N2); \
					SCALAR(v1, alpha, v2) \
					SUB(target, p1, v1) \
					return 1; \
				} \
				else \
				{ \
					SUB(v1, p2, p1) \
					SUB(v2, p2, q2) \
					alpha = DOT(v1, N1) / DOT(v2, N1); \
					SCALAR(v1, alpha, v2) \
					SUB(source, p2, v1) \
					SUB(v1, p1, p2) \
					SUB(v2, p1, q1) \
					alpha = DOT(v1, N2) / DOT(v2, N2); \
					SCALAR(v1, alpha, v2) \
					SUB(target, p1, v1) \
					return 1; \
				} \
			} \
		} \
	}

#define TRI_TRI_INTER_3D(p1, q1, r1, p2, q2, r2, dp2, dq2, dr2) \
	{ \
		if (dp2 > 0.0f) \
		{ \
			if (dq2 > 0.0f) \
				CONSTRUCT_INTERSECTION(p1, r1, q1, r2, p2, q2) \
			else if (dr2 > 0.0f) \
				CONSTRUCT_INTERSECTION(p1, r1, q1, q2, r2, p2) \
			else \
				CONSTRUCT_INTERSECTION(p1, q1, r1, p2, q2, r2) \
		} \
		else if (dp2 < 0.0f) \
		{ \
			if (dq2 < 0.0f) \
				CONSTRUCT_INTERSECTION(p1, q1, r1, r2, p2, q2) \
			else if (dr2 < 0.0f) \
				CONSTRUCT_INTERSECTION(p1, q1, r1, q2, r2, p2) \
			else \
				CONSTRUCT_INTERSECTION(p1, r1, q1, p2, q2, r2) \
		} \
		else \
		{ \
			if (dq2 < 0.0f) \
			{ \
				if (dr2 >= 0.0f) \
					CONSTRUCT_INTERSECTION(p1, r1, q1, q2, r2, p2) \
				else \
					CONSTRUCT_INTERSECTION(p1, q1, r1, p2, q2, r2) \
			} \
			else if (dq2 > 0.0f) \
			{ \
				if (dr2 > 0.0f) \
					CONSTRUCT_INTERSECTION(p1, r1, q1, p2, q2, r2) \
				else \
					CONSTRUCT_INTERSECTION(p1, q1, r1, q2, r2, p2) \
			} \
			else \
			{ \
				if (dr2 > 0.0f) \
					CONSTRUCT_INTERSECTION(p1, q1, r1, r2, p2, q2) \
				else if (dr2 < 0.0f) \
					CONSTRUCT_INTERSECTION(p1, r1, q1, r2, p2, q2) \
				else \
				{ \
					*coplanar = 1; \
					return coplanar_tri_tri3d(p1, q1, r1, p2, q2, r2, N1); \
				} \
			} \
		} \
	}

	int tri_tri_intersection_test_3d(float p1[3], float q1[3], float r1[3], float p2[3], float q2[3], float r2[3], int *coplanar, float source[3], float target[3])

	{
		float dp1, dq1, dr1, dp2, dq2, dr2;
		float v1[3], v2[3], v[3];
		float N1[3], N2[3], N[3];
		float alpha;

		SUB(v1, p2, r2)
		SUB(v2, q2, r2)
		CROSS(N2, v1, v2)

		SUB(v1, p1, r2)
		dp1 = DOT(v1, N2);
		SUB(v1, q1, r2)
		dq1 = DOT(v1, N2);
		SUB(v1, r1, r2)
		dr1 = DOT(v1, N2);

		if (((dp1 * dq1) > 0.0f) && ((dp1 * dr1) > 0.0f))
			return 0;

		SUB(v1, q1, p1)
		SUB(v2, r1, p1)
		CROSS(N1, v1, v2)

		SUB(v1, p2, r1)
		dp2 = DOT(v1, N1);
		SUB(v1, q2, r1)
		dq2 = DOT(v1, N1);
		SUB(v1, r2, r1)
		dr2 = DOT(v1, N1);

		if (((dp2 * dq2) > 0.0f) && ((dp2 * dr2) > 0.0f))
			return 0;

		if (dp1 > 0.0f)
		{
			if (dq1 > 0.0f)
				TRI_TRI_INTER_3D(r1, p1, q1, p2, r2, q2, dp2, dr2, dq2)
			else if (dr1 > 0.0f)
				TRI_TRI_INTER_3D(q1, r1, p1, p2, r2, q2, dp2, dr2, dq2)

			else
				TRI_TRI_INTER_3D(p1, q1, r1, p2, q2, r2, dp2, dq2, dr2)
		}
		else if (dp1 < 0.0f)
		{
			if (dq1 < 0.0f)
				TRI_TRI_INTER_3D(r1, p1, q1, p2, q2, r2, dp2, dq2, dr2)
			else if (dr1 < 0.0f)
				TRI_TRI_INTER_3D(q1, r1, p1, p2, q2, r2, dp2, dq2, dr2)
			else
				TRI_TRI_INTER_3D(p1, q1, r1, p2, r2, q2, dp2, dr2, dq2)
		}
		else
		{
			if (dq1 < 0.0f)
			{
				if (dr1 >= 0.0f)
					TRI_TRI_INTER_3D(q1, r1, p1, p2, r2, q2, dp2, dr2, dq2)
				else
					TRI_TRI_INTER_3D(p1, q1, r1, p2, q2, r2, dp2, dq2, dr2)
			}
			else if (dq1 > 0.0f)
			{
				if (dr1 > 0.0f)
					TRI_TRI_INTER_3D(p1, q1, r1, p2, r2, q2, dp2, dr2, dq2)
				else
					TRI_TRI_INTER_3D(q1, r1, p1, p2, q2, r2, dp2, dq2, dr2)
			}
			else
			{
				if (dr1 > 0.0f)
					TRI_TRI_INTER_3D(r1, p1, q1, p2, q2, r2, dp2, dq2, dr2)
				else if (dr1 < 0.0f)
					TRI_TRI_INTER_3D(r1, p1, q1, p2, r2, q2, dp2, dr2, dq2)
				else
				{
					// triangles are co-planar

					*coplanar = 1;
					return coplanar_tri_tri3d(p1, q1, r1, p2, q2, r2, N1);
				}
			}
		}
	};

#define ORIENT_2D(a, b, c) ((a[0] - c[0]) * (b[1] - c[1]) - (a[1] - c[1]) * (b[0] - c[0]))

#define INTERSECTION_TEST_VERTEX(P1, Q1, R1, P2, Q2, R2) \
	{ \
		if (ORIENT_2D(R2, P2, Q1) >= 0.0f) \
			if (ORIENT_2D(R2, Q2, Q1) <= 0.0f) \
				if (ORIENT_2D(P1, P2, Q1) > 0.0f) \
				{ \
					if (ORIENT_2D(P1, Q2, Q1) <= 0.0f) \
						return 1; \
					else \
						return 0; \
				} \
				else \
				{ \
					if (ORIENT_2D(P1, P2, R1) >= 0.0f) \
						if (ORIENT_2D(Q1, R1, P2) >= 0.0f) \
							return 1; \
						else \
							return 0; \
					else \
						return 0; \
				} \
			else if (ORIENT_2D(P1, Q2, Q1) <= 0.0f) \
				if (ORIENT_2D(R2, Q2, R1) <= 0.0f) \
					if (ORIENT_2D(Q1, R1, Q2) >= 0.0f) \
						return 1; \
					else \
						return 0; \
				else \
					return 0; \
			else \
				return 0; \
		else if (ORIENT_2D(R2, P2, R1) >= 0.0f) \
			if (ORIENT_2D(Q1, R1, R2) >= 0.0f) \
				if (ORIENT_2D(P1, P2, R1) >= 0.0f) \
					return 1; \
				else \
					return 0; \
			else if (ORIENT_2D(Q1, R1, Q2) >= 0.0f) \
			{ \
				if (ORIENT_2D(R2, R1, Q2) >= 0.0f) \
					return 1; \
				else \
					return 0; \
			} \
			else \
				return 0; \
		else \
			return 0; \
	};

#define INTERSECTION_TEST_EDGE(P1, Q1, R1, P2, Q2, R2) \
	{ \
		if (ORIENT_2D(R2, P2, Q1) >= 0.0f) \
		{ \
			if (ORIENT_2D(P1, P2, Q1) >= 0.0f) \
			{ \
				if (ORIENT_2D(P1, Q1, R2) >= 0.0f) \
					return 1; \
				else \
					return 0; \
			} \
			else \
			{ \
				if (ORIENT_2D(Q1, R1, P2) >= 0.0f) \
				{ \
					if (ORIENT_2D(R1, P1, P2) >= 0.0f) \
						return 1; \
					else \
						return 0; \
				} \
				else \
					return 0; \
			} \
		} \
		else \
		{ \
			if (ORIENT_2D(R2, P2, R1) >= 0.0f) \
			{ \
				if (ORIENT_2D(P1, P2, R1) >= 0.0f) \
				{ \
					if (ORIENT_2D(P1, R1, R2) >= 0.0f) \
						return 1; \
					else \
					{ \
						if (ORIENT_2D(Q1, R1, R2) >= 0.0f) \
							return 1; \
						else \
							return 0; \
					} \
				} \
				else \
					return 0; \
			} \
			else \
				return 0; \
		} \
	}

	int ccw_tri_tri_intersection_2d(float p1[2], float q1[2], float r1[2], float p2[2], float q2[2], float r2[2])
	{
		if (ORIENT_2D(p2, q2, p1) >= 0.0f)
		{
			if (ORIENT_2D(q2, r2, p1) >= 0.0f)
			{
				if (ORIENT_2D(r2, p2, p1) >= 0.0f)
					return 1;
				else
					INTERSECTION_TEST_EDGE(p1, q1, r1, p2, q2, r2)
			}
			else
			{
				if (ORIENT_2D(r2, p2, p1) >= 0.0f)
					INTERSECTION_TEST_EDGE(p1, q1, r1, r2, p2, q2)
				else
					INTERSECTION_TEST_VERTEX(p1, q1, r1, p2, q2, r2)
			}
		}
		else
		{
			if (ORIENT_2D(q2, r2, p1) >= 0.0f)
			{
				if (ORIENT_2D(r2, p2, p1) >= 0.0f)
					INTERSECTION_TEST_EDGE(p1, q1, r1, q2, r2, p2)
				else
					INTERSECTION_TEST_VERTEX(p1, q1, r1, q2, r2, p2)
			}
			else
				INTERSECTION_TEST_VERTEX(p1, q1, r1, r2, p2, q2)
		}
	};

	int tri_tri_overlap_test_2d(float p1[2], float q1[2], float r1[2], float p2[2], float q2[2], float r2[2])
	{
		if (ORIENT_2D(p1, q1, r1) < 0.0f)
			if (ORIENT_2D(p2, q2, r2) < 0.0f)
				return ccw_tri_tri_intersection_2d(p1, r1, q1, p2, r2, q2);
			else
				return ccw_tri_tri_intersection_2d(p1, r1, q1, p2, q2, r2);
		else if (ORIENT_2D(p2, q2, r2) < 0.0f)
			return ccw_tri_tri_intersection_2d(p1, q1, r1, p2, r2, q2);
		else
			return ccw_tri_tri_intersection_2d(p1, q1, r1, p2, q2, r2);
	};
}

#include <cage-core/geometry.h>

namespace cage
{
	bool intersects(const Triangle &a, const Triangle &b)
	{
		return tri_tri_overlap_test_3d((float *)a[0].data, (float *)a[1].data, (float *)a[2].data, (float *)b[0].data, (float *)b[1].data, (float *)b[2].data);
	}
}
