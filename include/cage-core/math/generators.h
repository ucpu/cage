#define GCHL_GEN_QUATERNION_BASE(NAME,N) \
real data[N]; \
explicit NAME (const real data[N]) { for (uint32 i = 0; i < N; i++) this->data[i] = data[i]; } \
NAME operator - () const { return inverse(); } \
NAME operator + () const { return *this; } \
real &operator [] (uint32 idx) { CAGE_ASSERT_RUNTIME (idx < N, "index out of range", idx, N); return data[idx]; } \
real operator [] (uint32 idx) const { CAGE_ASSERT_RUNTIME (idx < N, "index out of range", idx, N); return data[idx]; } \
bool operator == (const NAME &other) const { for (uint32 i = 0; i < N; i++) if (data[i] != other.data[i]) return false; return true; } \
bool operator != (const NAME &other) const { return !(*this == other); } \
operator string () const { string r = "("; for (uint32 i = 0; i < N - 1; i++) r += string(data[i]) + ", "; return r + string(data[N - 1]) + ")"; } \
NAME inverse () const { return *this * -1; } \
NAME normalize () const { return *this / length(); } \
real length () const { return squaredLength().sqrt(); } \
real squaredLength () const { return dot(*this); } \
real dot (const NAME &other) const { real sum = 0; for (uint32 i = 0; i < N; i++) sum += data[i] * other.data[i]; return sum; } \
bool valid () const { for (uint32 i = 0; i < N; i++) if (!data[i].valid()) return false; return true; } \
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
NAME () {} \
NAME min (const NAME &other) const { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = data[i] < other.data[i] ? data[i] : other.data[i]; return r; } \
NAME max (const NAME &other) const { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = data[i] > other.data[i] ? data[i] : other.data[i]; return r; } \
NAME min (real other) const { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = data[i] < other ? data[i] : other; return r; } \
NAME max (real other) const { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = data[i] > other ? data[i] : other; return r; } \
NAME abs () const { return this->max(-*this); } \
NAME clamp (const NAME &minimum, const NAME &maximum) const { CAGE_ASSERT_RUNTIME(minimum.min(maximum) == minimum); return min(maximum).max(minimum); } \
NAME clamp (real minimum, real maximum) const { CAGE_ASSERT_RUNTIME (minimum <= maximum, minimum, maximum); return min(maximum).max(minimum); } \
real distance(const NAME &other) const { return squaredDistance(other).sqrt(); } \
real squaredDistance(const NAME &other) const { return (*this - other).squaredLength(); } \
NAME &operator *= (const NAME &other) { return *this = *this * other; } \
NAME &operator /= (const NAME &other) { return *this = *this / other; } \
NAME operator * (const NAME &other) const { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = data[i] * other.data[i]; return r; } \
NAME operator / (const NAME &other) const { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = data[i] / other.data[i]; return r; } \
static const uint32 Dimension = N;

#define GCHL_GEN_QUATERNION_FUNCTIONS(NAME, N) \
inline NAME inverse (const NAME &th) { return th.inverse(); } \
inline NAME normalize (const NAME &th) { return th.normalize(); } \
inline real length (const NAME &th) { return th.length(); } \
inline real squaredLength (const NAME &th) { return th.squaredLength(); } \
inline real dot (const NAME &th, const NAME &other) { return th.dot(other); } \
inline bool valid (const NAME &th) { return th.valid(); } \
inline NAME operator + (real th, const NAME &other) { return other + th; } \
inline NAME operator - (real th, const NAME &other) { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = th - other.data[i]; return r; } \
inline NAME operator * (real th, const NAME &other) { return other * th; } \
inline NAME operator / (real th, const NAME &other) { NAME r; for (uint32 i = 0; i < N; i++) r.data[i] = th / other.data[i]; return r; }

#define GCHL_GEN_VECTOR_FUNCTIONS(NAME, N) \
GCHL_GEN_QUATERNION_FUNCTIONS(NAME, N) \
inline NAME min (const NAME &th, const NAME &other) { return th.min(other); } \
inline NAME max (const NAME &th, const NAME &other) { return th.max(other); } \
inline NAME min (const NAME &th, real other) { return th.min(other); } \
inline NAME max (const NAME &th, real other) { return th.max(other); } \
inline NAME min (real other, const NAME &th) { return th.min(other); } \
inline NAME max (real other, const NAME &th) { return th.max(other); } \
inline NAME abs (const NAME &th) { return th.abs(); } \
inline NAME clamp (const NAME &th, const NAME &minimum, const NAME &maximum) { return th.clamp(minimum, maximum); } \
inline NAME clamp (const NAME &th, real minimum, real maximum) { return th.clamp(minimum, maximum); } \
inline real distance (const NAME &th, const NAME &other) { return th.distance(other); } \
inline real squaredDistance (const NAME &th, const NAME &other) { return th.squaredDistance(other); }
