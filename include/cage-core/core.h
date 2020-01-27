#ifndef guard_core_h_39243ce0_71a5_4900_8898_63fb89591b7b_
#define guard_core_h_39243ce0_71a5_4900_8898_63fb89591b7b_

#if !defined(CAGE_DEBUG) && !defined(NDEBUG)
#define CAGE_DEBUG
#endif
#if defined(CAGE_DEBUG) && defined(NDEBUG)
#error CAGE_DEBUG and NDEBUG are incompatible
#endif

#if defined(_WIN32)
#define CAGE_API_EXPORT __declspec(dllexport)
#define CAGE_API_IMPORT __declspec(dllimport)
#else
#define CAGE_API_EXPORT __attribute__((visibility("default")))
#define CAGE_API_IMPORT __attribute__((visibility("default")))
#endif

#ifdef CAGE_CORE_EXPORT
#define CAGE_CORE_API CAGE_API_EXPORT
#else
#define CAGE_CORE_API CAGE_API_IMPORT
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

#define GCHL_XPR_0(OP, ARG_NOT_USED)
#define GCHL_XPR_1(OP, ARG1) OP (ARG1)
#define GCHL_XPR_2(OP, ARG1, ARG2) OP (ARG1) OP (ARG2)
#define GCHL_XPR_3(OP, ARG1, ARG2, ARG3) OP (ARG1) OP (ARG2) OP (ARG3)
#define GCHL_XPR_4(OP, ARG1, ARG2, ARG3, ARG4) OP (ARG1) OP (ARG2) OP (ARG3) OP (ARG4)
#define GCHL_XPR_5(OP, ARG1, ARG2, ARG3, ARG4, ARG5) OP (ARG1) OP (ARG2) OP (ARG3) OP (ARG4) OP (ARG5)
#define GCHL_XPR_6(OP, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) OP (ARG1) OP (ARG2) OP (ARG3) OP (ARG4) OP (ARG5) OP (ARG6)
#define GCHL_XPR_7(OP, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) OP (ARG1) OP (ARG2) OP (ARG3) OP (ARG4) OP (ARG5) OP (ARG6) OP (ARG7)
#define GCHL_XPR_8(OP, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8) OP (ARG1) OP (ARG2) OP (ARG3) OP (ARG4) OP (ARG5) OP (ARG6) OP (ARG7) OP (ARG8)
#define GCHL_XPR_9(OP, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9) OP (ARG1) OP (ARG2) OP (ARG3) OP (ARG4) OP (ARG5) OP (ARG6) OP (ARG7) OP (ARG8) OP (ARG9)
#define GCHL_XPR_10(OP, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10) OP (ARG1) OP (ARG2) OP (ARG3) OP (ARG4) OP (ARG5) OP (ARG6) OP (ARG7) OP (ARG8) OP (ARG9) OP (ARG10)
#define GCHL_XPR_11(OP, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11) OP (ARG1) OP (ARG2) OP (ARG3) OP (ARG4) OP (ARG5) OP (ARG6) OP (ARG7) OP (ARG8) OP (ARG9) OP (ARG10) OP (ARG11)
#define GCHL_XPR_12(OP, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12) OP (ARG1) OP (ARG2) OP (ARG3) OP (ARG4) OP (ARG5) OP (ARG6) OP (ARG7) OP (ARG8) OP (ARG9) OP (ARG10) OP (ARG11) OP (ARG12)
#define GCHL_XPR_13(OP, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13) OP (ARG1) OP (ARG2) OP (ARG3) OP (ARG4) OP (ARG5) OP (ARG6) OP (ARG7) OP (ARG8) OP (ARG9) OP (ARG10) OP (ARG11) OP (ARG12) OP (ARG13)
#define GCHL_XPR_14(OP, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14) OP (ARG1) OP (ARG2) OP (ARG3) OP (ARG4) OP (ARG5) OP (ARG6) OP (ARG7) OP (ARG8) OP (ARG9) OP (ARG10) OP (ARG11) OP (ARG12) OP (ARG13) OP (ARG14)
#define GCHL_XPR_15(OP, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15) OP (ARG1) OP (ARG2) OP (ARG3) OP (ARG4) OP (ARG5) OP (ARG6) OP (ARG7) OP (ARG8) OP (ARG9) OP (ARG10) OP (ARG11) OP (ARG12) OP (ARG13) OP (ARG14) OP (ARG15)
#define GCHL_XPR_16(OP, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16) OP (ARG1) OP (ARG2) OP (ARG3) OP (ARG4) OP (ARG5) OP (ARG6) OP (ARG7) OP (ARG8) OP (ARG9) OP (ARG10) OP (ARG11) OP (ARG12) OP (ARG13) OP (ARG14) OP (ARG15) OP (ARG16)
#define GCHL_XPR_17(OP, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17) OP (ARG1) OP (ARG2) OP (ARG3) OP (ARG4) OP (ARG5) OP (ARG6) OP (ARG7) OP (ARG8) OP (ARG9) OP (ARG10) OP (ARG11) OP (ARG12) OP (ARG13) OP (ARG14) OP (ARG15) OP (ARG16) OP (ARG17)
#define GCHL_XPR_18(OP, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18) OP (ARG1) OP (ARG2) OP (ARG3) OP (ARG4) OP (ARG5) OP (ARG6) OP (ARG7) OP (ARG8) OP (ARG9) OP (ARG10) OP (ARG11) OP (ARG12) OP (ARG13) OP (ARG14) OP (ARG15) OP (ARG16) OP (ARG17) OP (ARG18)
#define GCHL_XPR_19(OP, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19) OP (ARG1) OP (ARG2) OP (ARG3) OP (ARG4) OP (ARG5) OP (ARG6) OP (ARG7) OP (ARG8) OP (ARG9) OP (ARG10) OP (ARG11) OP (ARG12) OP (ARG13) OP (ARG14) OP (ARG15) OP (ARG16) OP (ARG17) OP (ARG18) OP (ARG19)
#define GCHL_XPR_20(OP, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20) OP (ARG1) OP (ARG2) OP (ARG3) OP (ARG4) OP (ARG5) OP (ARG6) OP (ARG7) OP (ARG8) OP (ARG9) OP (ARG10) OP (ARG11) OP (ARG12) OP (ARG13) OP (ARG14) OP (ARG15) OP (ARG16) OP (ARG17) OP (ARG18) OP (ARG19) OP (ARG20)
#define GCHL_XPR_21(OP, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21) OP (ARG1) OP (ARG2) OP (ARG3) OP (ARG4) OP (ARG5) OP (ARG6) OP (ARG7) OP (ARG8) OP (ARG9) OP (ARG10) OP (ARG11) OP (ARG12) OP (ARG13) OP (ARG14) OP (ARG15) OP (ARG16) OP (ARG17) OP (ARG18) OP (ARG19) OP (ARG20) OP (ARG21)
#define GCHL_XPR_22(OP, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21, ARG22) OP (ARG1) OP (ARG2) OP (ARG3) OP (ARG4) OP (ARG5) OP (ARG6) OP (ARG7) OP (ARG8) OP (ARG9) OP (ARG10) OP (ARG11) OP (ARG12) OP (ARG13) OP (ARG14) OP (ARG15) OP (ARG16) OP (ARG17) OP (ARG18) OP (ARG19) OP (ARG20) OP (ARG21) OP (ARG22)
#define GCHL_XPR_23(OP, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21, ARG22, ARG23) OP (ARG1) OP (ARG2) OP (ARG3) OP (ARG4) OP (ARG5) OP (ARG6) OP (ARG7) OP (ARG8) OP (ARG9) OP (ARG10) OP (ARG11) OP (ARG12) OP (ARG13) OP (ARG14) OP (ARG15) OP (ARG16) OP (ARG17) OP (ARG18) OP (ARG19) OP (ARG20) OP (ARG21) OP (ARG22) OP (ARG23)
#define GCHL_XPR_24(OP, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21, ARG22, ARG23, ARG24) OP (ARG1) OP (ARG2) OP (ARG3) OP (ARG4) OP (ARG5) OP (ARG6) OP (ARG7) OP (ARG8) OP (ARG9) OP (ARG10) OP (ARG11) OP (ARG12) OP (ARG13) OP (ARG14) OP (ARG15) OP (ARG16) OP (ARG17) OP (ARG18) OP (ARG19) OP (ARG20) OP (ARG21) OP (ARG22) OP (ARG23) OP (ARG24)
#define GCHL_XPR_25(OP, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21, ARG22, ARG23, ARG24, ARG25) OP (ARG1) OP (ARG2) OP (ARG3) OP (ARG4) OP (ARG5) OP (ARG6) OP (ARG7) OP (ARG8) OP (ARG9) OP (ARG10) OP (ARG11) OP (ARG12) OP (ARG13) OP (ARG14) OP (ARG15) OP (ARG16) OP (ARG17) OP (ARG18) OP (ARG19) OP (ARG20) OP (ARG21) OP (ARG22) OP (ARG23) OP (ARG24) OP (ARG25)
#define GCHL_XPR_26(OP, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21, ARG22, ARG23, ARG24, ARG25, ARG26) OP (ARG1) OP (ARG2) OP (ARG3) OP (ARG4) OP (ARG5) OP (ARG6) OP (ARG7) OP (ARG8) OP (ARG9) OP (ARG10) OP (ARG11) OP (ARG12) OP (ARG13) OP (ARG14) OP (ARG15) OP (ARG16) OP (ARG17) OP (ARG18) OP (ARG19) OP (ARG20) OP (ARG21) OP (ARG22) OP (ARG23) OP (ARG24) OP (ARG25) OP (ARG26)
#define GCHL_XPR_27(OP, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21, ARG22, ARG23, ARG24, ARG25, ARG26, ARG27) OP (ARG1) OP (ARG2) OP (ARG3) OP (ARG4) OP (ARG5) OP (ARG6) OP (ARG7) OP (ARG8) OP (ARG9) OP (ARG10) OP (ARG11) OP (ARG12) OP (ARG13) OP (ARG14) OP (ARG15) OP (ARG16) OP (ARG17) OP (ARG18) OP (ARG19) OP (ARG20) OP (ARG21) OP (ARG22) OP (ARG23) OP (ARG24) OP (ARG25) OP (ARG26) OP (ARG27)
#define GCHL_XPR_28(OP, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21, ARG22, ARG23, ARG24, ARG25, ARG26, ARG27, ARG28) OP (ARG1) OP (ARG2) OP (ARG3) OP (ARG4) OP (ARG5) OP (ARG6) OP (ARG7) OP (ARG8) OP (ARG9) OP (ARG10) OP (ARG11) OP (ARG12) OP (ARG13) OP (ARG14) OP (ARG15) OP (ARG16) OP (ARG17) OP (ARG18) OP (ARG19) OP (ARG20) OP (ARG21) OP (ARG22) OP (ARG23) OP (ARG24) OP (ARG25) OP (ARG26) OP (ARG27) OP (ARG28)
#define GCHL_XPR_29(OP, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21, ARG22, ARG23, ARG24, ARG25, ARG26, ARG27, ARG28, ARG29) OP (ARG1) OP (ARG2) OP (ARG3) OP (ARG4) OP (ARG5) OP (ARG6) OP (ARG7) OP (ARG8) OP (ARG9) OP (ARG10) OP (ARG11) OP (ARG12) OP (ARG13) OP (ARG14) OP (ARG15) OP (ARG16) OP (ARG17) OP (ARG18) OP (ARG19) OP (ARG20) OP (ARG21) OP (ARG22) OP (ARG23) OP (ARG24) OP (ARG25) OP (ARG26) OP (ARG27) OP (ARG28) OP (ARG29)
#define GCHL_XPR_30(OP, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21, ARG22, ARG23, ARG24, ARG25, ARG26, ARG27, ARG28, ARG29, ARG30) OP (ARG1) OP (ARG2) OP (ARG3) OP (ARG4) OP (ARG5) OP (ARG6) OP (ARG7) OP (ARG8) OP (ARG9) OP (ARG10) OP (ARG11) OP (ARG12) OP (ARG13) OP (ARG14) OP (ARG15) OP (ARG16) OP (ARG17) OP (ARG18) OP (ARG19) OP (ARG20) OP (ARG21) OP (ARG22) OP (ARG23) OP (ARG24) OP (ARG25) OP (ARG26) OP (ARG27) OP (ARG28) OP (ARG29) OP (ARG30)

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

#ifdef CAGE_DEBUG
#ifndef CAGE_ASSERT_ENABLED
#define CAGE_ASSERT_ENABLED
#endif
#endif
#ifdef CAGE_ASSERT_ENABLED
#define GCHL_ASSVAR(VAR) .variable(CAGE_STRINGIZE(VAR), VAR)
#define CAGE_ASSERT(EXP, ...) { ::cage::privat::AssertPriv(!!(EXP), CAGE_STRINGIZE(EXP), __FILE__, CAGE_STRINGIZE(__LINE__), __FUNCTION__) GCHL_DEFER(CAGE_EXPAND_ARGS(GCHL_ASSVAR, __VA_ARGS__))(); }
#else
#define CAGE_ASSERT(EXP, ...)
#endif

#define CAGE_THROW_SILENT(EXCEPTION, ...) { EXCEPTION e_(__FILE__, __LINE__, __FUNCTION__, ::cage::SeverityEnum::Error, __VA_ARGS__); throw e_; }
#define CAGE_THROW_ERROR(EXCEPTION, ...) { EXCEPTION e_(__FILE__, __LINE__, __FUNCTION__, ::cage::SeverityEnum::Error, __VA_ARGS__); e_.makeLog(); throw e_; }
#define CAGE_THROW_CRITICAL(EXCEPTION, ...) { EXCEPTION e_(__FILE__, __LINE__, __FUNCTION__, ::cage::SeverityEnum::Critical, __VA_ARGS__); e_.makeLog(); throw e_; }

#define CAGE_LOG(SEVERITY, COMPONENT, MESSAGE) ::cage::privat::makeLog(__FILE__, __LINE__, __FUNCTION__, SEVERITY, COMPONENT, MESSAGE, false, false)
#define CAGE_LOG_CONTINUE(SEVERITY, COMPONENT, MESSAGE) ::cage::privat::makeLog(__FILE__, __LINE__, __FUNCTION__, SEVERITY, COMPONENT, MESSAGE, true, false)
#ifdef CAGE_DEBUG
#define CAGE_LOG_DEBUG(SEVERITY, COMPONENT, MESSAGE) ::cage::privat::makeLog(__FILE__, __LINE__, __FUNCTION__, SEVERITY, COMPONENT, MESSAGE, false, true)
#define CAGE_LOG_CONTINUE_DEBUG(SEVERITY, COMPONENT, MESSAGE) ::cage::privat::makeLog(__FILE__, __LINE__, __FUNCTION__, SEVERITY, COMPONENT, MESSAGE, true, true)
#else
#define CAGE_LOG_DEBUG(SEVERITY, COMPONENT, MESSAGE)
#define CAGE_LOG_CONTINUE_DEBUG(SEVERITY, COMPONENT, MESSAGE)
#endif

namespace cage
{
	// numeric types

	typedef unsigned char uint8;
	typedef signed char sint8;
	typedef unsigned short uint16;
	typedef signed short sint16;
	typedef unsigned int uint32;
	typedef signed int sint32;
#ifdef CAGE_SYSTEM_WINDOWS
	typedef unsigned long long uint64;
	typedef signed long long sint64;
#else
	typedef unsigned long uint64;
	typedef signed long sint64;
#endif
#ifdef CAGE_ARCHITECTURE_64
	typedef uint64 uintPtr;
	typedef sint64 sintPtr;
#else
	typedef uint32 uintPtr;
	typedef sint32 sintPtr;
#endif

	// numeric limits

	namespace privat
	{
		template<bool>
		struct helper_is_char_signed
		{};

		template<>
		struct helper_is_char_signed<true>
		{
			typedef signed char type;
		};

		template<>
		struct helper_is_char_signed<false>
		{
			typedef unsigned char type;
		};
	}

	namespace detail
	{
		template<class T>
		struct numeric_limits
		{
			static const bool is_specialized = false;
		};

		template<>
		struct numeric_limits<unsigned char>
		{
			static const bool is_specialized = true;
			static const bool is_signed = false;
			static constexpr uint8 min() noexcept { return 0; };
			static constexpr uint8 max() noexcept { return 255u; };
			typedef uint8 make_unsigned;
		};

		template<>
		struct numeric_limits<unsigned short>
		{
			static const bool is_specialized = true;
			static const bool is_signed = false;
			static constexpr uint16 min() noexcept { return 0; };
			static constexpr uint16 max() noexcept { return 65535u; };
			typedef uint16 make_unsigned;
		};

		template<>
		struct numeric_limits<unsigned int>
		{
			static const bool is_specialized = true;
			static const bool is_signed = false;
			static constexpr uint32 min() noexcept { return 0; };
			static constexpr uint32 max() noexcept { return 4294967295u; };
			typedef uint32 make_unsigned;
		};

		template<>
		struct numeric_limits<unsigned long long>
		{
			static const bool is_specialized = true;
			static const bool is_signed = false;
			static constexpr uint64 min() noexcept { return 0; };
			static constexpr uint64 max() noexcept { return 18446744073709551615LLu; };
			typedef uint64 make_unsigned;
		};

#ifdef CAGE_SYSTEM_WINDOWS
		template<>
		struct numeric_limits<unsigned long> : public numeric_limits<unsigned int>
		{};
#else
		template<>
		struct numeric_limits<unsigned long> : public numeric_limits<unsigned long long>
		{};
#endif

		template<>
		struct numeric_limits<signed char>
		{
			static const bool is_specialized = true;
			static const bool is_signed = true;
			static constexpr sint8 min() noexcept { return -127 - 1; };
			static constexpr sint8 max() noexcept { return  127; };
			typedef uint8 make_unsigned;
		};

		template<>
		struct numeric_limits<signed short>
		{
			static const bool is_specialized = true;
			static const bool is_signed = true;
			static constexpr sint16 min() noexcept { return -32767 - 1; };
			static constexpr sint16 max() noexcept { return  32767; };
			typedef uint16 make_unsigned;
		};

		template<>
		struct numeric_limits<signed int>
		{
			static const bool is_specialized = true;
			static const bool is_signed = true;
			static constexpr sint32 min() noexcept { return -2147483647 - 1; };
			static constexpr sint32 max() noexcept { return  2147483647; };
			typedef uint32 make_unsigned;
		};

		template<>
		struct numeric_limits<signed long long>
		{
			static const bool is_specialized = true;
			static const bool is_signed = true;
			static constexpr sint64 min() noexcept { return -9223372036854775807LL - 1; };
			static constexpr sint64 max() noexcept { return  9223372036854775807LL; };
			typedef uint64 make_unsigned;
		};

#ifdef CAGE_SYSTEM_WINDOWS
		template<>
		struct numeric_limits<signed long> : public numeric_limits<signed int>
		{};
#else
		template<>
		struct numeric_limits<signed long> : public numeric_limits<signed long long>
		{};
#endif

		template<>
		struct numeric_limits<float>
		{
			static const bool is_specialized = true;
			static const bool is_signed = true;
			static constexpr float min() noexcept { return -1e+37f; };
			static constexpr float max() noexcept { return  1e+37f; };
			typedef float make_unsigned;
		};

		template<>
		struct numeric_limits<double>
		{
			static const bool is_specialized = true;
			static const bool is_signed = true;
			static constexpr double min() noexcept { return -1e+308; };
			static constexpr double max() noexcept { return  1e+308; };
			typedef double make_unsigned;
		};

		// char is distinct type from both unsigned char and signed char
		template<>
		struct numeric_limits<char> : public numeric_limits<privat::helper_is_char_signed<(char)-1 < (char)0>::type>
		{};
	}

	// max struct

	namespace privat
	{
		// represents the maximum positive value possible in any numeric type
		struct MaxValue
		{
			template<class T>
			constexpr operator T () const
			{
				return detail::numeric_limits<T>::max();
			}
		};

		template<class T> constexpr bool operator == (T lhs, MaxValue rhs) noexcept { return lhs == detail::numeric_limits<T>::max(); }
		template<class T> constexpr bool operator != (T lhs, MaxValue rhs) noexcept { return lhs != detail::numeric_limits<T>::max(); }
		template<class T> constexpr bool operator <= (T lhs, MaxValue rhs) noexcept { return lhs <= detail::numeric_limits<T>::max(); }
		template<class T> constexpr bool operator >= (T lhs, MaxValue rhs) noexcept { return lhs >= detail::numeric_limits<T>::max(); }
		template<class T> constexpr bool operator < (T lhs, MaxValue rhs) noexcept { return lhs < detail::numeric_limits<T>::max(); }
		template<class T> constexpr bool operator > (T lhs, MaxValue rhs) noexcept { return lhs > detail::numeric_limits<T>::max(); }
	}

	static constexpr const privat::MaxValue m = privat::MaxValue();

	// template magic

	namespace templates
	{
		template<bool Cond, class T> struct enable_if {};
		template<class T> struct enable_if<true, T> { typedef T type; };
		template<uintPtr Size> struct base_unsigned_type {};
		template<> struct base_unsigned_type<1> { typedef uint8 type; };
		template<> struct base_unsigned_type<2> { typedef uint16 type; };
		template<> struct base_unsigned_type<4> { typedef uint32 type; };
		template<> struct base_unsigned_type<8> { typedef uint64 type; };
		template<class T> struct underlying_type { typedef typename base_unsigned_type<sizeof(T)>::type type; };
		template<class T> struct remove_const { typedef T type; };
		template<class T> struct remove_const<const T> { typedef T type; };
		template<class T> struct remove_reference { typedef T type; };
		template<class T> struct remove_reference<T&> { typedef T type; };
		template<class T> struct remove_reference<T&&> { typedef T type; };
		template<typename T> struct is_lvalue_reference { static constexpr const bool value = false; };
		template<typename T> struct is_lvalue_reference<T&> { static constexpr const bool value = true; };
		template<class T> inline constexpr T &&forward(typename remove_reference<T>::type  &v) noexcept { return static_cast<T&&>(v); }
		template<class T> inline constexpr T &&forward(typename remove_reference<T>::type &&v) noexcept { static_assert(!is_lvalue_reference<T>::value, "invalid rvalue to lvalue conversion"); return static_cast<T&&>(v); }
		template<class T> inline constexpr typename remove_reference<T>::type &&move(T &&v) noexcept { return static_cast<typename remove_reference<T>::type&&>(v); }
	}

	// enum class bit operators

	template<class T> struct enable_bitmask_operators { static const bool enable = false; };
	template<class T> inline constexpr typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator ~ (T lhs) noexcept { typedef typename templates::underlying_type<T>::type underlying; return static_cast<T>(~static_cast<underlying>(lhs)); }
	template<class T> inline constexpr typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator | (T lhs, T rhs) noexcept { typedef typename templates::underlying_type<T>::type underlying; return static_cast<T>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs)); }
	template<class T> inline constexpr typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator & (T lhs, T rhs) noexcept { typedef typename templates::underlying_type<T>::type underlying; return static_cast<T>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs)); }
	template<class T> inline constexpr typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator ^ (T lhs, T rhs) noexcept { typedef typename templates::underlying_type<T>::type underlying; return static_cast<T>(static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs)); }
	template<class T> inline typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator |= (T &lhs, T rhs) noexcept { typedef typename templates::underlying_type<T>::type underlying; return lhs = static_cast<T>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs)); }
	template<class T> inline typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator &= (T &lhs, T rhs) noexcept { typedef typename templates::underlying_type<T>::type underlying; return lhs = static_cast<T>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs)); }
	template<class T> inline typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator ^= (T &lhs, T rhs) noexcept { typedef typename templates::underlying_type<T>::type underlying; return lhs = static_cast<T>(static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs)); }
	template<class T> inline typename templates::enable_if<enable_bitmask_operators<T>::enable, bool>::type any(T lhs) noexcept { typedef typename templates::underlying_type<T>::type underlying; return static_cast<underlying>(lhs) != 0; }
	template<class T> inline typename templates::enable_if<enable_bitmask_operators<T>::enable, bool>::type none(T lhs) noexcept { typedef typename templates::underlying_type<T>::type underlying; return static_cast<underlying>(lhs) == 0; }

	// this macro has to be used inside namespace cage
#define GCHL_ENUM_BITS(TYPE) template<> struct enable_bitmask_operators<TYPE> { static const bool enable = true; };

	// Immovable

	struct CAGE_CORE_API Immovable
	{
		Immovable() = default;
		Immovable(const Immovable &) = delete;
		Immovable(Immovable &&) = delete;
		Immovable &operator = (const Immovable &) = delete;
		Immovable &operator = (Immovable &&) = delete;
	};

	// forward declarations

	namespace detail { template<uint32 N> struct StringBase; }
	typedef detail::StringBase<1000> string;
	struct real;
	struct rads;
	struct degs;
	struct vec2;
	struct vec3;
	struct vec4;
	struct quat;
	struct mat3;
	struct mat4;
	struct transform;
	struct line;
	struct triangle;
	struct plane;
	struct sphere;
	struct aabb;

	class AssetManager;
	struct AssetManagerCreateConfig;
	struct AssetContext;
	struct AssetScheme;
	struct AssetHeader;
	struct AssetHeader;
	enum class StereoModeEnum : uint32;
	enum class StereoEyeEnum : uint32;
	struct StereoCameraInput;
	struct StereoCameraOutput;
	class CollisionMesh;
	struct CollisionPair;
	class CollisionQuery;
	class CollisionData;
	struct CollisionDataCreateConfig;
	class Mutex;
	class Barrier;
	class Semaphore;
	class ConditionalVariableBase;
	class ConditionalVariable;
	class Thread;
	struct ConcurrentQueueTerminated;
	template<class T> class ConcurrentQueue;
	enum class ConfigTypeEnum : uint32;
	struct ConfigBool;
	struct ConfigSint32;
	struct ConfigSint64;
	struct ConfigUint32;
	struct ConfigUint64;
	struct ConfigFloat;
	struct ConfigDouble;
	struct ConfigString;
	class ConfigList;
	struct EntityComponentCreateConfig;
	class EntityManager;
	struct EntityManagerCreateConfig;
	class Entity;
	class EntityComponent;
	class EntityGroup;
	struct FileMode;
	class File;
	enum class PathTypeFlags : uint32;
	class FilesystemWatcher;
	class DirectoryList;
	class Filesystem;
	template<uint32 N> struct Guid;
	struct HashString;
	class Image;
	class Ini;
	class LineReader;
	class Logger;
	class LoggerOutputFile;
	template<class Key, class Value, class Hasher> struct LruCache;
	template<class BoundsPolicy, class TaggingPolicy, class TrackingPolicy> struct MemoryAllocatorPolicyLinear;
	template<uint8 N, class BoundsPolicy, class TaggingPolicy, class TrackingPolicy> struct MemoryAllocatorPolicyNFrame;
	template<uintPtr AtomSize, class BoundsPolicy, class TaggingPolicy, class TrackingPolicy> struct MemoryAllocatorPolicyPool;
	template<class BoundsPolicy, class TaggingPolicy, class TrackingPolicy> struct MemoryAllocatorPolicyQueue;
	template<class BoundsPolicy, class TaggingPolicy, class TrackingPolicy> struct MemoryAllocatorPolicyStack;
	template<class AllocatorPolicy, class ConcurrentPolicy> struct MemoryArenaFixed;
	template<class AllocatorPolicy, class ConcurrentPolicy> struct MemoryArenaGrowing;
	template<class T> struct MemoryArenaStd;
	struct MemoryBoundsPolicyNone;
	struct MemoryBoundsPolicySimple;
	struct MemoryTagPolicyNone;
	struct MemoryTagPolicySimple;
	struct MemoryTrackPolicyNone;
	struct MemoryTrackPolicySimple;
	struct MemoryTrackPolicyAdvanced;
	struct MemoryConcurrentPolicyNone;
	struct MemoryConcurrentPolicyMutex;
	struct OutOfMemory;
	class VirtualMemory;
	struct MemoryBuffer;
	struct Disconnected;
	class TcpConnection;
	class TcpServer;
	struct UdpStatistics;
	class UdpConnection;
	class UdpServer;
	struct DiscoveryPeer;
	class DiscoveryClient;
	class DiscoveryServer;
	enum class NoiseTypeEnum : uint32;
	enum class NoiseInterpolationEnum : uint32;
	enum class NoiseFractalTypeEnum : uint32;
	enum class NoiseDistanceEnum : uint32;
	enum class NoiseOperationEnum : uint32;
	class NoiseFunction;
	struct NoiseFunctionCreateConfig;
	template<class T> struct PointerRangeHolder;
	struct ProcessCreateConfig;
	class Process;
	struct RandomGenerator;
	enum class ScheduleTypeEnum : uint32;
	struct ScheduleCreateConfig;
	class Schedule;
	struct SchedulerCreateConfig;
	class Scheduler;
	template<class T> struct ScopeLock;
	struct Serializer;
	struct Deserializer;
	class SpatialQuery;
	class SpatialData;
	struct SpatialDataCreateConfig;
	class BufferIStream;
	class BufferOStream;
	class SwapBufferGuard;
	struct SwapBufferGuardCreateConfig;
	class TextPack;
	class ThreadPool;
	class Timer;
	struct InvalidUtfString;
	template<class T, class F> struct VariableInterpolatingBuffer;
	template<class T, uint32 N = 16> struct VariableSmoothingBuffer;

	enum class SeverityEnum
	{
		Note, // details for subsequent log
		Hint, // possible improvement available
		Warning, // deprecated behavior, dangerous actions
		Info, // we are good, progress report
		Error, // invalid user input, network connection interrupted, file access denied
		Critical // not implemented function, exception inside destructor, assert failure
	};

	namespace privat
	{
		CAGE_CORE_API uint64 makeLog(const char *file, uint32 line, const char *function, SeverityEnum severity, const char *component, const string &message, bool continuous, bool debug) noexcept;
	}

	namespace detail
	{
		CAGE_CORE_API void terminate();
		CAGE_CORE_API void debugOutput(const string &msg);
		CAGE_CORE_API void debugBreakpoint();

		// makes all debugBreakpoint calls be ignored
		struct CAGE_CORE_API OverrideBreakpoint
		{
			explicit OverrideBreakpoint(bool enable = false);
			~OverrideBreakpoint();

		private:
			bool original;
		};

		// make assert failures throw a critical exception instead of terminating the application
		struct CAGE_CORE_API OverrideAssert
		{
			explicit OverrideAssert(bool deadly = false);
			~OverrideAssert();

		private:
			bool original;
		};

		// changes threshold for exception severity for logging
		struct CAGE_CORE_API OverrideException
		{
			explicit OverrideException(SeverityEnum severity = SeverityEnum::Critical);
			~OverrideException();

		private:
			SeverityEnum original;
		};

		CAGE_CORE_API void setGlobalBreakpointOverride(bool enable);
		CAGE_CORE_API void setGlobalAssertOverride(bool enable);
		CAGE_CORE_API void setGlobalExceptionOverride(SeverityEnum severity);
	}

	namespace privat
	{
		struct CAGE_CORE_API AssertPriv
		{
			explicit AssertPriv(bool exp, const char *expt, const char *file, const char *line, const char *function);
			void operator () () const;

#define GCHL_GENERATE(TYPE) AssertPriv &variable(const char *name, TYPE var);
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64, bool, float, double, const char*, \
				const string&, real, rads, degs, const vec2&, const vec3&, const vec4&, const quat&, const mat3&, const mat4&));
#undef GCHL_GENERATE

			template<class U>
			AssertPriv &variable(const char *name, U *var)
			{
				return variable(name, (uintPtr)var);
			}

			template<class U>
			AssertPriv &variable(const char *name, const U &var)
			{
				if (!valid)
					format(name, "<unknown variable type>");
				return *this;
			}

		private:
			const bool valid = false;
			void format(const char *name, const char *var) const;
		};
	}

	struct CAGE_CORE_API Exception
	{
		explicit Exception(const char *file, uint32 line, const char *function, SeverityEnum severity, const char *message) noexcept;
		virtual ~Exception() noexcept;

		void makeLog();
		virtual void log();

		const char *file = nullptr;
		const char *function = nullptr;
		const char *message = nullptr;
		uint32 line = 0;
		SeverityEnum severity = SeverityEnum::Critical;
	};

	struct CAGE_CORE_API NotImplemented : public Exception
	{
		explicit NotImplemented(const char *file, uint32 line, const char *function, SeverityEnum severity, const char *message) noexcept;
		virtual void log();
	};

	struct CAGE_CORE_API SystemError : public Exception
	{
		explicit SystemError(const char *file, uint32 line, const char *function, SeverityEnum severity, const char *message, uint32 code) noexcept;
		virtual void log();
		uint32 code = 0;
	};

	CAGE_CORE_API uint64 getApplicationTime();

	namespace privat
	{
		template<bool ToSig, bool FromSig>
		struct numeric_cast_helper_signed
		{
			template<class To, class From>
			static To cast(From from)
			{
				CAGE_ASSERT(from >= detail::numeric_limits<To>::min(), "numeric cast failure", from, detail::numeric_limits<To>::min());
				CAGE_ASSERT(from <= detail::numeric_limits<To>::max(), "numeric cast failure", from, detail::numeric_limits<To>::max());
				return static_cast<To>(from);
			}
		};

		template<>
		struct numeric_cast_helper_signed<false, true> // signed -> unsigned
		{
			template<class To, class From>
			static To cast(From from)
			{
				CAGE_ASSERT(from >= 0, "numeric cast failure", from);
				typedef typename detail::numeric_limits<From>::make_unsigned unsgFrom;
				CAGE_ASSERT(static_cast<unsgFrom>(from) <= detail::numeric_limits<To>::max(), "numeric cast failure", from, static_cast<unsgFrom>(from), detail::numeric_limits<To>::max());
				return static_cast<To>(from);
			}
		};

		template<>
		struct numeric_cast_helper_signed<true, false> // unsigned -> signed
		{
			template<class To, class From>
			static To cast(From from)
			{
				typedef typename detail::numeric_limits<To>::make_unsigned unsgTo;
				CAGE_ASSERT(from <= static_cast<unsgTo>(detail::numeric_limits<To>::max()), "numeric cast failure", from, detail::numeric_limits<To>::max(), static_cast<unsgTo>(detail::numeric_limits<To>::max()));
				return static_cast<To>(from);
			}
		};

		template<bool Specialized>
		struct numeric_cast_helper_specialized
		{};

		template<>
		struct numeric_cast_helper_specialized<true>
		{
			template<class To, class From>
			static To cast(From from)
			{
				return numeric_cast_helper_signed<detail::numeric_limits<To>::is_signed, detail::numeric_limits<From>::is_signed>::template cast<To>(from);
			}
		};
	}

	// with CAGE_ASSERT_ENABLED numeric_cast validates that the value is in range of the target type, preventing overflows
	// without CAGE_ASSERT_ENABLED numeric_cast is the same as static_cast
	template<class To, class From>
	To numeric_cast(From from)
	{
		return privat::numeric_cast_helper_specialized<detail::numeric_limits<To>::is_specialized && detail::numeric_limits<From>::is_specialized>::template cast<To>(from);
	}

	// with CAGE_ASSERT_ENABLED class_cast verifies that dynamic_cast would succeed
	// without CAGE_ASSERT_ENABLED class_cast is the same as static_cast
	template<class To, class From>
	To class_cast(From from)
	{
		CAGE_ASSERT(!from || dynamic_cast<To>(from), from);
		return static_cast<To>(from);
	}

	namespace privat
	{
#define GCHL_GENERATE(TYPE) CAGE_CORE_API uint32 toString(char *s, TYPE value); CAGE_CORE_API void fromString(const char *s, TYPE &value);
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64, float, double));
#undef GCHL_GENERATE
		CAGE_CORE_API uint32 toString(char *dst, uint32 dstLen, const char *src);

		CAGE_CORE_API void stringEncodeUrl(const char *dataIn, uint32 currentIn, char *dataOut, uint32 &currentOut, uint32 maxLength);
		CAGE_CORE_API void stringDecodeUrl(const char *dataIn, uint32 currentIn, char *dataOut, uint32 &currentOut, uint32 maxLength);
		CAGE_CORE_API void stringReplace(char *data, uint32 &current, uint32 maxLength, const char *what, uint32 whatLen, const char *with, uint32 withLen);
		CAGE_CORE_API void stringTrim(char *data, uint32 &current, const char *what, uint32 whatLen, bool left, bool right);
		CAGE_CORE_API void stringSplit(char *data, uint32 &current, char *ret, uint32 &retLen, const char *what, uint32 whatLen);
		CAGE_CORE_API uint32 stringFind(const char *data, uint32 current, const char *what, uint32 whatLen, uint32 offset);
		CAGE_CORE_API bool stringIsPattern(const char *data, uint32 dataLen, const char *prefix, uint32 prefixLen, const char *infix, uint32 infixLen, const char *suffix, uint32 suffixLen);
		CAGE_CORE_API int stringComparison(const char *ad, uint32 al, const char *bd, uint32 bl);
		CAGE_CORE_API uint32 stringToUpper(char *dst, uint32 dstLen, const char *src, uint32 srcLen);
		CAGE_CORE_API uint32 stringToLower(char *dst, uint32 dstLen, const char *src, uint32 srcLen);
	}

	namespace detail
	{
		CAGE_CORE_API void *memset(void *destination, int value, uintPtr num);
		CAGE_CORE_API void *memcpy(void *destination, const void *source, uintPtr num);
		CAGE_CORE_API void *memmove(void *destination, const void *source, uintPtr num);
		CAGE_CORE_API int memcmp(const void *ptr1, const void *ptr2, uintPtr num);

		template<uint32 N>
		struct StringBase
		{
			// constructors
			StringBase()
			{
				data[current] = 0;
			}

			StringBase(const StringBase &other) noexcept : current(other.current)
			{
				detail::memcpy(data, other.data, current);
				data[current] = 0;
			}

			template<uint32 M>
			StringBase(const StringBase<M> &other)
			{
				if (other.current > N)
					CAGE_THROW_ERROR(Exception, "string truncation");
				current = other.current;
				detail::memcpy(data, other.data, current);
				data[current] = 0;
			}

			explicit StringBase(bool other)
			{
				static_assert(N >= 6, "string too short");
				*this = (other ? "true" : "false");
			}

#define GCHL_GENERATE(TYPE) \
			explicit StringBase(TYPE other)\
			{\
				static_assert(N >= 20, "string too short");\
				current = privat::toString(data, other);\
				data[current] = 0;\
			}
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64, float, double));
#undef GCHL_GENERATE

			template<class T>
			explicit StringBase(T *other) : StringBase((uintPtr)other)
			{}

			explicit StringBase(const char *pos, uint32 len)
			{
				if (len > N)
					CAGE_THROW_ERROR(Exception, "string truncation");
				current = len;
				detail::memcpy(data, pos, len);
				data[current] = 0;
			}

			StringBase(char *other)
			{
				current = privat::toString(data, N, other);
				data[current] = 0;
			}

			StringBase(const char *other)
			{
				current = privat::toString(data, N, other);
				data[current] = 0;
			}

			// assignment operators
			StringBase &operator = (const StringBase &other) noexcept
			{
				if (this == &other)
					return *this;
				current = other.current;
				detail::memcpy(data, other.data, current);
				data[current] = 0;
				return *this;
			}

			// compound operators
			StringBase &operator += (const StringBase &other)
			{
				if (current + other.current > N)
					CAGE_THROW_ERROR(Exception, "string truncation");
				detail::memcpy(data + current, other.data, other.current);
				current += other.current;
				data[current] = 0;
				return *this;
			}

			// binary operators
			StringBase operator + (const StringBase &other) const
			{
				return StringBase(*this) += other;
			}

			char &operator [] (uint32 idx)
			{
				CAGE_ASSERT(idx < current, "index out of range", idx, current, N);
				return data[idx];
			}

			char operator [] (uint32 idx) const
			{
				CAGE_ASSERT(idx < current, "index out of range", idx, current, N);
				return data[idx];
			}

			// comparison operators
#define GCHL_GENERATE(OPERATOR) bool operator OPERATOR (const StringBase &other) const { return privat::stringComparison(data, current, other.data, other.current) OPERATOR 0; }
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, == , != , <= , >= , <, >));
#undef GCHL_GENERATE

			// methods
			const char *c_str() const
			{
				CAGE_ASSERT(data[current] == 0);
				return data;
			}

			StringBase reverse() const
			{
				StringBase ret(*this);
				for (uint32 i = 0; i < current; i++)
					ret.data[current - i - 1] = data[i];
				return ret;
			}

			StringBase subString(uint32 start, uint32 length) const
			{
				if (start >= current)
					return "";
				uint32 len = length;
				if (length == m || start + length > current)
					len = current - start;
				return StringBase(data + start, len);
			}

			StringBase replace(const StringBase &what, const StringBase &with) const
			{
				StringBase ret(*this);
				privat::stringReplace(ret.data, ret.current, N, what.data, what.current, with.data, with.current);
				ret.data[ret.current] = 0;
				return ret;
			}

			StringBase replace(uint32 start, uint32 length, const StringBase &with) const
			{
				return subString(0, start) + with + subString(start + length, m);
			}

			StringBase remove(uint32 start, uint32 length) const
			{
				return subString(0, start) + subString(start + length, m);
			}

			StringBase insert(uint32 start, const StringBase &what) const
			{
				return subString(0, start) + what + subString(start, m);
			}

			StringBase trim(bool left = true, bool right = true, const StringBase &trimChars = "\t\n ") const
			{
				StringBase ret(*this);
				privat::stringTrim(ret.data, ret.current, trimChars.data, trimChars.current, left, right);
				ret.data[ret.current] = 0;
				return ret;
			}

			StringBase split(const StringBase &splitChars = "\t\n ")
			{
				StringBase ret;
				privat::stringSplit(data, current, ret.data, ret.current, splitChars.data, splitChars.current);
				data[current] = 0;
				ret.data[ret.current] = 0;
				return ret;
			}

			StringBase fill(uint32 size, char c = ' ') const
			{
				StringBase cc(&c, 1);
				StringBase ret = *this;
				while (ret.length() < size)
					ret += cc;
				ret.data[ret.current] = 0;
				return ret;
			}

			StringBase toUpper() const
			{
				StringBase ret;
				ret.current = privat::stringToUpper(ret.data, N, data, current);
				ret.data[ret.current] = 0;
				return ret;
			}

			StringBase toLower() const
			{
				StringBase ret;
				ret.current = privat::stringToLower(ret.data, N, data, current);
				ret.data[ret.current] = 0;
				return ret;
			}

			float toFloat() const
			{
				float i;
				privat::fromString(data, i);
				return i;
			}

			double toDouble() const
			{
				double i;
				privat::fromString(data, i);
				return i;
			}

			sint32 toSint32() const
			{
				sint32 i;
				privat::fromString(data, i);
				return i;
			}

			uint32 toUint32() const
			{
				uint32 i;
				privat::fromString(data, i);
				return i;
			}

			sint64 toSint64() const
			{
				sint64 i;
				privat::fromString(data, i);
				return i;
			}

			uint64 toUint64() const
			{
				uint64 i;
				privat::fromString(data, i);
				return i;
			}

			bool toBool() const
			{
				StringBase l = toLower();
				if (l == "false" || l == "f" || l == "no" || l == "n" || l == "off" || l == "0")
					return false;
				if (l == "true" || l == "t" || l == "yes" || l == "y" || l == "on" || l == "1")
					return true;
				CAGE_THROW_ERROR(Exception, "invalid value");
			}

			uint32 find(const StringBase &other, uint32 offset = 0) const
			{
				return privat::stringFind(data, current, other.data, other.current, offset);
			}

			uint32 find(char other, uint32 offset = 0) const
			{
				return privat::stringFind(data, current, &other, 1, offset);
			}

			bool isPattern(const StringBase &prefix, const StringBase &infix, const StringBase &suffix) const
			{
				return privat::stringIsPattern(data, current, prefix.data, prefix.current, infix.data, infix.current, suffix.data, suffix.current);
			}

			bool isDigitsOnly() const
			{
				for (char c : *this)
				{
					if (c < '0' || c > '9')
						return false;
				}
				return true;
			}

			bool isInteger(bool allowSign) const
			{
				if (empty())
					return false;
				if (allowSign && (data[0] == '-' || data[0] == '+'))
					return subString(1, current - 1).isDigitsOnly();
				else
					return isDigitsOnly();
			}

			bool isReal(bool allowSign) const
			{
				if (empty())
					return false;
				uint32 d = find('.');
				if (d == m)
					return isInteger(allowSign);
				StringBase whole = subString(0, d);
				StringBase part = subString(d + 1, m);
				return whole.isInteger(allowSign) && part.isDigitsOnly();
			}

			bool isBool() const
			{
				StringBase l = toLower();
				if (l == "false" || l == "f" || l == "no" || l == "n" || l == "off")
					return true;
				if (l == "true" || l == "t" || l == "yes" || l == "y" || l == "on")
					return true;
				return false;
			}

			StringBase encodeUrl() const
			{
				StringBase ret;
				privat::stringEncodeUrl(data, current, ret.data, ret.current, N);
				ret.data[ret.current] = 0;
				return ret;
			}

			StringBase decodeUrl() const
			{
				StringBase ret;
				privat::stringDecodeUrl(data, current, ret.data, ret.current, N);
				ret.data[ret.current] = 0;
				return ret;
			}

			bool empty() const
			{
				return current == 0;
			}

			uint32 length() const
			{
				return current;
			}

			uint32 size() const
			{
				return current;
			}

			char *begin()
			{
				return data;
			}

			char *end()
			{
				return data + current;
			}

			const char *begin() const
			{
				return data;
			}

			const char *end() const
			{
				return data + current;
			}

			static const uint32 MaxLength = N;

		private:
			char data[N + 1];
			uint32 current = 0;

			template<uint32 M>
			friend struct StringBase;
		};

		template<uint32 N>
		struct StringizerBase
		{
			StringBase<N> value;

			operator const StringBase<N> & () const
			{
				return value;
			}

			template<uint32 M>
			operator const StringBase<M> () const
			{
				return value;
			}
		};

		template<uint32 N, uint32 M>
		StringizerBase<N> &operator + (StringizerBase<N> &str, const StringizerBase<M> &other)
		{
			str.value += other.value;
			return str;
		}

		template<uint32 N, uint32 M>
		StringizerBase<N> &operator + (StringizerBase<N> &str, const StringBase<M> &other)
		{
			str.value += other;
			return str;
		}

		template<uint32 N>
		StringizerBase<N> &operator + (StringizerBase<N> &str, const char *other)
		{
			str.value += other;
			return str;
		}

		template<uint32 N>
		StringizerBase<N> &operator + (StringizerBase<N> &str, char *other)
		{
			str.value += other;
			return str;
		}

		template<uint32 N, class T>
		StringizerBase<N> &operator + (StringizerBase<N> &str, T *other)
		{
			return str + (uintPtr)other;
		}

#define GCHL_GENERATE(TYPE) \
		template<uint32 N> \
		inline StringizerBase<N> &operator + (StringizerBase<N> &str, TYPE other) \
		{ \
			return str + StringBase<20>(other); \
		}
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64, float, double, bool));
#undef GCHL_GENERATE

		template<uint32 N, class T>
		inline StringizerBase<N> &operator + (StringizerBase<N> &&str, const T &other)
		{
			// allow to use l-value-reference operator overloads with r-value-reference stringizer
			return str + other;
		}

		template<uint32 N>
		struct StringComparatorFastBase
		{
			bool operator () (const StringBase<N> &a, const StringBase<N> &b) const noexcept
			{
				if (a.length() == b.length())
					return detail::memcmp(a.begin(), b.begin(), a.length()) < 0;
				return a.length() < b.length();
			}
		};
	}

	typedef detail::StringBase<1000> string;
	typedef detail::StringizerBase<1000> stringizer;
	typedef detail::StringComparatorFastBase<1000> stringComparatorFast;

	// delegates

	template<class T>
	struct Delegate;

	template<class R, class... Ts>
	struct Delegate<R(Ts...)>
	{
	private:
		R(*fnc)(void *, Ts...) = nullptr;
		void *inst = nullptr;

	public:
		template<R(*F)(Ts...)>
		Delegate &bind() noexcept
		{
			fnc = +[](void *inst, Ts... vs) {
				(void)inst;
				return F(templates::forward<Ts>(vs)...);
			};
			return *this;
		}

		template<class D, R(*F)(D, Ts...)>
		Delegate &bind(D d) noexcept
		{
			static_assert(sizeof(d) <= sizeof(inst), "invalid size of data stored in delegate");
			union U
			{
				void *p;
				D d;
			};
			fnc = +[](void *inst, Ts... vs) {
				U u;
				u.p = inst;
				return F(u.d , templates::forward<Ts>(vs)...);
			};
			U u;
			u.d = d;
			inst = u.p;
			return *this;
		}

		template<class C, R(C::*F)(Ts...)>
		Delegate &bind(C *i) noexcept
		{
			fnc = +[](void *inst, Ts... vs) {
				return (((C*)inst)->*F)(templates::forward<Ts>(vs)...);
			};
			inst = i;
			return *this;
		}

		template<class C, R(C::*F)(Ts...) const>
		Delegate &bind(const C *i) noexcept
		{
			fnc = +[](void *inst, Ts... vs) {
				return (((const C*)inst)->*F)(templates::forward<Ts>(vs)...);
			};
			inst = const_cast<C*>(i);
			return *this;
		}

		explicit operator bool() const noexcept
		{
			return !!fnc;
		}

		void clear() noexcept
		{
			inst = nullptr;
			fnc = nullptr;
		}

		R operator ()(Ts... vs) const
		{
			CAGE_ASSERT(!!*this, inst, fnc);
			return fnc(inst, templates::forward<Ts>(vs)...);
		}

		bool operator == (const Delegate &other) const noexcept
		{
			return fnc == other.fnc && inst == other.inst;
		}

		bool operator != (const Delegate &other) const noexcept
		{
			return !(*this == other);
		}
	};

	// events

	template<class>
	struct EventListener;
	template<class>
	struct EventDispatcher;

	namespace privat
	{
		struct CAGE_CORE_API EventLinker : private Immovable
		{
			explicit EventLinker(const string &name);
			~EventLinker();
			void attach(EventLinker *d, sint32 order);
			void detach();
			void logAllNames();
			EventLinker *p = nullptr, *n = nullptr;
		private:
			void unlink();
			bool valid() const;
			sint32 order;
			const detail::StringBase<60> name;
		};
	}

	template<class... Ts>
	struct EventListener<bool(Ts...)> : private privat::EventLinker, private Delegate<bool(Ts...)>
	{
		explicit EventListener(const string &name = "listener") : privat::EventLinker(name)
		{}

		void attach(EventDispatcher<bool(Ts...)> &dispatcher, sint32 order = 0)
		{
			privat::EventLinker::attach(&dispatcher, order);
		}

		using privat::EventLinker::detach;
		using Delegate<bool(Ts...)>::bind;
		using Delegate<bool(Ts...)>::clear;

	private:
		bool invoke(Ts... vs) const
		{
			auto &d = (Delegate<bool(Ts...)>&)*this;
			if (d)
				return d(templates::forward<Ts>(vs)...);
			return false;
		}

		using privat::EventLinker::logAllNames;
		friend struct EventDispatcher<bool(Ts...)>;
	};

	template<class... Ts>
	struct EventListener<void(Ts...)> : private EventListener<bool(Ts...)>, private Delegate<void(Ts...)>
	{
		explicit EventListener(const string &name = "listener") : EventListener<bool(Ts...)>(name)
		{
			EventListener<bool(Ts...)>::template bind<EventListener, &EventListener::invoke>(this);
		}

		using EventListener<bool(Ts...)>::attach;
		using EventListener<bool(Ts...)>::detach;
		using Delegate<void(Ts...)>::bind;
		using Delegate<void(Ts...)>::clear;

	private:
		bool invoke(Ts... vs) const
		{
			auto &d = (Delegate<void(Ts...)>&)*this;
			if (d)
				d(templates::forward<Ts>(vs)...);
			return false;
		}
	};

	template<class... Ts>
	struct EventDispatcher<bool(Ts...)> : private EventListener<bool(Ts...)>
	{
		explicit EventDispatcher(const string &name = "dispatcher") : EventListener<bool(Ts...)>(name)
		{}

		bool dispatch(Ts... vs) const
		{
			CAGE_ASSERT(!this->p);
			const privat::EventLinker *l = this->n;
			while (l)
			{
				if (static_cast<const EventListener<bool(Ts...)>*>(l)->invoke(templates::forward<Ts>(vs)...))
					return true;
				l = l->n;
			}
			return false;
		}

		void attach(EventListener<bool(Ts...)> &listener, sint32 order = 0)
		{
			listener.attach(*this, order);
		}

		void attach(EventListener<void(Ts...)> &listener, sint32 order = 0)
		{
			listener.attach(*this, order);
		}

		using EventListener<bool(Ts...)>::detach;
		using EventListener<bool(Ts...)>::logAllNames;

	private:
		friend struct EventListener<bool(Ts...)>;
	};

	// holder

	template<class T> struct Holder;

	namespace privat
	{
		template<class R, class S>
		struct HolderCaster
		{
			R *operator () (S *p) const
			{
				if (!p)
					return nullptr;
				R *r = dynamic_cast<R*>(p);
				if (!r)
					CAGE_THROW_ERROR(Exception, "bad cast");
				return r;
			}
		};

		template<class R>
		struct HolderCaster<R, void>
		{
			R *operator () (void *p) const
			{
				return (R*)p;
			}
		};

		template<class S>
		struct HolderCaster<void, S>
		{
			void *operator () (S *p) const
			{
				return (void*)p;
			}
		};

		template<>
		struct HolderCaster<void, void>
		{
			void *operator () (void *p) const
			{
				return p;
			}
		};

		template<class T>
		struct HolderDereference
		{
			typedef T &type;
		};

		template<>
		struct HolderDereference<void>
		{
			typedef void type;
		};

		CAGE_CORE_API bool isHolderShareable(const Delegate<void(void *)> &deleter);
		CAGE_CORE_API void incHolderShareable(void *ptr, const Delegate<void(void *)> &deleter);
		CAGE_CORE_API void makeHolderShareable(void *&ptr, Delegate<void(void *)> &deleter);

		template<class T>
		struct HolderBase
		{
			HolderBase() {}
			explicit HolderBase(T *data, void *ptr, Delegate<void(void*)> deleter) : deleter_(deleter), ptr_(ptr), data_(data) {}

			HolderBase(const HolderBase &) = delete;
			HolderBase(HolderBase &&other) noexcept
			{
				deleter_ = other.deleter_;
				ptr_ = other.ptr_;
				data_ = other.data_;
				other.deleter_.clear();
				other.ptr_ = nullptr;
				other.data_ = nullptr;
			}

			HolderBase &operator = (const HolderBase &) = delete;
			HolderBase &operator = (HolderBase &&other) noexcept
			{
				if (this == &other)
					return *this;
				if (deleter_)
					deleter_(ptr_);
				deleter_ = other.deleter_;
				ptr_ = other.ptr_;
				data_ = other.data_;
				other.deleter_.clear();
				other.ptr_ = nullptr;
				other.data_ = nullptr;
				return *this;
			}

			~HolderBase()
			{
				data_ = nullptr;
				void *tmpPtr = ptr_;
				Delegate<void(void *)> tmpDeleter = deleter_;
				ptr_ = nullptr;
				deleter_.clear();
				// calling the deleter is purposefully deferred to until this holder is cleared first
				if (tmpDeleter)
					tmpDeleter(tmpPtr);
			}

			explicit operator bool() const noexcept
			{
				return !!data_;
			}

			T *operator -> () const
			{
				CAGE_ASSERT(data_, "data is null");
				return data_;
			}

			typename HolderDereference<T>::type operator * () const
			{
				CAGE_ASSERT(data_, "data is null");
				return *data_;
			}

			T *get() const
			{
				return data_;
			}

			void clear()
			{
				if (deleter_)
					deleter_(ptr_);
				deleter_.clear();
				ptr_ = nullptr;
				data_ = nullptr;
			}

			bool isShareable() const
			{
				return isHolderShareable(deleter_);
			}

			Holder<T> share() const
			{
				incHolderShareable(ptr_, deleter_);
				return Holder<T>(data_, ptr_, deleter_);
			}

			Holder<T> makeShareable() &&
			{
				Holder<T> tmp(data_, ptr_, deleter_);
				makeHolderShareable(tmp.ptr_, tmp.deleter_);
				deleter_.clear();
				ptr_ = nullptr;
				data_ = nullptr;
				return tmp;
			}

		protected:
			Delegate<void(void *)> deleter_;
			void *ptr_ = nullptr; // pointer to deallocate
			T *data_ = nullptr; // pointer to the object (may differ in case of classes with inheritance)
		};
	}

	template<class T>
	struct Holder : public privat::HolderBase<T>
	{
		using privat::HolderBase<T>::HolderBase;

		template<class M>
		Holder<M> cast() &&
		{
			Holder<M> tmp(privat::HolderCaster<M, T>()(this->data_), this->ptr_, this->deleter_);
			this->deleter_.clear();
			this->ptr_ = nullptr;
			this->data_ = nullptr;
			return tmp;
		}
	};

	// pointer range

	template<class T>
	struct PointerRange
	{
	private:
		T *begin_;
		T *end_;

	public:
		typedef uintPtr size_type;

		PointerRange() : begin_(nullptr), end_(nullptr) {}
		template<class U>
		PointerRange(U *begin, U *end) : begin_(begin), end_(end) {}
		template<class U>
		PointerRange(const PointerRange<U> &other) : begin_(other.begin()), end_(other.end()) {}
		template<class U>
		PointerRange(U &&other) : begin_(other.data()), end_(other.data() + other.size()) {}

		T *begin() const { return begin_; }
		T *end() const { return end_; }
		T *data() const { return begin_; }
		size_type size() const { return end_ - begin_; }
		bool empty() const { return size() == 0; }
		T &operator[] (uint32 idx) const { CAGE_ASSERT(idx < size(), idx, size()); return begin_[idx]; }
	};

	template<class T>
	struct Holder<PointerRange<T>> : public privat::HolderBase<PointerRange<T>>
	{
		using privat::HolderBase<PointerRange<T>>::HolderBase;
		typedef typename PointerRange<T>::size_type size_type;

		operator Holder<PointerRange<const T>> () &&
		{
			Holder<PointerRange<const T>> tmp((PointerRange<const T>*)this->data_, this->ptr_, this->deleter_);
			this->deleter_.clear();
			this->ptr_ = nullptr;
			this->data_ = nullptr;
			return tmp;
		}

		T *begin() const { return this->data_->begin(); }
		T *end() const { return this->data_->end(); }
		T *data() const { return begin(); }
		size_type size() const { return end() - begin(); }
		bool empty() const { return size() == 0; }
		T &operator[] (uint32 idx) const { CAGE_ASSERT(idx < size(), idx, size()); return begin()[idx]; }
	};

	// memory arena

	namespace privat
	{
		struct CageNewTrait
		{};
	}
}

inline void *operator new(cage::uintPtr size, void *ptr, cage::privat::CageNewTrait) noexcept
{
	return ptr;
}

inline void *operator new[](cage::uintPtr size, void *ptr, cage::privat::CageNewTrait) noexcept
{
	return ptr;
}

inline void operator delete(void *ptr, void *ptr2, cage::privat::CageNewTrait) noexcept {}
inline void operator delete[](void *ptr, void *ptr2, cage::privat::CageNewTrait) noexcept {}

namespace cage
{
	struct CAGE_CORE_API MemoryArena
	{
	private:
		struct Stub
		{
			void *(*alloc)(void *, uintPtr, uintPtr);
			void(*dealloc)(void *, void *);
			void(*fls)(void *);

			template<class A>
			static void *allocate(void *inst, uintPtr size, uintPtr alignment)
			{
				return ((A*)inst)->allocate(size, alignment);
			}

			template<class A>
			static void deallocate(void *inst, void *ptr) noexcept
			{
				((A*)inst)->deallocate(ptr);
			}

			template<class A>
			static void flush(void *inst) noexcept
			{
				((A*)inst)->flush();
			}

			template<class A>
			static Stub init() noexcept
			{
				Stub s;
				s.alloc = &allocate<A>;
				s.dealloc = &deallocate<A>;
				s.fls = &flush<A>;
				return s;
			}
		} *stub = nullptr;
		void *inst = nullptr;

	public:
		MemoryArena() noexcept;

		template<class A>
		explicit MemoryArena(A *a) noexcept
		{
			static Stub stb = Stub::init<A>();
			this->stub = &stb;
			inst = a;
		}

		MemoryArena(const MemoryArena &) = default;
		MemoryArena(MemoryArena &&) = default;
		MemoryArena &operator = (const MemoryArena &) = default;
		MemoryArena &operator = (MemoryArena &&) = default;

		void *allocate(uintPtr size, uintPtr alignment);
		void deallocate(void *ptr) noexcept;
		void flush() noexcept;

		template<class T, class... Ts>
		T *createObject(Ts... vs)
		{
			void *ptr = allocate(sizeof(T), alignof(T));
			try
			{
				return new(ptr, privat::CageNewTrait()) T(templates::forward<Ts>(vs)...);
			}
			catch (...)
			{
				deallocate(ptr);
				throw;
			}
		}

		template<class T, class... Ts>
		Holder<T> createHolder(Ts... vs)
		{
			Delegate<void(void*)> d;
			d.bind<MemoryArena, &MemoryArena::destroy<T>>(this);
			T *p = createObject<T>(templates::forward<Ts>(vs)...);
			return Holder<T>(p, p, d);
		};

		template<class Interface, class Impl, class... Ts>
		Holder<Interface> createImpl(Ts... vs)
		{
			return createHolder<Impl>(templates::forward<Ts>(vs)...).template cast<Interface>();
		};

		template<class T>
		void destroy(void *ptr) noexcept
		{
			if (!ptr)
				return;
			try
			{
				((T*)ptr)->~T();
			}
			catch (...)
			{
				CAGE_ASSERT(false, "exception thrown in destructor");
				detail::terminate();
			}
			deallocate(ptr);
		}

		bool operator == (const MemoryArena &other) const noexcept;
		bool operator != (const MemoryArena &other) const noexcept;
	};

	namespace detail
	{
		CAGE_CORE_API MemoryArena &systemArena();
	}
}


#endif // guard_core_h_39243ce0_71a5_4900_8898_63fb89591b7b_
