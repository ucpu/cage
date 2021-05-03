#ifndef guard_entitiesVisitor_h_m1nb54v6sre8t
#define guard_entitiesVisitor_h_m1nb54v6sre8t

#include <cage-core/entities.h>

#include <tuple>

namespace cage
{
	namespace privat
	{
		template<class>
		struct DecomposeLambda;

		template<class R, class T, class... Args>
		struct DecomposeLambda<R (T::*)(Args...) const>
		{
			using Return = R;
			using Class = T;
			using Params = std::tuple<Args...>;
		};

		template<class Lambda>
		struct LambdaParamsPack
		{
			using Params = typename DecomposeLambda<decltype(&Lambda::operator())>::Params;
		};

		template<class Types, std::size_t I>
		CAGE_FORCE_INLINE void fillComponentsArray(EntityManager *ents, EntityComponent *components[])
		{
			using T = std::tuple_element_t<I, Types>;
			using D = std::decay_t<T>;
			static_assert(std::is_same_v<T, D &> || std::is_same_v<T, const D &>);
			components[I] = ents->component<D>();
			if (!components[I])
				CAGE_THROW_CRITICAL(Exception, "undefined entities component for type");
		}

		template<class Types, std::size_t... I>
		CAGE_FORCE_INLINE void fillComponentsArray(EntityManager *ents, EntityComponent *components[], std::index_sequence<I...>)
		{
			((fillComponentsArray<Types, I>(ents, components)), ...);
		}

		template<class Visitor, class Types, std::size_t... I>
		CAGE_FORCE_INLINE void invokeVisitor(Visitor &visitor, EntityComponent *components[], Entity *e, std::index_sequence<I...>)
		{
			visitor(((e->value<std::decay_t<std::tuple_element_t<I, Types>>>(components[I])))...);
		}
	}

	template<class Visitor>
	void visit(EntityManager *ents, Visitor &&visitor)
	{
		using Types = typename privat::LambdaParamsPack<Visitor>::Params;
		constexpr uint32 cnt = std::tuple_size_v<Types>;

		EntityComponent *components[cnt] = {};
		privat::fillComponentsArray<Types>(ents, components, std::make_index_sequence<cnt>());

		for (Entity *e : ents->entities())
		{
			bool ok = true;
			for (EntityComponent *c : components)
				ok = ok && e->has(c);
			if (!ok)
				continue;
			privat::invokeVisitor<Visitor, Types>(visitor, components, e, std::make_index_sequence<cnt>());
		}
	}
}

#endif // guard_entitiesVisitor_h_m1nb54v6sre8t
