/*
 * The hhev library.
 * Copyright (C) 2018, Aleksei Shashev <ashashev@gmail.com>
 */

#ifndef __PIPELINE_HPP__
#define __PIPELINE_HPP__

#include <tuple>
#include <functional>

namespace hhev
{

namespace pipeline
{

namespace details
{

template<typename T>
struct FuncTraits;

template<typename R, typename ... A>
struct FuncTraits<R(*) (A...)>
{
    using Ret = R;
    using Args = std::tuple<A...>;
};

template<typename R, typename O, typename ... A>
struct FuncTraits<R (O::*) (A...)>: public FuncTraits<R (*) (A...)>
{};

template<typename R, typename O, typename ... A>
struct FuncTraits<R (O::*) (A...) const>: public FuncTraits<R (*) (A...)>
{};

template<typename T>
struct FuncTraits: FuncTraits<decltype(&T::operator())>
{};

template<typename T>
struct FuncTraits<T&>: FuncTraits<T>
{};

template<typename T>
struct FuncTraits<T&&>: FuncTraits<T>
{};

template<typename Context, typename R>
class Finalizer
{
public:
    using Op = std::function<R (Context&)>;
    Finalizer(const Op& op)
        : op_(op)
    {}

    R operator()(Context& c) const
    {
        return op_(c);
    }
private:
    Op op_;
};


template<typename Context>
class PipelineControl
{
public:
    using Op = std::function<bool (Context&)>;
    PipelineControl(Context& c)
        : context_(c)
        , good_(true)
    {}

    PipelineControl operator | (const Op& op) const
    {
        if (good_)
            good_ = op(context_);

        return *this;
    }

    template<typename R>
    R operator | (const Finalizer<Context, R>& op) const
    {
        return op(context_);
    }
private:
    Context& context_;
    mutable bool good_;
};

} /* namespace details */

template<typename Context>
details::PipelineControl<Context> start(Context& c)
{
    return details::PipelineControl<Context>(c);
}

template<typename F>
auto finish(F&& op)
{
    using Traits = details::FuncTraits<F>;
    using Context = std::decay_t<std::tuple_element_t<0, typename Traits::Args>>;
    return details::Finalizer<Context, typename Traits::Ret>(op);
}

} /* namespace pipeline */

} /* namespace hhev */

#endif //!__PIPELINE_HPP__
