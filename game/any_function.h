// See http://www.boost.org/libs/any for Documentation.

#ifndef __IPKN_ANY_FUNCTION_VARIATION_OF_BOOST_ANY_INCLUDED
#define __IPKN_ANY_FUNCTION_VARIATION_OF_BOOST_ANY_INCLUDED

// what:  variant type boost::any
// who:   contributed by Kevlin Henney,
//        with features contributed and bugs found by
//        Ed Brey, Mark Rodgers, Peter Dimov, and James Curran
// when:  July 2001
// where: tested with BCC 5.5, MSVC 6.0, and g++ 2.95

#include <algorithm>
#include <typeinfo>

#define boost _boost_func_of_SQLMsg
#define func_arg_type SQLMsg*
#define func_arg pmsg
#include "any_function.inc"
#undef func_arg
#undef func_arg_type
#undef boost

typedef _boost_func_of_SQLMsg::any any_function;

#define boost _boost_func_of_void
#define func_arg_type 
#define func_arg 
#include "any_function.inc"
#undef func_arg
#undef func_arg_type
#undef boost

typedef _boost_func_of_void::any any_void_function;

template <class F>
class void_binder
{
	protected:
		F f;
		typename F::argument_type value;
	public:
		void_binder(const F& f, const typename F::argument_type x)
			: f(f), value(x) {}
		void operator()() const {
			return f(value);
		}
};

	template <class F, class Arg> 
inline void_binder<F> void_bind(const F& f, const Arg& arg)
{
	typedef typename F::argument_type arg_type;
	return void_binder<F>(f, arg_type(arg));
}

// Copyright Kevlin Henney, 2000, 2001, 2002. All rights reserved.
//
// Permission to use, copy, modify, and distribute this software for any
// purpose is hereby granted without fee, provided that this copyright and
// permissions notice appear in all copies and derivatives.
//
// This software is provided "as is" without express or implied warranty.

#endif


