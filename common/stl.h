#ifndef __INC_METIN_II_STL_H__
#define __INC_METIN_II_STL_H__

#include <vector>
#include <string>
#include <map>
#include <list>
#include <functional>
#include <stack>
#include <set>
#ifdef __GNUC__
#include <ext/functional>
#endif

#ifndef itertype
#define itertype(v) typeof((v).begin())
#endif

inline void stl_lowers(std::string& rstRet)
{
	for (size_t i = 0; i < rstRet.length(); ++i)
		rstRet[i] = tolower(rstRet[i]);
}

struct stringhash       
{
	size_t operator () (const std::string & str) const
	{
		const unsigned char * s = (const unsigned char*) str.c_str();
		const unsigned char * end = s + str.size();
		size_t h = 0;

		while (s < end)
		{
			h *= 16777619;
			h ^= *(s++);
		}

		return h;
	}
};

// code from tr1/functional_hash.h
template<typename T>
struct hash;

template<typename _Tp>
struct hash<_Tp*>
: public std::unary_function<_Tp*, std::size_t>
{
	std::size_t
		operator()(_Tp* __p) const
		{ return reinterpret_cast<std::size_t>(__p); }
};

namespace std
{
	template <class container, class Pred>
		void erase_if (container & a, typename container::iterator first, typename container::iterator past, Pred pred)
		{
			while (first != past)
				if (pred(*first))
					a.erase(first++);
				else
					++first;
		}

	template <class container>
		void wipe(container & a)
		{
			typename container::iterator first, past;

			first = a.begin();
			past = a.end();

			while (first != past)
				delete *(first++);

			a.clear();
		}

	template <class container>
		void wipe_second(container & a)
		{
			typename container::iterator first, past;

			first = a.begin();
			past = a.end();

			while (first != past)
			{
				delete first->second;
				++first;
			}

			a.clear();
		}

	template <typename T> T MIN(T a, T b)
	{
		return a < b ? a : b;
	}

	template <typename T> T MAX(T a, T b)
	{
		return a > b ? a : b;
	}

	template <typename T> T MINMAX(T min, T value, T max)
	{
		T tv;

		tv = (min > value ? min : value);
		return (max < tv) ? max : tv;
	}

	template <class _Ty>
		class void_mem_fun_t : public unary_function<_Ty *, void>
		{
			public:
				explicit void_mem_fun_t(void (_Ty::*_Pm)()) : _Ptr(_Pm)
				{
				}

				void operator()(_Ty* p) const
				{
					((p->*_Ptr)());
				}

			private:
				void (_Ty::*_Ptr)();
		};

	template<class _Ty> inline
		void_mem_fun_t<_Ty> void_mem_fun(void (_Ty::*_Pm)())
		{ return (void_mem_fun_t<_Ty>(_Pm)); }

	template<class _Ty>
		class void_mem_fun_ref_t : public unary_function<_Ty, void>
		{
			public:
				explicit void_mem_fun_ref_t(void (_Ty::*_Pm)()) : _Ptr(_Pm) {}
				void operator()(_Ty& x) const
				{ return ((x.*_Ptr)()); }
			private:
				void (_Ty::*_Ptr)();
		};

	template<class _Ty> inline
		void_mem_fun_ref_t<_Ty> void_mem_fun_ref(void (_Ty::*_Pm)())
		{ return (void_mem_fun_ref_t< _Ty>(_Pm)); }
};

#endif
