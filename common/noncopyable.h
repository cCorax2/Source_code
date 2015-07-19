#ifndef INC_METIN_II_COMMON_NONCOPYABLE_TEMPLATE
#define INC_METIN_II_COMMON_NONCOPYABLE_TEMPLATE

class noncopyable
{
	protected:
		noncopyable() {}
		~noncopyable() {}

	private:
		noncopyable(const noncopyable &);
		noncopyable& operator = (const noncopyable &);
};

#endif
