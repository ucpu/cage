#include <cage-core/geometry.h>

#include <array>

namespace cage
{
	namespace
	{
		// https://www.geometrictools.com/GTE/Mathematics/IntrAlignedBox3Cone3.h modified

		enum
		{
			NUM_BOX_VERTICES = 8,
			NUM_BOX_EDGES = 12,
			NUM_BOX_FACES = 6,
			MAX_VERTICES = 32,
			VERTEX_MIN_BASE = 8,
			VERTEX_MAX_BASE = 20,
			MAX_CANDIDATE_EDGES = 496,
			NUM_CONFIGURATIONS = 81
		};

		struct Face
		{
			std::array<uint16, 4> v, e;
		};

		class TIQuery;
		typedef void (TIQuery:: *ConfigurationFunction)(uint16, const Face &);

		class TIQuery
		{
		public:
			TIQuery()
			{
				std::array<uint16, 2> ezero = { 0, 0 };
				mCandidateEdges.fill(ezero);
				for (uint16 r = 0; r < MAX_VERTICES; ++r)
					mAdjacencyMatrix[r].fill(0);
			}

			bool operator()(Aabb const &box, Cone const &cone)
			{
				real boxMinHeight(0), boxMaxHeight(0);
				ComputeBoxHeightInterval(box, cone, boxMinHeight, boxMaxHeight);
				if (boxMinHeight >= cone.length)
					return false;
				if (ConeAxisIntersectsBox(box, cone))
					return true;
				if (BoxFullyInConeSlab(box, boxMinHeight, boxMaxHeight, cone))
					return CandidatesHavePointInsideCone(cone);
				ClearCandidates();
				ComputeCandidatesOnBoxEdges(cone);
				ComputeCandidatesOnBoxFaces();
				return CandidatesHavePointInsideCone(cone);
			}

			std::array<std::array<uint16, 2>, MAX_CANDIDATE_EDGES> mCandidateEdges;
			std::array<std::array<uint16, MAX_VERTICES>, MAX_VERTICES> mAdjacencyMatrix;
			std::array<vec3, MAX_VERTICES> mVertices;
			std::array<real, NUM_BOX_VERTICES> mProjectionMin, mProjectionMax;
			uint16 mNumCandidateEdges = 0;

			static void ComputeBoxHeightInterval(Aabb const &box, Cone const &cone, real &boxMinHeight, real &boxMaxHeight)
			{
				vec3 C = box.center(), e = box.size() / 2;
				vec3 const &V = cone.origin;
				vec3 const &U = cone.direction;
				vec3 CmV = C - V;
				real DdCmV = dot(U, CmV);
				real radius = e[0] * abs(U[0]) + e[1] * abs(U[1]) + e[2] * abs(U[2]);
				boxMinHeight = DdCmV - radius;
				boxMaxHeight = DdCmV + radius;
			}

			static bool ConeAxisIntersectsBox(Aabb const &box, Cone const &cone)
			{
				return intersects(box, makeSegment(cone.origin, cone.origin + cone.direction * cone.length));
			}

			static bool HasPointInsideCone(vec3 const &P0, vec3 const &P1, Cone const &cone)
			{
				const real coneCosAngle = cos(cone.halfAngle);
				vec3 const &U = cone.direction;
				real g = dot(U, P0) - coneCosAngle * length(P0);
				if (g > (real)0)
					return true;
				g = dot(U, P1) - coneCosAngle * length(P1);
				if (g > (real)0)
					return true;
				vec3 E = P1 - P0;
				vec3 crossP0U = cross(P0, U);
				vec3 crossP0E = cross(P0, E);
				real dphi0 = dot(crossP0E, crossP0U);
				if (dphi0 > (real)0)
				{
					vec3 crossP1U = cross(P1, U);
					real dphi1 = dot(crossP0E, crossP1U);
					if (dphi1 < (real)0)
					{
						real t = dphi0 / (dphi0 - dphi1);
						vec3 PMax = P0 + t * E;
						g = dot(U, PMax) - coneCosAngle * length(PMax);
						if (g > (real)0)
							return true;
					}
				}
				return false;
			}

			bool BoxFullyInConeSlab(Aabb const &box, real boxMinHeight, real boxMaxHeight, Cone const &cone);

			bool CandidatesHavePointInsideCone(Cone const &cone) const
			{
				for (uint16 i = 0; i < mNumCandidateEdges; ++i)
				{
					auto const &edge = mCandidateEdges[i];
					vec3 const &P0 = mVertices[edge[0]];
					vec3 const &P1 = mVertices[edge[1]];
					if (HasPointInsideCone(P0, P1, cone))
						return true;
				}
				return false;
			}

			void ComputeCandidatesOnBoxEdges(Cone const &cone);

			void ComputeCandidatesOnBoxFaces();

			void ClearCandidates()
			{
				for (uint16 i = 0; i < mNumCandidateEdges; ++i)
				{
					auto const &edge = mCandidateEdges[i];
					mAdjacencyMatrix[edge[0]][edge[1]] = 0;
					mAdjacencyMatrix[edge[1]][edge[0]] = 0;
				}
				mNumCandidateEdges = 0;
			}

			void InsertEdge(uint16 v0, uint16 v1)
			{
				if (mAdjacencyMatrix[v0][v1] == 0)
				{
					mAdjacencyMatrix[v0][v1] = 1;
					mAdjacencyMatrix[v1][v0] = 1;
					mCandidateEdges[mNumCandidateEdges] = { v0, v1 };
					++mNumCandidateEdges;
				}
			}

			void NNNN_0(uint16, Face const &)
			{
			}

			void NNNP_2(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[2], base + face.e[3]);
			}

			void NNZP_5(uint16 base, Face const &face)
			{
				InsertEdge(face.v[2], base + face.e[3]);
			}

			void NNPN_6(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[1], base + face.e[2]);
			}

			void NNPZ_7(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[1], face.v[3]);
			}

			void NNPP_8(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[1], base + face.e[3]);
			}

			void NZNP_11(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[2], face.v[3]);
				InsertEdge(base + face.e[3], face.v[3]);
			}

			void NZZP_14(uint16 base, Face const &face)
			{
				InsertEdge(face.v[2], face.v[3]);
				InsertEdge(base + face.e[3], face.v[3]);
			}

			void NZPN_15(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[2], face.v[1]);
			}

			void NZPZ_16(uint16, Face const &face)
			{
				InsertEdge(face.v[1], face.v[3]);
			}

			void NZPP_17(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[3], face.v[1]);
			}

			void NPNN_18(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[0], base + face.e[1]);
			}

			void NPNZ_19(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[0], face.v[1]);
				InsertEdge(base + face.e[1], face.v[1]);
			}

			void NPNP_20(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[0], face.v[1]);
				InsertEdge(base + face.e[1], face.v[1]);
				InsertEdge(base + face.e[2], face.v[3]);
				InsertEdge(base + face.e[3], face.v[3]);
			}

			void NPZN_21(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[0], face.v[2]);
			}

			void NPZZ_22(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[0], face.v[1]);
				InsertEdge(face.v[1], face.v[2]);
			}

			void NPZP_23(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[0], face.v[1]);
				InsertEdge(face.v[1], face.v[2]);
				InsertEdge(base + face.e[3], face.v[2]);
				InsertEdge(face.v[2], face.v[3]);
			}

			void NPPN_24(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[0], base + face.e[2]);
			}

			void NPPZ_25(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[0], face.v[3]);
			}

			void NPPP_26(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[0], base + face.e[3]);
			}

			void ZNNP_29(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[2], face.v[0]);
			}

			void ZNZP_32(uint16, Face const &face)
			{
				InsertEdge(face.v[0], face.v[2]);
			}

			void ZNPN_33(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[1], face.v[2]);
				InsertEdge(base + face.e[2], face.v[2]);
			}

			void ZNPZ_34(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[1], face.v[2]);
				InsertEdge(face.v[2], face.v[3]);
			}

			void ZNPP_35(uint16 base, Face const &face)
			{
				InsertEdge(face.v[0], base + face.e[1]);
			}

			void ZZNP_38(uint16 base, Face const &face)
			{
				InsertEdge(face.v[0], face.v[3]);
				InsertEdge(face.v[3], base + face.e[2]);
			}

			void ZZZP_41(uint16, Face const &face)
			{
				InsertEdge(face.v[0], face.v[3]);
				InsertEdge(face.v[3], face.v[2]);
			}

			void ZZPN_42(uint16 base, Face const &face)
			{
				InsertEdge(face.v[1], face.v[2]);
				InsertEdge(face.v[2], base + face.e[2]);
			}

			void ZZPZ_43(uint16, Face const &face)
			{
				InsertEdge(face.v[1], face.v[2]);
				InsertEdge(face.v[2], face.v[3]);
			}

			void ZPNN_45(uint16 base, Face const &face)
			{
				InsertEdge(face.v[0], base + face.e[1]);
			}

			void ZPNZ_46(uint16 base, Face const &face)
			{
				InsertEdge(face.v[0], face.v[1]);
				InsertEdge(face.v[1], base + face.e[1]);
			}

			void ZPNP_47(uint16 base, Face const &face)
			{
				InsertEdge(face.v[0], face.v[1]);
				InsertEdge(face.v[1], base + face.e[1]);
				InsertEdge(base + face.e[2], face.v[3]);
				InsertEdge(face.v[3], face.v[0]);
			}

			void ZPZN_48(uint16, Face const &face)
			{
				InsertEdge(face.v[0], face.v[2]);
			}

			void ZPZZ_49(uint16, Face const &face)
			{
				InsertEdge(face.v[0], face.v[1]);
				InsertEdge(face.v[1], face.v[2]);
			}

			void ZPPN_51(uint16 base, Face const &face)
			{
				InsertEdge(face.v[0], base + face.e[2]);
			}

			void PNNN_54(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[3], base + face.e[0]);
			}

			void PNNZ_55(uint16 base, Face const &face)
			{
				InsertEdge(face.v[3], base + face.e[0]);
			}

			void PNNP_56(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[2], base + face.e[0]);
			}

			void PNZN_57(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[3], face.v[0]);
				InsertEdge(face.v[0], base + face.e[0]);
			}

			void PNZZ_58(uint16 base, Face const &face)
			{
				InsertEdge(face.v[3], face.v[0]);
				InsertEdge(face.v[0], base + face.e[0]);
			}

			void PNZP_59(uint16 base, Face const &face)
			{
				InsertEdge(face.v[2], base + face.e[0]);
			}

			void PNPN_60(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[3], face.v[0]);
				InsertEdge(face.v[0], base + face.e[0]);
				InsertEdge(base + face.e[1], face.v[2]);
				InsertEdge(face.v[2], base + face.e[2]);
			}

			void PNPZ_61(uint16 base, Face const &face)
			{
				InsertEdge(face.v[3], face.v[0]);
				InsertEdge(face.v[0], base + face.e[0]);
				InsertEdge(base + face.e[1], face.v[2]);
				InsertEdge(face.v[2], face.v[3]);
			}

			void PNPP_62(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[0], base + face.e[1]);
			}

			void PZNN_63(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[3], face.v[1]);
			}

			void PZNZ_64(uint16, Face const &face)
			{
				InsertEdge(face.v[3], face.v[1]);
			}

			void PZNP_65(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[2], face.v[1]);
			}

			void PZZN_66(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[3], face.v[0]);
				InsertEdge(face.v[0], face.v[1]);
			}

			void PZPN_69(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[3], face.v[0]);
				InsertEdge(face.v[0], face.v[1]);
				InsertEdge(face.v[1], face.v[2]);
				InsertEdge(face.v[2], base + face.e[2]);
			}

			void PPNN_72(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[3], base + face.e[1]);
			}

			void PPNZ_73(uint16 base, Face const &face)
			{
				InsertEdge(face.v[3], base + face.e[1]);
			}

			void PPNP_74(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[2], base + face.e[1]);
			}

			void PPZN_75(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[2], face.v[2]);
			}

			void PPPN_78(uint16 base, Face const &face)
			{
				InsertEdge(base + face.e[3], base + face.e[2]);
			}
		};

		constexpr std::array<ConfigurationFunction, NUM_CONFIGURATIONS> initConfiguration()
		{
			return {
				&TIQuery::NNNN_0,
				&TIQuery::NNNN_0,
				&TIQuery::NNNP_2,
				&TIQuery::NNNN_0,
				&TIQuery::NNNN_0,
				&TIQuery::NNZP_5,
				&TIQuery::NNPN_6,
				&TIQuery::NNPZ_7,
				&TIQuery::NNPP_8,
				&TIQuery::NNNN_0,
				&TIQuery::NNNN_0,
				&TIQuery::NZNP_11,
				&TIQuery::NNNN_0,
				&TIQuery::NNNN_0,
				&TIQuery::NZZP_14,
				&TIQuery::NZPN_15,
				&TIQuery::NZPZ_16,
				&TIQuery::NZPP_17,
				&TIQuery::NPNN_18,
				&TIQuery::NPNZ_19,
				&TIQuery::NPNP_20,
				&TIQuery::NPZN_21,
				&TIQuery::NPZZ_22,
				&TIQuery::NPZP_23,
				&TIQuery::NPPN_24,
				&TIQuery::NPPZ_25,
				&TIQuery::NPPP_26,
				&TIQuery::NNNN_0,
				&TIQuery::NNNN_0,
				&TIQuery::ZNNP_29,
				&TIQuery::NNNN_0,
				&TIQuery::NNNN_0,
				&TIQuery::ZNZP_32,
				&TIQuery::ZNPN_33,
				&TIQuery::ZNPZ_34,
				&TIQuery::ZNPP_35,
				&TIQuery::NNNN_0,
				&TIQuery::NNNN_0,
				&TIQuery::ZZNP_38,
				&TIQuery::NNNN_0,
				&TIQuery::NNNN_0,
				&TIQuery::ZZZP_41,
				&TIQuery::ZZPN_42,
				&TIQuery::ZZPZ_43,
				&TIQuery::NNNN_0,
				&TIQuery::ZPNN_45,
				&TIQuery::ZPNZ_46,
				&TIQuery::ZPNP_47,
				&TIQuery::ZPZN_48,
				&TIQuery::ZPZZ_49,
				&TIQuery::NNNN_0,
				&TIQuery::ZPPN_51,
				&TIQuery::NNNN_0,
				&TIQuery::NNNN_0,
				&TIQuery::PNNN_54,
				&TIQuery::PNNZ_55,
				&TIQuery::PNNP_56,
				&TIQuery::PNZN_57,
				&TIQuery::PNZZ_58,
				&TIQuery::PNZP_59,
				&TIQuery::PNPN_60,
				&TIQuery::PNPZ_61,
				&TIQuery::PNPP_62,
				&TIQuery::PZNN_63,
				&TIQuery::PZNZ_64,
				&TIQuery::PZNP_65,
				&TIQuery::PZZN_66,
				&TIQuery::NNNN_0,
				&TIQuery::NNNN_0,
				&TIQuery::PZPN_69,
				&TIQuery::NNNN_0,
				&TIQuery::NNNN_0,
				&TIQuery::PPNN_72,
				&TIQuery::PPNZ_73,
				&TIQuery::PPNP_74,
				&TIQuery::PPZN_75,
				&TIQuery::NNNN_0,
				&TIQuery::NNNN_0,
				&TIQuery::PPPN_78,
				&TIQuery::NNNN_0,
				&TIQuery::NNNN_0,
			};
		}
		constexpr const std::array<ConfigurationFunction, NUM_CONFIGURATIONS> mConfiguration = initConfiguration();

		constexpr std::array<std::array<uint16, 2>, NUM_BOX_EDGES> initEdges()
		{
			return {
				std::array<uint16, 2>{ 0, 1 },
				std::array<uint16, 2>{ 1, 3 },
				std::array<uint16, 2>{ 2, 3 },
				std::array<uint16, 2>{ 0, 2 },
				std::array<uint16, 2>{ 4, 5 },
				std::array<uint16, 2>{ 5, 7 },
				std::array<uint16, 2>{ 6, 7 },
				std::array<uint16, 2>{ 4, 6 },
				std::array<uint16, 2>{ 0, 4 },
				std::array<uint16, 2>{ 1, 5 },
				std::array<uint16, 2>{ 3, 7 },
				std::array<uint16, 2>{ 2, 6 }
			};
		}
		constexpr const std::array<std::array<uint16, 2>, NUM_BOX_EDGES> mEdges = initEdges();

		constexpr std::array<Face, NUM_BOX_FACES> initFaces()
		{
			return {
				Face{ { 0, 4, 6, 2 }, {  8,  7, 11,  3 } },
				Face{ { 1, 3, 7, 5 }, {  1, 10,  5,  9 } },
				Face{ { 0, 1, 5, 4 }, {  0,  9,  4,  8 } },
				Face{ { 2, 6, 7, 3 }, { 11,  6, 10,  2 } },
				Face{ { 0, 2, 3, 1 }, {  3,  2,  1,  0 } },
				Face{ { 4, 5, 7, 6 }, {  4,  5,  6,  7 } },
			};
		}
		constexpr const std::array<Face, NUM_BOX_FACES> mFaces = initFaces();

		bool TIQuery::BoxFullyInConeSlab(Aabb const &box, real boxMinHeight, real boxMaxHeight, Cone const &cone)
		{
			mVertices[0] = vec3(box.a[0], box.a[1], box.a[2]);
			mVertices[1] = vec3(box.b[0], box.a[1], box.a[2]);
			mVertices[2] = vec3(box.a[0], box.b[1], box.a[2]);
			mVertices[3] = vec3(box.b[0], box.b[1], box.a[2]);
			mVertices[4] = vec3(box.a[0], box.a[1], box.b[2]);
			mVertices[5] = vec3(box.b[0], box.a[1], box.b[2]);
			mVertices[6] = vec3(box.a[0], box.b[1], box.b[2]);
			mVertices[7] = vec3(box.b[0], box.b[1], box.b[2]);
			for (int i = 0; i < NUM_BOX_VERTICES; ++i)
				mVertices[i] -= cone.origin;
			real coneMaxHeight = cone.length;
			if (0 <= boxMinHeight && boxMaxHeight <= coneMaxHeight)
			{
				std::copy(mEdges.begin(), mEdges.end(), mCandidateEdges.begin());
				mNumCandidateEdges = 12;
				return true;
			}
			return false;
		}

		void TIQuery::ComputeCandidatesOnBoxEdges(Cone const &cone)
		{
			for (uint16 i = 0; i < NUM_BOX_VERTICES; ++i)
			{
				real h = dot(cone.direction, mVertices[i]);
				real coneMaxHeight = cone.length;
				mProjectionMin[i] = -h;
				mProjectionMax[i] = h - coneMaxHeight;
			}
			uint16 v0 = VERTEX_MIN_BASE, v1 = VERTEX_MAX_BASE;
			for (uint16 i = 0; i < NUM_BOX_EDGES; ++i, ++v0, ++v1)
			{
				auto const &edge = mEdges[i];
				real p0Min = mProjectionMin[edge[0]];
				real p1Min = mProjectionMin[edge[1]];
				bool clipMin = (p0Min < (real)0 && p1Min >(real)0) || (p0Min > (real)0 && p1Min < (real)0);
				if (clipMin)
					mVertices[v0] = (p1Min * mVertices[edge[0]] - p0Min * mVertices[edge[1]]) / (p1Min - p0Min);
				real p0Max = mProjectionMax[edge[0]];
				real p1Max = mProjectionMax[edge[1]];
				bool clipMax = (p0Max < (real)0 && p1Max >(real)0) || (p0Max > (real)0 && p1Max < (real)0);
				if (clipMax)
					mVertices[v1] = (p1Max * mVertices[edge[0]] - p0Max * mVertices[edge[1]]) / (p1Max - p0Max);
				if (clipMin)
				{
					if (clipMax)
						InsertEdge(v0, v1);
					else if (p0Min < (real)0)
						InsertEdge(edge[0], v0);
					else
						InsertEdge(edge[1], v0);
				}
				else if (clipMax)
				{
					if (p0Max < (real)0)
						InsertEdge(edge[0], v1);
					else
						InsertEdge(edge[1], v1);
				}
				else if (p0Min <= (real)0 && p1Min <= (real)0 && p0Max <= (real)0 && p1Max <= (real)0)
					InsertEdge(edge[0], edge[1]);
			}
		}

		void TIQuery::ComputeCandidatesOnBoxFaces()
		{
			real p0, p1, p2, p3;
			uint16 b0, b1, b2, b3, index;
			for (uint16 i = 0; i < NUM_BOX_FACES; ++i)
			{
				auto const &face = mFaces[i];

				// Process the hmin-plane.
				p0 = mProjectionMin[face.v[0]];
				p1 = mProjectionMin[face.v[1]];
				p2 = mProjectionMin[face.v[2]];
				p3 = mProjectionMin[face.v[3]];
				b0 = (p0 < (real)0 ? 0 : (p0 > (real)0 ? 2 : 1));
				b1 = (p1 < (real)0 ? 0 : (p1 > (real)0 ? 2 : 1));
				b2 = (p2 < (real)0 ? 0 : (p2 > (real)0 ? 2 : 1));
				b3 = (p3 < (real)0 ? 0 : (p3 > (real)0 ? 2 : 1));
				index = b3 + 3 * (b2 + 3 * (b1 + 3 * b0));
				(this->*mConfiguration[index])(VERTEX_MIN_BASE, face);

				// Process the hmax-plane.
				p0 = mProjectionMax[face.v[0]];
				p1 = mProjectionMax[face.v[1]];
				p2 = mProjectionMax[face.v[2]];
				p3 = mProjectionMax[face.v[3]];
				b0 = (p0 < (real)0 ? 0 : (p0 > (real)0 ? 2 : 1));
				b1 = (p1 < (real)0 ? 0 : (p1 > (real)0 ? 2 : 1));
				b2 = (p2 < (real)0 ? 0 : (p2 > (real)0 ? 2 : 1));
				b3 = (p3 < (real)0 ? 0 : (p3 > (real)0 ? 2 : 1));
				index = b3 + 3 * (b2 + 3 * (b1 + 3 * b0));
				(this->*mConfiguration[index])(VERTEX_MAX_BASE, face);
			}
		}
	}

	bool intersects(const Aabb &box, const Cone &cone)
	{
		TIQuery query;
		return query(box, cone);
	}
}
