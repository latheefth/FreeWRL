/*    sv.h
 *
 *    Copyright (C) 1991, 1992, 1993, 1994, 1995, 1996, 1997, 1998, 1999,
 *    2000, 2001, 2002, 2003, 2004, by Larry Wall and others
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifdef sv_flags
#undef sv_flags		/* Convex has this in <signal.h> for sigvec() */
#endif

typedef enum {
	SVt_NULL,	/* 0 */
	SVt_IV,		/* 1 */
	SVt_NV,		/* 2 */
	SVt_RV,		/* 3 */
	SVt_PV,		/* 4 */
	SVt_PVIV,	/* 5 */
	SVt_PVNV,	/* 6 */
	SVt_PVMG,	/* 7 */
	SVt_PVBM,	/* 8 */
	SVt_PVLV,	/* 9 */
	SVt_PVAV,	/* 10 */
	SVt_PVHV,	/* 11 */
	SVt_PVCV,	/* 12 */
	SVt_PVGV,	/* 13 */
	SVt_PVFM,	/* 14 */
	SVt_PVIO	/* 15 */
} svtype;

/* Using C's structural equivalence to help emulate C++ inheritance here... */

struct STRUCT_SV {		/* struct sv { */
    void*	sv_any;		/* pointer to something */
    unsigned int		sv_refcnt;	/* how many references to us */
    unsigned int		sv_flags;	/* what we are */
};

struct gv {
    void*	sv_any;		/* pointer to something */
    unsigned int		sv_refcnt;	/* how many references to us */
    unsigned int		sv_flags;	/* what we are */
};

struct cv {
    void*	sv_any;		/* pointer to something */
    unsigned int		sv_refcnt;	/* how many references to us */
    unsigned int		sv_flags;	/* what we are */
};

struct av {
    void*	sv_any;		/* pointer to something */
    unsigned int		sv_refcnt;	/* how many references to us */
    unsigned int		sv_flags;	/* what we are */
};

struct hv {
    void*	sv_any;		/* pointer to something */
    unsigned int		sv_refcnt;	/* how many references to us */
    unsigned int		sv_flags;	/* what we are */
};

struct io {
    void*	sv_any;		/* pointer to something */
    unsigned int		sv_refcnt;	/* how many references to us */
    unsigned int		sv_flags;	/* what we are */
};

#define SvANY(sv)	(sv)->sv_any
#define SvFLAGS(sv)	(sv)->sv_flags
#define SvREFCNT(sv)	(sv)->sv_refcnt

typedef struct STRUCT_SV SV;
typedef unsigned int STRLEN;

#define SVf_POK            0x00040000      /* has valid public pointer value */

struct xpv {
    char *      xpv_pv;         /* pointer to malloced string */
    STRLEN      xpv_cur;        /* length of xpv_pv as a C string */
    STRLEN      xpv_len;        /* allocated size */
};

