
#if !defined(CAGE_DEBUG) && !defined(NDEBUG)
#define CAGE_DEBUG
#endif
#if defined(CAGE_DEBUG) && defined(NDEBUG)
#error CAGE_DEBUG and NDEBUG are incompatible
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#define GCHL_API_EXPORT __declspec(dllexport)
#define GCHL_API_IMPORT __declspec(dllimport)
#else
#define GCHL_API_EXPORT __attribute__((visibility("default")))
#define GCHL_API_IMPORT __attribute__((visibility("default")))
#endif

#if defined(_WIN32) || defined(__WIN32__)
#define CAGE_SYSTEM_WINDOWS
#elif defined(linux) || defined(__linux) || defined(__linux__)
#define CAGE_SYSTEM_LINUX
#elif defined(__APPLE__)
#define CAGE_SYSTEM_MAC
#else
#error This operating system is not supported
#endif

#ifdef CAGE_SYSTEM_WINDOWS
#define CAGE_API_PRIVATE
#if defined(_WIN64)
#define CAGE_ARCHITECTURE_64
#else
#define CAGE_ARCHITECTURE_32
#endif
#else
#define CAGE_API_PRIVATE __attribute__((visibility("hidden")))
#if __x86_64__ || __ppc64__
#define CAGE_ARCHITECTURE_64
#else
#define CAGE_ARCHITECTURE_32
#endif
#endif

#include "api.h"

#define CAGE_EVAL_SMALL(...)	GCHL_VL_1(GCHL_VL_1(GCHL_VL_1(__VA_ARGS__)))
#define GCHL_VL_1(...)			GCHL_VL_2(GCHL_VL_2(GCHL_VL_2(__VA_ARGS__)))
#define GCHL_VL_2(...)			GCHL_VL_3(GCHL_VL_3(GCHL_VL_3(__VA_ARGS__)))
#define GCHL_VL_3(...)			__VA_ARGS__

#define CAGE_EVAL_MEDIUM(...)	GCHL_VLL_1(GCHL_VLL_1(GCHL_VLL_1(__VA_ARGS__)))
#define GCHL_VLL_1(...)			GCHL_VLL_2(GCHL_VLL_2(GCHL_VLL_2(__VA_ARGS__)))
#define GCHL_VLL_2(...)			GCHL_VLL_3(GCHL_VLL_3(GCHL_VLL_3(__VA_ARGS__)))
#define GCHL_VLL_3(...)			CAGE_EVAL_SMALL(__VA_ARGS__)

#define CAGE_EVAL_LARGE(...)	GCHL_VLLL_1(GCHL_VLLL_1(GCHL_VLLL_1(__VA_ARGS__)))
#define GCHL_VLLL_1(...)		GCHL_VLLL_2(GCHL_VLLL_2(GCHL_VLLL_2(__VA_ARGS__)))
#define GCHL_VLLL_2(...)		GCHL_VLLL_3(GCHL_VLLL_3(GCHL_VLLL_3(__VA_ARGS__)))
#define GCHL_VLLL_3(...)		CAGE_EVAL_MEDIUM(__VA_ARGS__)

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

#define GCHL_NMD_1 0
#define GCHL_NMD_2 1
#define GCHL_NMD_3 2
#define GCHL_NMD_4 3
#define GCHL_NMD_5 4
#define GCHL_NMD_6 5
#define GCHL_NMD_7 6
#define GCHL_NMD_8 7
#define GCHL_NMD_9 8
#define GCHL_NMD_10 9
#define GCHL_NMD_11 10
#define GCHL_NMD_12 11
#define GCHL_NMD_13 12
#define GCHL_NMD_14 13
#define GCHL_NMD_15 14
#define GCHL_NMD_16 15
#define GCHL_NMD_17 16
#define GCHL_NMD_18 17
#define GCHL_NMD_19 18
#define GCHL_NMD_20 19
#define GCHL_NMD_21 20
#define GCHL_NMD_22 21
#define GCHL_NMD_23 22
#define GCHL_NMD_24 23
#define GCHL_NMD_25 24
#define GCHL_NMD_26 25
#define GCHL_NMD_27 26
#define GCHL_NMD_28 27
#define GCHL_NMD_29 28
#define GCHL_NMD_30 29
#define GCHL_NMD_31 30
#define GCHL_NMD_32 31
#define GCHL_NMD_33 32
#define GCHL_NMD_34 33
#define GCHL_NMD_35 34
#define GCHL_NMD_36 35
#define GCHL_NMD_37 36
#define GCHL_NMD_38 37
#define GCHL_NMD_39 38
#define GCHL_NMD_40 39
#define GCHL_NMD_41 40
#define GCHL_NMD_42 41
#define GCHL_NMD_43 42
#define GCHL_NMD_44 43
#define GCHL_NMD_45 44
#define GCHL_NMD_46 45
#define GCHL_NMD_47 46
#define GCHL_NMD_48 47
#define GCHL_NMD_49 48
#define GCHL_NMD_50 49

#define GCHL_NMI_0 1
#define GCHL_NMI_1 2
#define GCHL_NMI_2 3
#define GCHL_NMI_3 4
#define GCHL_NMI_4 5
#define GCHL_NMI_5 6
#define GCHL_NMI_6 7
#define GCHL_NMI_7 8
#define GCHL_NMI_8 9
#define GCHL_NMI_9 10
#define GCHL_NMI_10 11
#define GCHL_NMI_11 12
#define GCHL_NMI_12 13
#define GCHL_NMI_13 14
#define GCHL_NMI_14 15
#define GCHL_NMI_15 16
#define GCHL_NMI_16 17
#define GCHL_NMI_17 18
#define GCHL_NMI_18 19
#define GCHL_NMI_19 20
#define GCHL_NMI_20 21
#define GCHL_NMI_21 22
#define GCHL_NMI_22 23
#define GCHL_NMI_23 24
#define GCHL_NMI_24 25
#define GCHL_NMI_25 26
#define GCHL_NMI_26 27
#define GCHL_NMI_27 28
#define GCHL_NMI_28 29
#define GCHL_NMI_29 30
#define GCHL_NMI_30 31
#define GCHL_NMI_31 32
#define GCHL_NMI_32 33
#define GCHL_NMI_33 34
#define GCHL_NMI_34 35
#define GCHL_NMI_35 36
#define GCHL_NMI_36 37
#define GCHL_NMI_37 38
#define GCHL_NMI_38 39
#define GCHL_NMI_39 40
#define GCHL_NMI_40 41
#define GCHL_NMI_41 42
#define GCHL_NMI_42 43
#define GCHL_NMI_43 44
#define GCHL_NMI_44 45
#define GCHL_NMI_45 46
#define GCHL_NMI_46 47
#define GCHL_NMI_47 48
#define GCHL_NMI_48 49
#define GCHL_NMI_49 50
#define GCHL_NMI_50 51

#define GCHL_PARL (
#define GCHL_PARR )
#define GCHL_PASS_ARGS(...) GCHL_PARL __VA_ARGS__ GCHL_PARR
#if defined(_MSC_VER) && !defined(__llvm__)
#define GCHL_NUM_ARGS(...) GCHL_NMR GCHL_PARL ,##__VA_ARGS__,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0 GCHL_PARR
#define GCHL_NMR(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,N,...) N
#else
#define PP_ARG_N(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,N,...) N
#define PP_RSEQ_N() 50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0
#define PP_NARG_(...) PP_ARG_N(__VA_ARGS__)
#define PP_COMMASEQ_N() 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0
#define PP_COMMA() ,
#define PP_HASCOMMA(...) PP_NARG_(__VA_ARGS__,PP_COMMASEQ_N())
#define GCHL_NUM_ARGS(...) PP_NARG_HELPER1(PP_HASCOMMA(__VA_ARGS__),PP_HASCOMMA(PP_COMMA __VA_ARGS__ ()),PP_NARG_(__VA_ARGS__,PP_RSEQ_N()))
#define PP_NARG_HELPER1(a,b,N) PP_NARG_HELPER2(a,b,N)
#define PP_NARG_HELPER2(a,b,N) PP_NARG_HELPER3_ ## a ## b(N)
#define PP_NARG_HELPER3_01(N) 0
#define PP_NARG_HELPER3_00(N) 1
#define PP_NARG_HELPER3_11(N) N
#endif

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

//#define CAGE_LIST(count, prefix) GCHL_WHEN(count)(GCHL_OBSTRUCT(GCHL_LIST)()(CAGE_NUM_DEC(count), prefix) GCHL_OBSTRUCT(CAGE_COMMA)(CAGE_NUM_DEC(count)) GCHL_OBSTRUCT(CAGE_JOIN(prefix, count)))
//#define GCHL_LIST() CAGE_LIST
//#define CAGE_LIST_2(count, prefix1, prefix2) GCHL_WHEN(count)(GCHL_OBSTRUCT(GCHL_LIST_2)()(CAGE_NUM_DEC(count), prefix1, prefix2) GCHL_OBSTRUCT(CAGE_COMMA(CAGE_NUM_DEC(count))) GCHL_OBSTRUCT(CAGE_JOIN)(prefix1, count) GCHL_OBSTRUCT(CAGE_JOIN)(prefix2, count))
//#define GCHL_LIST_2() CAGE_LIST_2

//#define GCHL_COMMA_0
//#define GCHL_COMMA_1 ,
//#define GCHL_COMMA(cond) CAGE_JOIN(GCHL_COMMA_, GCHL_BOOL(cond))
