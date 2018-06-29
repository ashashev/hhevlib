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
        Setter(MPtr ptr, std::tuple<Args&&...>&& args)
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
        std::tuple<Args&&...> args_;
    };

    SetterFactory(MPtr ptr)
        : ptr_(ptr)
    {}

    Setter operator=(std::tuple<Args&& ...>&& args) const
    {
        return Setter(ptr_, std::move(args));
    }

    template<typename T>
    Setter operator=(T&& v) const
    {
        return Setter(ptr_, std::forward_as_tuple(std::forward<T>(v)));
    }

private:
    const MPtr ptr_;
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

} /* namespace params_setters */

} /* namespace hhev */

#endif //!__PARAMS_SETTERS_HPP__
