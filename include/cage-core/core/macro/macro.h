#ifdef NULL
#undef NULL // encourage using nullptr
#endif

#if defined(_DEBUG) || defined(DEBUG) || defined(__DEBUG__)
#define CAGE_DEBUG
#endif

#if defined(CAGE_DEBUG) && defined(NDEBUG)
#error CAGE_DEBUG and NDEBUG are incompatible
#endif

#include "platform.h"
#include "api.h"
#include "eval.h"

#define GCHL_EMPTY(...)
#define GCHL_DEFER(...) __VA_ARGS__ GCHL_EMPTY()
#define GCHL_OBSTRUCT(...) __VA_ARGS__ GCHL_DEFER(GCHL_EMPTY)()
#define GCHL_EXPAND(...) __VA_ARGS__

#define CAGE_JOIN(a, ...) GCHL_JOIN(a, __VA_ARGS__)
#define GCHL_JOIN(a, ...) a ## __VA_ARGS__

#define CAGE_STRINGIZE(a) GCHL_STRINGIZE(a)
#define GCHL_STRINGIZE(a) #a

#define GCHL_NUM_INC(x) CAGE_JOIN(GCHL_NMI_, x)
#define GCHL_NUM_DEC(x) CAGE_JOIN(GCHL_NMD_, x)
#include "inc.h"
#include "dec.h"

#define GCHL_PARL (
#define GCHL_PARR )
#define GCHL_PASS_ARGS(...) GCHL_PARL __VA_ARGS__ GCHL_PARR
#include "numargs.h"

#define CAGE_EXPAND_ARGS(OP, ...) CAGE_JOIN(GCHL_XPR_, GCHL_NUM_ARGS(__VA_ARGS__)) GCHL_PASS_ARGS(OP, __VA_ARGS__)
#include "expargs.h"

#define GCHL_CHECK(...) GCHL_NUM_DEC(GCHL_NUM_ARGS(__VA_ARGS__))
#define GCHL_NOT(x) GCHL_CHECK(CAGE_JOIN(GCHL_NOT_, x))
#define GCHL_NOT_0 ~, 1
#define GCHL_COMPL(b) CAGE_JOIN(GCHL_COMPL_, b)
#define GCHL_COMPL_0 1
#define GCHL_COMPL_1 0
#define GCHL_BOOL(x) GCHL_COMPL(GCHL_NOT(x))
#define GCHL_IIF(c) CAGE_JOIN(GCHL_IIF_, c)
#define GCHL_IIF_0(t, ...) __VA_ARGS__
#define GCHL_IIF_1(t, ...) t
#define GCHL_IF(c) GCHL_IIF(GCHL_BOOL(c))
#define GCHL_WHEN(c) GCHL_IF(c)(GCHL_EXPAND, GCHL_EMPTY)

#define CAGE_REPEAT(count, macro) GCHL_WHEN(count)(GCHL_OBSTRUCT(GCHL_REPEAT)()(GCHL_NUM_DEC(count), macro) GCHL_OBSTRUCT(macro)(count))
#define GCHL_REPEAT() CAGE_REPEAT

#define CAGE_LIST(count, prefix) GCHL_WHEN(count)(GCHL_OBSTRUCT(GCHL_LIST)()(CAGE_NUM_DEC(count), prefix) GCHL_OBSTRUCT(CAGE_COMMA)(CAGE_NUM_DEC(count)) GCHL_OBSTRUCT(CAGE_JOIN(prefix, count)))
#define GCHL_LIST() CAGE_LIST
#define CAGE_LIST_2(count, prefix1, prefix2) GCHL_WHEN(count)(GCHL_OBSTRUCT(GCHL_LIST_2)()(CAGE_NUM_DEC(count), prefix1, prefix2) GCHL_OBSTRUCT(CAGE_COMMA(CAGE_NUM_DEC(count))) GCHL_OBSTRUCT(CAGE_JOIN)(prefix1, count) GCHL_OBSTRUCT(CAGE_JOIN)(prefix2, count))
#define GCHL_LIST_2() CAGE_LIST_2

#define GCHL_COMMA_0
#define GCHL_COMMA_1 ,
#define GCHL_COMMA(cond) CAGE_JOIN(GCHL_COMMA_, GCHL_BOOL(cond))
