/*
 * The hhev library.
 * Copyright (C) 2018, Aleksei Shashev <ashashev@gmail.com>
 */

#ifndef __PARAMS_SETTERS_HPP__
#define __PARAMS_SETTERS_HPP__

#include <tuple>

namespace hhev
{

namespace params_setters
{

namespace details
{

template<typename O, typename R, typename ... Args>
class SetterFactory
{
public:
    using MPtr = R (O::*)(Args...);

    class Setter
    {
    public:
        using TupleArgs = std::tuple<Args...>;

        Setter(MPtr ptr, TupleArgs&& args)
            : ptr_(ptr)
            , args_(std::move(args))
        {}

        void operator()(O* o)
        {
            apply(o, std::make_index_sequence<sizeof...(Args)>{});
        }
    private:
        template<std::size_t ... I>
        void apply(O* o, std::index_sequence<I...>)
        {
            (o->*ptr_)(std::get<I>(args_)...);
        }

        MPtr ptr_;
        TupleArgs args_;
    };

    SetterFactory(MPtr ptr)
        : ptr_(ptr)
    {}

    Setter operator=(typename Setter::TupleArgs&& args) const
    {
        return Setter(ptr_, std::move(args));
    }

    Setter operator=(std::tuple_element_t<0, typename Setter::TupleArgs> v) const
    {
        return Setter(ptr_, std::forward_as_tuple(v));
    }

private:
    const MPtr ptr_;
};

template<typename T>
struct Setter;

template<typename R, typename O, typename ... Args>
struct Setter<R (O::*) (Args ...)>
{
    using Type = typename details::SetterFactory<O, R, Args...>::Setter;
};

template<typename R, typename O, typename ... Args>
struct Setter<R (O::*) (Args ...) const>
{
    using Type = typename details::SetterFactory<O, R, Args...>::Setter;
};

} /* namespace details */

template<typename O, typename R, typename ... Args>
auto makeSetterFactory(R (O::* ptr)(Args...))
{
    return details::SetterFactory<O, R, Args...>(ptr);
}

template<typename O>
void applySetters(O*)
{
}

template<typename O, typename Setter, typename ... Setters>
void applySetters(O* o, Setter&& setter, Setters&& ... setters)
{
    std::forward<Setter>(setter)(o);
    applySetters(o, std::forward<Setters>(setters)...);
}

template<typename T>
using Setter = typename details::Setter<T>::Type;

} /* namespace params_setters */

} /* namespace hhev */

#endif //!__PARAMS_SETTERS_HPP__
