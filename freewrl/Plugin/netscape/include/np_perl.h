/*
 * np_perl.h
 *
 * Frank Holtry  <fholtry@lucent.com>
 * 
 *
 * Copyright (C) 1998 Frank Holtry.  All Rights Reserved.
 * (see the accompanying COPYRIGHT files for details of the copyright
 *  terms and conditions).
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as Perl itself.
 *
 * perl-specific macros, etc.
 */

#define FROM_APPLICATION	1
#define FROM_CGI		2

#define CGISIZE		1 /* Number of bytes returned by the authorization CGI */

#ifndef CGI_CTYPE
#define CGI_CTYPE     "Content-type: application/x-secure-cgi\nContent-length:"
#endif

/* Constants for various array sizes */
#define PPL_INBUF_SZ		4096 /* size of area to malloc for stream buffer */
#define PPL_OPLIST_SZ		2048 /* size of the area to malloc for the oplist */
#define PPL_MAX_STRING_LENGTH	 256 /* Generic max string length */
#define PPL_WINPARM_SZ		 256 /* max size of window parameters string */
#define PPL_MAX_CMD		PPL_MAX_STRING_LENGTH+256+PPL_WINPARM_SZ+PPL_OPLIST_SZ+7+1

/*
 * The following macros include subsets of the opcodes defined by the Opcodes
 * extension module for optags of similar names.  The complement of these
 * lists is used in npshell.c to construct an opcode mask.  Perl opcodes not
 * listed in one of these macros will not be compiled by Perl.  This provides
 * a degree of security against malicious programs being executed by the plugin.
 * The L? indicates the degree of security attached to each set of 
 * opcodes, with L1 intended to be the most restrictive and higher numbers
 * providing progressively less.
 */

 /*
  * This set of macros implements the "default" opset as defined in the Opcode
  * pacakge documentation.  It isn't broad enough to permit Tk to run, but could
  * be useful for many "Perl only" applications.  It's the most secure of the lot.
  */
#define NP_BASE_CORE_L1		":base_core"
#define NP_BASE_MEM_L1		":base_mem"
#define NP_BASE_LOOP_L1		":base_loop"
#define NP_BASE_IO_L1		":base_io"
#define NP_BASE_ORIG_L1		":base_orig"
#define NP_BASE_MATH_L1	
#define NP_BASE_THREAD_L1	":base_thread"
#define NP_FILESYS_READ_L1
#define NP_SYS_DB_L1	
#define NP_FILESYS_OPEN_L1
#define NP_FILESYS_WRITE_L1
#define NP_SUBPROCESS_L1
#define NP_OWNPROCESS_L1
#define NP_OTHERS_L1	
#define NP_STILL_TO_BE_DECIDED_L1
#define NP_DANGEROUS_L1	

 /*
 * The opcodes defined in L2 are intended to be the minimum subset required to
 * allow Perl/Tk programs to display widgets and to allow Perl/OpenGL programs
 * to run. 
 */

#define NP_BASE_CORE_L2	"stub scalar pushmark wantarray const defined undef\
 rv2sv sassign rv2av aassign aelem rv2hv helem each\
 values keys exists delete preinc postinc int abs\
 multiply divide modulo add subtract left_shift\
 right_shift bit_and bit_xor bit_or negate not lt gt\
 le ge eq ne seq sne substr stringify length index\
 ucfirst lcfirst lc quotemeta chop split lslice splice\
 push pop shift unshift warn die unstack enter leave\
 rv2cv anoncode entersub leavesub return method leaveeval null"

#define NP_BASE_MEM_L2	"concat join anonlist anonhash"
#define NP_BASE_LOOP_L2	"grepstart mapstart enteriter iter leaveloop last next goto"
#define NP_BASE_IO_L2	"print"
#define NP_BASE_ORIG_L2	"gv gelem padany rv2gv refgen ref bless regcreset sprintf"
#define NP_BASE_MATH_L2	"sin cos"
#define NP_BASE_THREAD_L2
#define NP_FILESYS_READ_L2	"ftdir ftfile"
#define NP_SYS_DB_L2
#define NP_FILESYS_OPEN_L2
#define NP_FILESYS_WRITE_L2
#define NP_SUBPROCESS_L2
#define NP_OWNPROCESS_L2
#define NP_OTHERS_L2
#define NP_STILL_TO_BE_DECIDED_L2	"sleep sort tied entereval require caller"
#define NP_DANGEROUS_L2


 /*
  * L3 adds the remaining array, hash, string, math, bit manipulation, and
  * filehandle loop functions to L2
  */

#define NP_BASE_CORE_L3		NP_BASE_CORE_L2 " i_preinc predec i_predec \
  i_postinc postdec i_postdec hex oct pow i_multiply i_divide i_modulo \
  i_add i_subtract i_subtract i_negate complement i_ne ncmp i_ncmp \
  cond_expr flip flop andassign orassign and or xor aelemfast aslice av2arylen \
  hslice vec study pos rindex ord chr uc trans schop chomp schomp match \
  qr list reverse slt sgt sle scmp"
  
#define NP_BASE_MEM_L3		NP_BASE_MEM_L2 " repeat range"
#define NP_BASE_LOOP_L3		NP_BASE_LOOP_L2 " grepwhile mapwhile enterloop redo"
#define NP_BASE_IO_L3		NP_BASE_IO_L2 " readline rcatline getc read \
formline enterwrite leavewrite sysread syswrite send recv eof tell seek sysseek \
readdir telldir seekdir rewinddir "
#define NP_BASE_ORIG_L3		NP_BASE_ORIG_L2 " gvsv padsv padav padhv srefgen \
pushre regcmaybe regcomp subst substcont prtf crypt tie untie"
#define NP_BASE_MATH_L3		NP_BASE_MATH_L2 " atan2 exp log sqrt rand srand "
#define NP_BASE_THREAD_L3	NP_BASE_THREAD_L2
#define NP_FILESYS_READ_L3	NP_FILESYS_READ_L2
#define NP_SYS_DB_L3		NP_SYS_DB_L2
#define NP_FILESYS_OPEN_L3	NP_FILESYS_OPEN_L2
#define NP_FILESYS_WRITE_L3	NP_FILESYS_WRITE_L2
#define NP_SUBPROCESS_L3	NP_SUBPROCESS_L2
#define NP_OWNPROCESS_L3	NP_OWNPROCESS_L2
#define NP_OTHERS_L3		NP_OTHERS_L2 
#define NP_STILL_TO_BE_DECIDED_L3	NP_STILL_TO_BE_DECIDED_L2
#define NP_DANGEROUS_L3		NP_DANGEROUS_L2

 /*
  * L4 is intended to be the full set of Perl opcodes  and hence provides no
  * security (via this mechanism) at all.
  */
#define NP_BASE_CORE_L4		":base_core"
#define NP_BASE_MEM_L4		":base_mem"
#define NP_BASE_LOOP_L4		":base_loop"
#define NP_BASE_IO_L4		":base_io"
#define NP_BASE_ORIG_L4		":base_orig"
#define NP_BASE_MATH_L4		":base_math"
#define NP_BASE_THREAD_L4	":base_thread"
#define NP_FILESYS_READ_L4	":filesys_read"
#define NP_SYS_DB_L4		":sys_db"
#define NP_FILESYS_OPEN_L4	":filesys_open"
#define NP_FILESYS_WRITE_L4	":filesys_write"
#define NP_SUBPROCESS_L4	":subprocess"
#define NP_OWNPROCESS_L4	":ownprocess"
#define NP_OTHERS_L4		":others"

#ifdef PERL5005
#define NP_STILL_TO_BE_DECIDED_L4	":still_to_be_decided lock threadsv"
#else
#define NP_STILL_TO_BE_DECIDED_L4	":still_to_be_decided"
#endif

#define NP_DANGEROUS_L4		":dangerous"

/*
 * This level is for user defined security 
 */
#define NP_BASE_CORE_USER		NP_BASE_CORE_L2 " postdec i_postdec \
av2arylen reverse flip flop"
#define NP_BASE_MEM_USER		NP_BASE_MEM_L2 " range"
#define NP_BASE_LOOP_USER		NP_BASE_LOOP_L2
#define NP_BASE_IO_USER			NP_BASE_IO_L2 
#define NP_BASE_ORIG_USER		NP_BASE_ORIG_L2 " localtime gmtime"
#define NP_BASE_MATH_USER		NP_BASE_MATH_L2
#define NP_BASE_THREAD_USER		NP_BASE_THREAD_L2
#define NP_FILESYS_READ_USER		NP_FILESYS_READ_L2
#define NP_SYS_DB_USER			NP_SYS_DB_L2
#define NP_FILESYS_OPEN_USER		NP_FILESYS_OPEN_L2 " open close"
#define NP_FILESYS_WRITE_USER		NP_FILESYS_WRITE_L2
#define NP_SUBPROCESS_USER		NP_SUBPROCESS_L2
#define NP_OWNPROCESS_USER		NP_OWNPROCESS_L2 " time"
#define NP_OTHERS_USER			NP_OTHERS_L2 
#define NP_STILL_TO_BE_DECIDED_USER	NP_STILL_TO_BE_DECIDED_L2 " pack unpack"
#define NP_DANGEROUS_USER		NP_DANGEROUS_L2


 /*
  * This macro construct works correctly with both gnu and Cygnus gcc.  It may
  * not work with other cpp's and is potentially a nasty portability issue.  F.H.
  */
#define OPSET(x)  ":Opl_" #x

#define	OPMASK(maskname,oplist)	"Opcode::define_optag(\"" maskname "\",Opcode::opset(qw(" oplist ")));"

 /*
  * Define sets of opsets.  These resolve to Perl code in npperlplus.c to define
  * groups of opsets
  */
 
#define SECURE_OPSETS_L1 OPMASK(OPSET(a),NP_BASE_CORE_L1) \
		OPMASK(OPSET(b),NP_BASE_MEM_L1) \
		OPMASK(OPSET(c),NP_BASE_LOOP_L1) \
		OPMASK(OPSET(d),NP_BASE_IO_L1) \
		OPMASK(OPSET(e),NP_BASE_ORIG_L1) \
		OPMASK(OPSET(f),NP_BASE_MATH_L1) \
		OPMASK(OPSET(g),NP_BASE_THREAD_L1) \
		OPMASK(OPSET(h),NP_FILESYS_READ_L1) \
		OPMASK(OPSET(i),NP_SYS_DB_L1) \
		OPMASK(OPSET(j),NP_FILESYS_OPEN_L1) \
		OPMASK(OPSET(k),NP_FILESYS_WRITE_L1) \
		OPMASK(OPSET(l),NP_SUBPROCESS_L1) \
		OPMASK(OPSET(m),NP_OWNPROCESS_L1) \
		OPMASK(OPSET(n),NP_OTHERS_L1) \
		OPMASK(OPSET(o),NP_STILL_TO_BE_DECIDED_L1) \
		OPMASK(OPSET(p),NP_DANGEROUS_L1)

#define SECURE_OPSETS_L2 OPMASK(OPSET(a),NP_BASE_CORE_L2) \
		OPMASK(OPSET(b),NP_BASE_MEM_L2) \
		OPMASK(OPSET(c),NP_BASE_LOOP_L2) \
		OPMASK(OPSET(d),NP_BASE_IO_L2) \
		OPMASK(OPSET(e),NP_BASE_ORIG_L2) \
		OPMASK(OPSET(f),NP_BASE_MATH_L2) \
		OPMASK(OPSET(g),NP_BASE_THREAD_L2) \
		OPMASK(OPSET(h),NP_FILESYS_READ_L2) \
		OPMASK(OPSET(i),NP_SYS_DB_L2) \
		OPMASK(OPSET(j),NP_FILESYS_OPEN_L2) \
		OPMASK(OPSET(k),NP_FILESYS_WRITE_L2) \
		OPMASK(OPSET(l),NP_SUBPROCESS_L2) \
		OPMASK(OPSET(m),NP_OWNPROCESS_L2) \
		OPMASK(OPSET(n),NP_OTHERS_L2) \
		OPMASK(OPSET(o),NP_STILL_TO_BE_DECIDED_L2) \
		OPMASK(OPSET(p),NP_DANGEROUS_L2)

#define SECURE_OPSETS_L3 OPMASK(OPSET(a),NP_BASE_CORE_L3) \
		OPMASK(OPSET(b),NP_BASE_MEM_L3) \
		OPMASK(OPSET(c),NP_BASE_LOOP_L3) \
		OPMASK(OPSET(d),NP_BASE_IO_L3) \
		OPMASK(OPSET(e),NP_BASE_ORIG_L3) \
		OPMASK(OPSET(f),NP_BASE_MATH_L3) \
		OPMASK(OPSET(g),NP_BASE_THREAD_L3) \
		OPMASK(OPSET(h),NP_FILESYS_READ_L3) \
		OPMASK(OPSET(i),NP_SYS_DB_L3) \
		OPMASK(OPSET(j),NP_FILESYS_OPEN_L3) \
		OPMASK(OPSET(k),NP_FILESYS_WRITE_L3) \
		OPMASK(OPSET(l),NP_SUBPROCESS_L3) \
		OPMASK(OPSET(m),NP_OWNPROCESS_L3) \
		OPMASK(OPSET(n),NP_OTHERS_L3) \
		OPMASK(OPSET(o),NP_STILL_TO_BE_DECIDED_L3) \
		OPMASK(OPSET(p),NP_DANGEROUS_L3)

#define SECURE_OPSETS_L4 OPMASK(OPSET(a),NP_BASE_CORE_L4) \
		OPMASK(OPSET(b),NP_BASE_MEM_L4) \
		OPMASK(OPSET(c),NP_BASE_LOOP_L4) \
		OPMASK(OPSET(d),NP_BASE_IO_L4) \
		OPMASK(OPSET(e),NP_BASE_ORIG_L4) \
		OPMASK(OPSET(f),NP_BASE_MATH_L4) \
		OPMASK(OPSET(g),NP_BASE_THREAD_L4) \
		OPMASK(OPSET(h),NP_FILESYS_READ_L4) \
		OPMASK(OPSET(i),NP_SYS_DB_L4) \
		OPMASK(OPSET(j),NP_FILESYS_OPEN_L4) \
		OPMASK(OPSET(k),NP_FILESYS_WRITE_L4) \
		OPMASK(OPSET(l),NP_SUBPROCESS_L4) \
		OPMASK(OPSET(m),NP_OWNPROCESS_L4) \
		OPMASK(OPSET(n),NP_OTHERS_L4) \
		OPMASK(OPSET(o),NP_STILL_TO_BE_DECIDED_L4) \
		OPMASK(OPSET(p),NP_DANGEROUS_L4)

#define SECURE_OPSETS_USER OPMASK(OPSET(a),NP_BASE_CORE_USER) \
		OPMASK(OPSET(b),NP_BASE_MEM_USER) \
		OPMASK(OPSET(c),NP_BASE_LOOP_USER) \
		OPMASK(OPSET(d),NP_BASE_IO_USER) \
		OPMASK(OPSET(e),NP_BASE_ORIG_USER) \
		OPMASK(OPSET(f),NP_BASE_MATH_USER) \
		OPMASK(OPSET(g),NP_BASE_THREAD_USER) \
		OPMASK(OPSET(h),NP_FILESYS_READ_USER) \
		OPMASK(OPSET(i),NP_SYS_DB_USER) \
		OPMASK(OPSET(j),NP_FILESYS_OPEN_USER) \
		OPMASK(OPSET(k),NP_FILESYS_WRITE_USER) \
		OPMASK(OPSET(l),NP_SUBPROCESS_USER) \
		OPMASK(OPSET(m),NP_OWNPROCESS_USER) \
		OPMASK(OPSET(n),NP_OTHERS_USER) \
		OPMASK(OPSET(o),NP_STILL_TO_BE_DECIDED_USER) \
		OPMASK(OPSET(p),NP_DANGEROUS_USER)

 /*
  * Create lists of opsets for varying degrees of security these simplify the
  * Opcode::opmask_add command
  */

#define OPSLIST	OPSET(a) " "\
		OPSET(b) " "\
		OPSET(c) " "\
		OPSET(d) " "\
		OPSET(e) " "\
		OPSET(f) " "\
		OPSET(g) " "\
		OPSET(h) " "\
		OPSET(i) " "\
		OPSET(j) " "\
		OPSET(k) " "\
		OPSET(l) " "\
		OPSET(m) " "\
		OPSET(n) " "\
		OPSET(o) " "\
		OPSET(p)

#define OPM_ADD(oplist)	"Opcode::opmask_add(Opcode::invert_opset(Opcode::opset(qw(" oplist "))));"

