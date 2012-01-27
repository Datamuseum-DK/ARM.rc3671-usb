/*
 * Standard (almost) FIFO implementation
 *
 * This is C(PP) implementation simple FIFOs
 *
 * First you need to define the "class" of fifos with the FIFO_TYPE
 * macro, it takes four arguments:
 *
 *	#define FIFO_TYPE(name, type, rettype, errval, idxtype)
 *
 * For instance:
 *
 *	FIFO_TYPE(foo, uint16_t, int, -1, int8_t)
 *
 *	 name (foo)
 *		Typename of the fifo, used as prefix for all the functions.
 *	type (uint16_t)
 *		What you store in this kind of FIFOs.
 *	rettype (int)
 *		The return type of the get/put functions.
 *	errval (-1)
 *		The error return value for the get/put functions.
 *	idxtype (int8_t)
 *		The type used to index the FIFO, use 'int' unless badly need
 *		to save space.
 *
 * If you don't plan on getting errors from get/put, you can use the
 * same type for rettype as for type, and some suitable in-domain
 * value as errval.
 *
 * The call above will define the functions:
 *	foo_put(), foo_get(), foo_full(), foo_empty() and foo_len().
 * which do exactly what you would expect them to.
 *
 * Next you instantiate your actual fifos:
 *
 *	#define FIFO(name, type, dim)
 *
 * For instance:
 *
 *	FIFO(rxbuf, foo, 40);
 *	FIFO(txbuf, foo, 120);
 *
 *	name (rxbuf/txbuf)
 *		The C variable name of your fifos.
		The names __rxbuf/__txbuf used for the storage array.
 *	type (foo)
 *		Matches argument 1 of FIFO_TYPE() above.
 *	dim (40/120)
 *		Number of elements in the FIFO.
 *
 * Have fun.
 *
 * This file was written by Poul-Henning Kamp <phk@FreeBSD.org> and is in
 * the public domain.
 *
 */

#define FIFO_TYPE(name, type, rettype, errval, idxtype)			\
									\
	struct name##_s {						\
		type	* const ptr;					\
		const idxtype	sze;					\
		volatile idxtype len;					\
		volatile idxtype rd_idx;				\
		volatile idxtype wr_idx;				\
	};								\
									\
	static inline rettype						\
	name##_put(struct name##_s *f, type b)				\
	{								\
									\
		if (f->len == f->sze)					\
			return (errval);				\
		f->ptr[(int)(f->wr_idx)] = b;				\
		f->wr_idx++;						\
		f->wr_idx %= f->sze;					\
		f->len++;						\
		return (0);						\
	}								\
									\
	static inline rettype						\
	name##_get(struct name##_s *f)					\
	{								\
		type r;							\
									\
		if (f->len == 0)					\
			return (errval);				\
		r = f->ptr[(int)(f->rd_idx)];				\
		f->len--;						\
		f->rd_idx++;						\
		f->rd_idx %= f->sze;					\
		return (r);						\
	}								\
									\
	static inline int						\
	name##_full(const struct name##_s *f)				\
	{								\
		return (f->len == f->sze);				\
	}								\
									\
	static inline int						\
	name##_empty(const struct name##_s *f)				\
	{								\
		return (f->len == 0);					\
	}								\
									\
	static inline idxtype						\
	name##_len(const struct name##_s *f)				\
	{								\
		return (f->len);					\
	}

#define FIFO(name, type, dim)						\
	__typeof__(*((struct type##_s*)0)->ptr) __##name[dim];		\
	struct type##_s name = { __##name, dim, 0, 0, 0}
