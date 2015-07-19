#ifndef __INC_METIN_II_AFFECT_FLAG_H__
#define __INC_METIN_II_AFFECT_FLAG_H__

#ifndef IS_SET
#define IS_SET(flag, bit)                ((flag) & (bit))
#endif

#ifndef SET_BIT
#define SET_BIT(var, bit)                ((var) |= (bit))
#endif

#ifndef REMOVE_BIT
#define REMOVE_BIT(var, bit)             ((var) &= ~(bit))
#endif

#ifndef TOGGLE_BIT
#define TOGGLE_BIT(var, bit)             ((var) = (var) ^ (bit))
#endif

struct TAffectFlag
{
	DWORD bits[2];

	inline TAffectFlag() { bits[0] = 0; bits[1] = 0; }
	inline TAffectFlag(DWORD v1, DWORD v2 = 0) {bits[0] = v1; bits[1] = v2;}

	inline bool IsSet(int flag) const
	{ 
		if (AFF_BITS_MAX <= flag || 0 >= flag)
			return false;

		return IS_SET(bits[(flag - 1) >> 5], (((DWORD)1) << ((flag - 1) & 31))); 
	}

	inline void Set(int flag)
	{
		if (AFF_BITS_MAX <= flag || 0 >= flag)
			return;

		SET_BIT(bits[(flag-1)>>5], (((DWORD)1)<<((flag-1)&31))); 
	}

	inline void Reset(int flag)
	{
		if (AFF_BITS_MAX <= flag || 0 >= flag)
			return;

		REMOVE_BIT(bits[(flag-1)>>5], (((DWORD)1)<<((flag-1)&31)));
	}

	inline TAffectFlag& operator = (const TAffectFlag& rhs)
	{
		bits[0] = rhs.bits[0];
		bits[1] = rhs.bits[1];
		return *this;
	}
};

inline bool operator == (const TAffectFlag& lhs, const TAffectFlag& rhs)
{
	return lhs.bits[0] == rhs.bits[0] && lhs.bits[1] == rhs.bits[1];
}

inline bool operator != (const TAffectFlag& lhs, const TAffectFlag& rhs)
{
	return !(lhs == rhs);
}

#endif
