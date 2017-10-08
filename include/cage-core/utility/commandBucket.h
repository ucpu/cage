#ifndef commandBucket_h__
#define commandBucket_h__

namespace cage
{
	struct CAGE_API commandBucket
	{
	private:
		memoryArena arena;
		struct cmdNode
		{
			cmdNode *next;
			void *inst;
			void(*fnc)(void*);

			cmdNode();
			void invoke() const { fnc(inst); }
		} *first, *last;

		void addNode();
		template<class C, void (C::*F)()> static void stub(void *inst) { (((C*)inst)->*F)(); }

#define GCHL_GENERATE_HELPER(X) CAGE_JOIN (P, X) CAGE_JOIN (V, X);
#define GCHL_GENERATE_CMDVALS(N) \
		template<class R CAGE_COMMA (N) CAGE_LIST (N, class P), R (*F)(CAGE_LIST (N, P))> struct CAGE_JOIN (cmdVals, N) \
		{ \
		CAGE_REPEAT (N, GCHL_GENERATE_HELPER); \
		void invoke () { F (CAGE_LIST (N, V)); } \
		}; \
		template<class C, class R CAGE_COMMA (N) CAGE_LIST (N, class P), R (C::*F)(CAGE_LIST (N, P))> struct CAGE_JOIN (cmdValsC, N) \
		{ \
		C *inst; \
		CAGE_REPEAT (N, GCHL_GENERATE_HELPER); \
		void invoke () { (inst->*F) (CAGE_LIST (N, V)); } \
		}; \
		template<class C, class R CAGE_COMMA (N) CAGE_LIST (N, class P), R (C::*F)(CAGE_LIST (N, P)) const> struct CAGE_JOIN (cmdValsCC, N) \
		{ \
		const C *inst; \
		CAGE_REPEAT (N, GCHL_GENERATE_HELPER); \
		void invoke () { (inst->*F) (CAGE_LIST (N, V)); } \
		};
		CAGE_EVAL_SMALL(GCHL_GENERATE_CMDVALS(0));
		CAGE_EVAL_MEDIUM(CAGE_REPEAT(20, GCHL_GENERATE_CMDVALS));
#undef GCHL_GENERATE_CMDVALS
#undef GCHL_GENERATE_HELPER

	public:
		commandBucket(memoryArena &arena);
		~commandBucket();
		void dispatch();
		void flush();

#define GCHL_GENERATE_HELPER(X) CAGE_JOIN (v->V, X) = CAGE_JOIN (A, X);
#define GCHL_GENERATE_ADD(N) \
		template<class R CAGE_COMMA (N) CAGE_LIST (N, class P), R (*F)(CAGE_LIST (N, P))> void addFunction (CAGE_LIST_2 (N, P, A)) \
		{ \
		addNode(); \
		CAGE_JOIN (cmdVals, N) <R CAGE_COMMA (N) CAGE_LIST (N, P), F> *v = arena.createObject <CAGE_JOIN (cmdVals, N) <R CAGE_COMMA (N) CAGE_LIST (N, P), F> > (); \
		CAGE_REPEAT (N, GCHL_GENERATE_HELPER); \
		last->inst = v; \
		last->fnc = &stub <CAGE_JOIN (cmdVals, N) <R CAGE_COMMA (N) CAGE_LIST (N, P), F>, & CAGE_JOIN (cmdVals, N) <R CAGE_COMMA (N) CAGE_LIST (N, P), F>::invoke>; \
		} \
		template<class C, class R CAGE_COMMA (N) CAGE_LIST (N, class P), R (C::*F)(CAGE_LIST (N, P))> void addMethod (C *inst CAGE_COMMA (N) CAGE_LIST_2 (N, P, A)) \
		{ \
		addNode(); \
		CAGE_JOIN (cmdValsC, N) <C, R CAGE_COMMA (N) CAGE_LIST (N, P), F> *v = arena.createObject <CAGE_JOIN (cmdValsC, N) <C , R CAGE_COMMA (N) CAGE_LIST (N, P), F> > (); \
		v->inst = inst; \
		CAGE_REPEAT (N, GCHL_GENERATE_HELPER); \
		last->inst = v; \
		last->fnc = &stub <CAGE_JOIN (cmdValsC, N) <C, R CAGE_COMMA (N) CAGE_LIST (N, P), F>, & CAGE_JOIN (cmdValsC, N) <C, R CAGE_COMMA (N) CAGE_LIST (N, P), F>::invoke>; \
		} \
		template<class C, class R CAGE_COMMA (N) CAGE_LIST (N, class P), R (C::*F)(CAGE_LIST (N, P)) const> void addConst (const C *inst CAGE_COMMA (N) CAGE_LIST_2 (N, P, A)) \
		{ \
		addNode(); \
		CAGE_JOIN (cmdValsCC, N) <C, R CAGE_COMMA (N) CAGE_LIST (N, P), F> *v = arena.createObject <CAGE_JOIN (cmdValsCC, N) <C, R CAGE_COMMA (N) CAGE_LIST (N, P), F> > (); \
		v->inst = inst; \
		CAGE_REPEAT (N, GCHL_GENERATE_HELPER); \
		last->inst = v; \
		last->fnc = &stub <CAGE_JOIN (cmdValsCC, N) <C, R CAGE_COMMA (N) CAGE_LIST (N, P), F>, & CAGE_JOIN (cmdValsCC, N) <C, R CAGE_COMMA (N) CAGE_LIST (N, P), F>::invoke>; \
		}
		CAGE_EVAL_SMALL(GCHL_GENERATE_ADD(0));
		CAGE_EVAL_MEDIUM(CAGE_REPEAT(10, GCHL_GENERATE_ADD));
#undef GCHL_GENERATE_ADD
#undef GCHL_GENERATE_HELPER
	};
}

#endif // commandBucket_h__
