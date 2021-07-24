/*
 * libwebsockets - small server side websockets and web server implementation
 *
 * Copyright (C) 2010 - 2021 Andy Green <andy@warmcat.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/** \defgroup lecp CBOR parser
 * ##CBOR parsing related functions
 * \ingroup lwsapi
 *
 * LECP is an extremely lightweight CBOR stream parser included in lws.  It
 * is aligned in approach with the LEJP JSON stream parser, with some additional
 * things needed for CBOR.
 */
//@{

#ifndef LECP_MAX_PARSING_STACK_DEPTH
#define LECP_MAX_PARSING_STACK_DEPTH	5
#endif
#ifndef LECP_MAX_DEPTH
#define LECP_MAX_DEPTH			12
#endif
#ifndef LECP_MAX_INDEX_DEPTH
#define LECP_MAX_INDEX_DEPTH		8
#endif
#ifndef LECP_MAX_PATH
#define LECP_MAX_PATH			128
#endif
#ifndef LECP_STRING_CHUNK
/* must be >= 30 to assemble floats */
#define LECP_STRING_CHUNK		254
#endif

#define LECP_FLAG_CB_IS_VALUE 64

/*
 * CBOR initial byte 3 x MSB bits are these
 */

enum {
	LWS_CBOR_MAJTYP_UINT		= 0 << 5,
	LWS_CBOR_MAJTYP_INT_NEG		= 1 << 5,
	LWS_CBOR_MAJTYP_BSTR		= 2 << 5,
	LWS_CBOR_MAJTYP_TSTR		= 3 << 5,
	LWS_CBOR_MAJTYP_ARRAY		= 4 << 5,
	LWS_CBOR_MAJTYP_MAP		= 5 << 5,
	LWS_CBOR_MAJTYP_TAG		= 6 << 5,
	LWS_CBOR_MAJTYP_FLOAT		= 7 << 5,  /* also BREAK */

	LWS_CBOR_MAJTYP_MASK		= 7 << 5,

	/*
	 * For the low 5 bits of the opcode, 0-23 are literals, unless it's
	 * FLOAT.
	 *
	 * 24 = 1 byte; 25 = 2..., 26 = 4... and 27 = 8 bytes following literal.
	 */
	LWS_CBOR_1			= 24,
	LWS_CBOR_2			= 25,
	LWS_CBOR_4			= 26,
	LWS_CBOR_8			= 27,

	LWS_CBOR_RESERVED		= 28,

	LWS_CBOR_SUBMASK		= 0x1f,

	/*
	 * Major type 7 discriminators in low 5 bits
	 * 0 - 23 is SIMPLE implicit value (like, eg, LWS_CBOR_SWK_TRUE)
	 */
	LWS_CBOR_SWK_FALSE		= 20,
	LWS_CBOR_SWK_TRUE		= 21,
	LWS_CBOR_SWK_NULL		= 22,
	LWS_CBOR_SWK_UNDEFINED		= 23,

	LWS_CBOR_M7_SUBTYP_SIMPLE_X8	= 24, /* simple with additional byte */
	LWS_CBOR_M7_SUBTYP_FLOAT16	= 25,
	LWS_CBOR_M7_SUBTYP_FLOAT32	= 26,
	LWS_CBOR_M7_SUBTYP_FLOAT64	= 27,
	LWS_CBOR_M7_BREAK		= 31,

/* 28, 29, 30 are illegal.
 *
 * 31 is illegal for UINT, INT_NEG, and TAG;
 *               for BSTR, TSTR, ARRAY and MAP it means "indefinite length", ie,
 *               it's made up of an endless amount of determinite-length
 *               fragments terminated with a BREAK (FLOAT | 31) instead of the
 *               next determinite-length fragment.  The second framing level
 *               means no need for escapes for BREAK in the data.
 */

	LWS_CBOR_INDETERMINITE		= 31,

/*
 * Well-known tags
 */

	LWS_CBOR_WKTAG_DATETIME_STD	= 0, /* text */
	LWS_CBOR_WKTAG_DATETIME_EPOCH	= 1, /* int or float */
	LWS_CBOR_WKTAG_BIGNUM_UNSIGNED	= 2, /* byte string */
	LWS_CBOR_WKTAG_BIGNUM_NEGATIVE	= 3, /* byte string */
	LWS_CBOR_WKTAG_DECIMAL_FRAC	= 4, /* array */
	LWS_CBOR_WKTAG_BIGFLOAT		= 5, /* array */

	LWS_CBOR_WKTAG_TO_B64U		= 21, /* any */
	LWS_CBOR_WKTAG_TO_B64		= 22, /* any */
	LWS_CBOR_WKTAG_TO_B16		= 23, /* any */
	LWS_CBOR_WKTAG_CBOR		= 24, /* byte string */

	LWS_CBOR_WKTAG_URI		= 32, /* text string */
	LWS_CBOR_WKTAG_B64U		= 33, /* text string */
	LWS_CBOR_WKTAG_B64		= 34, /* text string */
	LWS_CBOR_WKTAG_MIME		= 36, /* text string */

	LWS_CBOR_WKTAG_SELFDESCCBOR	= 55799
};

enum lecp_callbacks {
	LECPCB_CONSTRUCTED	= 0,
	LECPCB_DESTRUCTED	= 1,

	LECPCB_COMPLETE		= 3,
	LECPCB_FAILED		= 4,

	LECPCB_PAIR_NAME	= 5,

	LECPCB_VAL_TRUE		= LECP_FLAG_CB_IS_VALUE | 6,
	LECPCB_VAL_FALSE	= LECP_FLAG_CB_IS_VALUE | 7,
	LECPCB_VAL_NULL		= LECP_FLAG_CB_IS_VALUE | 8,
	LECPCB_VAL_NUM_INT	= LECP_FLAG_CB_IS_VALUE | 9,
	LECPCB_VAL_RESERVED	= LECP_FLAG_CB_IS_VALUE | 10,
	LECPCB_VAL_STR_START	= 11, /* notice handle separately */
	LECPCB_VAL_STR_CHUNK	= LECP_FLAG_CB_IS_VALUE | 12,
	LECPCB_VAL_STR_END	= LECP_FLAG_CB_IS_VALUE | 13,

	LECPCB_ARRAY_START	= 14,
	LECPCB_ARRAY_END	= 15,

	LECPCB_OBJECT_START	= 16,
	LECPCB_OBJECT_END	= 17,

	LECPCB_TAG_START	= 18,
	LECPCB_TAG_END		= 19,

	LECPCB_VAL_NUM_UINT	= LECP_FLAG_CB_IS_VALUE | 20,
	LECPCB_VAL_UNDEFINED	= LECP_FLAG_CB_IS_VALUE | 21,
	LECPCB_VAL_FLOAT16	= LECP_FLAG_CB_IS_VALUE | 22,
	LECPCB_VAL_FLOAT32	= LECP_FLAG_CB_IS_VALUE | 23,
	LECPCB_VAL_FLOAT64	= LECP_FLAG_CB_IS_VALUE | 24,

	LECPCB_VAL_SIMPLE	= LECP_FLAG_CB_IS_VALUE | 25,

	LECPCB_VAL_BLOB_START	= 26, /* notice handle separately */
	LECPCB_VAL_BLOB_CHUNK	= LECP_FLAG_CB_IS_VALUE | 27,
	LECPCB_VAL_BLOB_END	= LECP_FLAG_CB_IS_VALUE | 28,
};

enum lecp_reasons {
	LECP_CONTINUE				= -1,
	LECP_REJECT_BAD_CODING			= -2,
	LECP_REJECT_UNKNOWN			= -3,
	LECP_REJECT_CALLBACK			= -4,
	LECP_STACK_OVERFLOW			= -5,
};


struct lecp_item {
	union {
		uint64_t	u64;
		int64_t		i64;

		uint64_t	u32;

		uint16_t	hf;
#if defined(LWS_WITH_LECP_FLOAT)
		float		f;
		double		d;
#else
		uint32_t	f;
		uint64_t	d;
#endif
	} u;
	uint8_t			opcode;
};

struct lecp_ctx;
typedef signed char (*lecp_callback)(struct lecp_ctx *ctx, char reason);

struct _lecp_stack {
	char			s; /* lejp_state stack*/
	uint8_t			p; /* path length */
	char			i; /* index array length */
	char			indet; /* indeterminite */
	char			intermediate; /* in middle of string */

	char			pop_iss;
	uint64_t		tag;
	uint64_t		collect_rem;
	uint32_t		ordinal;
	uint8_t			opcode;
};

struct _lecp_parsing_stack {
	void			*user;	/* private to the stack level */
	lecp_callback		cb;
	const char * const	*paths;
	uint8_t			count_paths;
	uint8_t			ppos;
	uint8_t			path_match;
};

struct lecp_ctx {

	/* sorted by type for most compact alignment
	 *
	 * pointers
	 */
	void *user;
	uint8_t			*collect_tgt;

	/* arrays */

	struct _lecp_parsing_stack pst[LECP_MAX_PARSING_STACK_DEPTH];
	struct _lecp_stack	st[LECP_MAX_DEPTH];
	uint16_t		i[LECP_MAX_INDEX_DEPTH]; /* index array */
	uint16_t		wild[LECP_MAX_INDEX_DEPTH]; /* index array */
	char			path[LECP_MAX_PATH];

	struct lecp_item	item;


	/* size_t */

	size_t path_stride; /* 0 means default ptr size, else stride...
			     * allows paths to be provided composed inside a
			     * larger user struct instead of a duplicated
			     * array */

	/* short */

	uint16_t uni;

	/* char */

	uint8_t			npos;
	uint8_t			dcount;
	uint8_t			f;
	uint8_t			sp; /* stack head */
	uint8_t			ipos; /* index stack depth */
	uint8_t			count_paths;
	uint8_t			path_match;
	uint8_t			path_match_len;
	uint8_t			wildcount;
	uint8_t			pst_sp; /* parsing stack head */
	uint8_t			outer_array;
	char			present; /* temp for cb reason to use */

	uint8_t			be; /* big endian */

	/* at end so we can memset the rest of it */

	char buf[LECP_STRING_CHUNK + 1];
};

LWS_EXTERN signed char
_lecp_callback(struct lecp_ctx *ctx, char reason);


LWS_VISIBLE LWS_EXTERN void
lecp_construct(struct lecp_ctx *ctx, lecp_callback cb, void *user,
	       const char * const *paths, unsigned char paths_count);

LWS_VISIBLE LWS_EXTERN void
lecp_destruct(struct lecp_ctx *ctx);

LWS_VISIBLE LWS_EXTERN int
lecp_parse(struct lecp_ctx *ctx, const uint8_t *cbor, size_t len);

LWS_VISIBLE LWS_EXTERN void
lecp_change_callback(struct lecp_ctx *ctx, lecp_callback cb);

LWS_VISIBLE LWS_EXTERN const char *
lecp_error_to_string(int e);

//@}
