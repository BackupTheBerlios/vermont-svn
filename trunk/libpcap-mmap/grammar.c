/* A Bison parser, made by GNU Bison 1.875a.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0

/* If NAME_PREFIX is specified substitute the variables and functions
   names.  */
#define yyparse pcap_parse
#define yylex   pcap_lex
#define yyerror pcap_error
#define yylval  pcap_lval
#define yychar  pcap_char
#define yydebug pcap_debug
#define yynerrs pcap_nerrs


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     DST = 258,
     SRC = 259,
     HOST = 260,
     GATEWAY = 261,
     NET = 262,
     NETMASK = 263,
     PORT = 264,
     LESS = 265,
     GREATER = 266,
     PROTO = 267,
     PROTOCHAIN = 268,
     CBYTE = 269,
     ARP = 270,
     RARP = 271,
     IP = 272,
     SCTP = 273,
     TCP = 274,
     UDP = 275,
     ICMP = 276,
     IGMP = 277,
     IGRP = 278,
     PIM = 279,
     VRRP = 280,
     ATALK = 281,
     AARP = 282,
     DECNET = 283,
     LAT = 284,
     SCA = 285,
     MOPRC = 286,
     MOPDL = 287,
     TK_BROADCAST = 288,
     TK_MULTICAST = 289,
     NUM = 290,
     INBOUND = 291,
     OUTBOUND = 292,
     PF_IFNAME = 293,
     PF_RSET = 294,
     PF_RNR = 295,
     PF_SRNR = 296,
     PF_REASON = 297,
     PF_ACTION = 298,
     LINK = 299,
     GEQ = 300,
     LEQ = 301,
     NEQ = 302,
     ID = 303,
     EID = 304,
     HID = 305,
     HID6 = 306,
     AID = 307,
     LSH = 308,
     RSH = 309,
     LEN = 310,
     IPV6 = 311,
     ICMPV6 = 312,
     AH = 313,
     ESP = 314,
     VLAN = 315,
     MPLS = 316,
     ISO = 317,
     ESIS = 318,
     CLNP = 319,
     ISIS = 320,
     L1 = 321,
     L2 = 322,
     IIH = 323,
     LSP = 324,
     SNP = 325,
     CSNP = 326,
     PSNP = 327,
     STP = 328,
     IPX = 329,
     NETBEUI = 330,
     LANE = 331,
     LLC = 332,
     METAC = 333,
     BCC = 334,
     SC = 335,
     ILMIC = 336,
     OAMF4EC = 337,
     OAMF4SC = 338,
     OAM = 339,
     OAMF4 = 340,
     CONNECTMSG = 341,
     METACONNECT = 342,
     VPI = 343,
     VCI = 344,
     AND = 345,
     OR = 346,
     UMINUS = 347
   };
#endif
#define DST 258
#define SRC 259
#define HOST 260
#define GATEWAY 261
#define NET 262
#define NETMASK 263
#define PORT 264
#define LESS 265
#define GREATER 266
#define PROTO 267
#define PROTOCHAIN 268
#define CBYTE 269
#define ARP 270
#define RARP 271
#define IP 272
#define SCTP 273
#define TCP 274
#define UDP 275
#define ICMP 276
#define IGMP 277
#define IGRP 278
#define PIM 279
#define VRRP 280
#define ATALK 281
#define AARP 282
#define DECNET 283
#define LAT 284
#define SCA 285
#define MOPRC 286
#define MOPDL 287
#define TK_BROADCAST 288
#define TK_MULTICAST 289
#define NUM 290
#define INBOUND 291
#define OUTBOUND 292
#define PF_IFNAME 293
#define PF_RSET 294
#define PF_RNR 295
#define PF_SRNR 296
#define PF_REASON 297
#define PF_ACTION 298
#define LINK 299
#define GEQ 300
#define LEQ 301
#define NEQ 302
#define ID 303
#define EID 304
#define HID 305
#define HID6 306
#define AID 307
#define LSH 308
#define RSH 309
#define LEN 310
#define IPV6 311
#define ICMPV6 312
#define AH 313
#define ESP 314
#define VLAN 315
#define MPLS 316
#define ISO 317
#define ESIS 318
#define CLNP 319
#define ISIS 320
#define L1 321
#define L2 322
#define IIH 323
#define LSP 324
#define SNP 325
#define CSNP 326
#define PSNP 327
#define STP 328
#define IPX 329
#define NETBEUI 330
#define LANE 331
#define LLC 332
#define METAC 333
#define BCC 334
#define SC 335
#define ILMIC 336
#define OAMF4EC 337
#define OAMF4SC 338
#define OAM 339
#define OAMF4 340
#define CONNECTMSG 341
#define METACONNECT 342
#define VPI 343
#define VCI 344
#define AND 345
#define OR 346
#define UMINUS 347




/* Copy the first part of user declarations.  */
#line 1 "grammar.y"

/*
 * Copyright (c) 1988, 1989, 1990, 1991, 1992, 1993, 1994, 1995, 1996
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#ifndef lint
static const char rcsid[] _U_ =
    "@(#) $Header: /n/CVS/sirt/libpcap/grammar.y,v 0.8.3.1 2004/10/01 22:21:34 cpw Exp $ (LBL)";
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef WIN32
#include <pcap-stdinc.h>
#else /* WIN32 */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#endif /* WIN32 */

#include <stdlib.h>

#ifndef WIN32
#if __STDC__
struct mbuf;
struct rtentry;
#endif

#include <netinet/in.h>
#endif /* WIN32 */

#include <stdio.h>
#include <strings.h>

#include "pcap-int.h"

#include "gencode.h"
#include "pf.h"
#include <pcap-namedb.h>

#ifdef HAVE_OS_PROTO_H
#include "os-proto.h"
#endif

#define QSET(q, p, d, a) (q).proto = (p),\
			 (q).dir = (d),\
			 (q).addr = (a)

int n_errors = 0;

static struct qual qerr = { Q_UNDEF, Q_UNDEF, Q_UNDEF, Q_UNDEF };

static void
yyerror(char *msg)
{
	++n_errors;
	bpf_error("%s", msg);
	/* NOTREACHED */
}

#ifndef YYBISON
int yyparse(void);

int
pcap_parse()
{
	return (yyparse());
}
#endif



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 92 "grammar.y"
typedef union YYSTYPE {
	int i;
	bpf_u_int32 h;
	u_char *e;
	char *s;
	struct stmt *stmt;
	struct arth *a;
	struct {
		struct qual q;
		int atmfieldtype;
		struct block *b;
	} blk;
	struct block *rblk;
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 375 "y.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 387 "y.tab.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   569

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  108
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  36
/* YYNRULES -- Number of rules. */
#define YYNRULES  166
/* YYNRULES -- Number of states. */
#define YYNSTATES  228

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   347

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    92,     2,     2,     2,     2,    94,     2,
     101,   100,    97,    95,     2,    96,     2,    98,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   107,     2,
     104,   103,   102,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   105,     2,   106,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    93,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    99
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short yyprhs[] =
{
       0,     0,     3,     6,     8,     9,    11,    15,    19,    23,
      27,    29,    31,    33,    35,    39,    41,    45,    49,    51,
      55,    57,    59,    61,    64,    66,    68,    70,    74,    78,
      80,    82,    84,    87,    91,    94,    97,   100,   103,   106,
     109,   113,   115,   119,   123,   125,   127,   129,   132,   134,
     135,   137,   139,   143,   147,   151,   155,   157,   159,   161,
     163,   165,   167,   169,   171,   173,   175,   177,   179,   181,
     183,   185,   187,   189,   191,   193,   195,   197,   199,   201,
     203,   205,   207,   209,   211,   213,   215,   217,   219,   221,
     223,   225,   227,   229,   231,   233,   235,   237,   240,   243,
     246,   249,   254,   256,   258,   261,   263,   266,   268,   270,
     273,   276,   279,   282,   285,   288,   290,   292,   294,   296,
     298,   300,   302,   304,   306,   308,   310,   315,   322,   326,
     330,   334,   338,   342,   346,   350,   354,   357,   361,   363,
     365,   367,   369,   371,   373,   375,   379,   381,   383,   385,
     387,   389,   391,   393,   395,   397,   399,   401,   403,   405,
     407,   409,   412,   415,   419,   421,   423
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short yyrhs[] =
{
     109,     0,    -1,   110,   111,    -1,   110,    -1,    -1,   120,
      -1,   111,   112,   120,    -1,   111,   112,   114,    -1,   111,
     113,   120,    -1,   111,   113,   114,    -1,    90,    -1,    91,
      -1,   115,    -1,   137,    -1,   117,   118,   100,    -1,    48,
      -1,    50,    98,    35,    -1,    50,     8,    50,    -1,    50,
      -1,    51,    98,    35,    -1,    51,    -1,    49,    -1,    52,
      -1,   116,   114,    -1,    92,    -1,   101,    -1,   115,    -1,
     119,   112,   114,    -1,   119,   113,   114,    -1,   137,    -1,
     118,    -1,   122,    -1,   116,   120,    -1,   123,   124,   125,
      -1,   123,   124,    -1,   123,   125,    -1,   123,    12,    -1,
     123,    13,    -1,   123,   126,    -1,   121,   114,    -1,   117,
     111,   100,    -1,   127,    -1,   134,   132,   134,    -1,   134,
     133,   134,    -1,   128,    -1,   138,    -1,   139,    -1,   140,
     141,    -1,   127,    -1,    -1,     4,    -1,     3,    -1,     4,
      91,     3,    -1,     3,    91,     4,    -1,     4,    90,     3,
      -1,     3,    90,     4,    -1,     5,    -1,     7,    -1,     9,
      -1,     6,    -1,    44,    -1,    17,    -1,    15,    -1,    16,
      -1,    18,    -1,    19,    -1,    20,    -1,    21,    -1,    22,
      -1,    23,    -1,    24,    -1,    25,    -1,    26,    -1,    27,
      -1,    28,    -1,    29,    -1,    30,    -1,    32,    -1,    31,
      -1,    56,    -1,    57,    -1,    58,    -1,    59,    -1,    62,
      -1,    63,    -1,    65,    -1,    66,    -1,    67,    -1,    68,
      -1,    69,    -1,    70,    -1,    72,    -1,    71,    -1,    64,
      -1,    73,    -1,    74,    -1,    75,    -1,   123,    33,    -1,
     123,    34,    -1,    10,    35,    -1,    11,    35,    -1,    14,
      35,   136,    35,    -1,    36,    -1,    37,    -1,    60,   137,
      -1,    60,    -1,    61,   137,    -1,    61,    -1,   129,    -1,
      38,    48,    -1,    39,    48,    -1,    40,    35,    -1,    41,
      35,    -1,    42,   130,    -1,    43,   131,    -1,    35,    -1,
      48,    -1,    48,    -1,   102,    -1,    45,    -1,   103,    -1,
      46,    -1,   104,    -1,    47,    -1,   137,    -1,   135,    -1,
     127,   105,   134,   106,    -1,   127,   105,   134,   107,    35,
     106,    -1,   134,    95,   134,    -1,   134,    96,   134,    -1,
     134,    97,   134,    -1,   134,    98,   134,    -1,   134,    94,
     134,    -1,   134,    93,   134,    -1,   134,    53,   134,    -1,
     134,    54,   134,    -1,    96,   134,    -1,   117,   135,   100,
      -1,    55,    -1,    94,    -1,    93,    -1,   104,    -1,   102,
      -1,   103,    -1,    35,    -1,   117,   137,   100,    -1,    76,
      -1,    77,    -1,    78,    -1,    79,    -1,    82,    -1,    83,
      -1,    80,    -1,    81,    -1,    84,    -1,    85,    -1,    86,
      -1,    87,    -1,    88,    -1,    89,    -1,   142,    -1,   132,
      35,    -1,   133,    35,    -1,   117,   143,   100,    -1,    35,
      -1,   142,    -1,   143,   113,   142,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   155,   155,   159,   161,   163,   164,   165,   166,   167,
     169,   171,   173,   174,   176,   178,   179,   181,   183,   188,
     197,   206,   215,   224,   226,   228,   230,   231,   232,   234,
     236,   238,   239,   241,   242,   243,   244,   245,   246,   248,
     249,   250,   251,   253,   255,   256,   257,   258,   261,   262,
     265,   266,   267,   268,   269,   270,   273,   274,   275,   278,
     280,   281,   282,   283,   284,   285,   286,   287,   288,   289,
     290,   291,   292,   293,   294,   295,   296,   297,   298,   299,
     300,   301,   302,   303,   304,   305,   306,   307,   308,   309,
     310,   311,   312,   313,   314,   315,   316,   318,   319,   320,
     321,   322,   323,   324,   325,   326,   327,   328,   329,   332,
     333,   334,   335,   336,   337,   340,   341,   354,   365,   366,
     367,   369,   370,   371,   373,   374,   376,   377,   378,   379,
     380,   381,   382,   383,   384,   385,   386,   387,   388,   390,
     391,   392,   393,   394,   396,   397,   399,   400,   401,   402,
     403,   404,   405,   406,   408,   409,   410,   411,   414,   415,
     417,   418,   419,   420,   422,   429,   430
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "DST", "SRC", "HOST", "GATEWAY", "NET", 
  "NETMASK", "PORT", "LESS", "GREATER", "PROTO", "PROTOCHAIN", "CBYTE", 
  "ARP", "RARP", "IP", "SCTP", "TCP", "UDP", "ICMP", "IGMP", "IGRP", 
  "PIM", "VRRP", "ATALK", "AARP", "DECNET", "LAT", "SCA", "MOPRC", 
  "MOPDL", "TK_BROADCAST", "TK_MULTICAST", "NUM", "INBOUND", "OUTBOUND", 
  "PF_IFNAME", "PF_RSET", "PF_RNR", "PF_SRNR", "PF_REASON", "PF_ACTION", 
  "LINK", "GEQ", "LEQ", "NEQ", "ID", "EID", "HID", "HID6", "AID", "LSH", 
  "RSH", "LEN", "IPV6", "ICMPV6", "AH", "ESP", "VLAN", "MPLS", "ISO", 
  "ESIS", "CLNP", "ISIS", "L1", "L2", "IIH", "LSP", "SNP", "CSNP", "PSNP", 
  "STP", "IPX", "NETBEUI", "LANE", "LLC", "METAC", "BCC", "SC", "ILMIC", 
  "OAMF4EC", "OAMF4SC", "OAM", "OAMF4", "CONNECTMSG", "METACONNECT", 
  "VPI", "VCI", "AND", "OR", "'!'", "'|'", "'&'", "'+'", "'-'", "'*'", 
  "'/'", "UMINUS", "')'", "'('", "'>'", "'='", "'<'", "'['", "']'", "':'", 
  "$accept", "prog", "null", "expr", "and", "or", "id", "nid", "not", 
  "paren", "pid", "qid", "term", "head", "rterm", "pqual", "dqual", 
  "aqual", "ndaqual", "pname", "other", "pfvar", "reason", "action", 
  "relop", "irelop", "arth", "narth", "byteop", "pnum", "atmtype", 
  "atmmultitype", "atmfield", "atmvalue", "atmfieldvalue", "atmlistvalue", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,    33,   124,    38,    43,    45,    42,    47,   347,
      41,    40,    62,    61,    60,    91,    93,    58
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,   108,   109,   109,   110,   111,   111,   111,   111,   111,
     112,   113,   114,   114,   114,   115,   115,   115,   115,   115,
     115,   115,   115,   115,   116,   117,   118,   118,   118,   119,
     119,   120,   120,   121,   121,   121,   121,   121,   121,   122,
     122,   122,   122,   122,   122,   122,   122,   122,   123,   123,
     124,   124,   124,   124,   124,   124,   125,   125,   125,   126,
     127,   127,   127,   127,   127,   127,   127,   127,   127,   127,
     127,   127,   127,   127,   127,   127,   127,   127,   127,   127,
     127,   127,   127,   127,   127,   127,   127,   127,   127,   127,
     127,   127,   127,   127,   127,   127,   127,   128,   128,   128,
     128,   128,   128,   128,   128,   128,   128,   128,   128,   129,
     129,   129,   129,   129,   129,   130,   130,   131,   132,   132,
     132,   133,   133,   133,   134,   134,   135,   135,   135,   135,
     135,   135,   135,   135,   135,   135,   135,   135,   135,   136,
     136,   136,   136,   136,   137,   137,   138,   138,   138,   138,
     138,   138,   138,   138,   139,   139,   139,   139,   140,   140,
     141,   141,   141,   141,   142,   143,   143
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     2,     1,     0,     1,     3,     3,     3,     3,
       1,     1,     1,     1,     3,     1,     3,     3,     1,     3,
       1,     1,     1,     2,     1,     1,     1,     3,     3,     1,
       1,     1,     2,     3,     2,     2,     2,     2,     2,     2,
       3,     1,     3,     3,     1,     1,     1,     2,     1,     0,
       1,     1,     3,     3,     3,     3,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     2,     2,     2,
       2,     4,     1,     1,     2,     1,     2,     1,     1,     2,
       2,     2,     2,     2,     2,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     4,     6,     3,     3,
       3,     3,     3,     3,     3,     3,     2,     3,     1,     1,
       1,     1,     1,     1,     1,     3,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     2,     2,     3,     1,     1,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       4,     0,    49,     1,     0,     0,     0,    62,    63,    61,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    78,    77,   144,   102,   103,     0,     0,
       0,     0,     0,     0,    60,   138,    79,    80,    81,    82,
     105,   107,    83,    84,    93,    85,    86,    87,    88,    89,
      90,    92,    91,    94,    95,    96,   146,   147,   148,   149,
     152,   153,   150,   151,   154,   155,   156,   157,   158,   159,
      24,     0,    25,     2,    49,    49,     5,     0,    31,     0,
      48,    44,   108,     0,   125,   124,    45,    46,     0,    99,
     100,     0,   109,   110,   111,   112,   115,   116,   113,   117,
     114,     0,   104,   106,     0,     0,   136,    10,    11,    49,
      49,    32,     0,   125,   124,    15,    21,    18,    20,    22,
      39,    12,     0,     0,    13,    51,    50,    56,    59,    57,
      58,    36,    37,    97,    98,    34,    35,    38,     0,   119,
     121,   123,     0,     0,     0,     0,     0,     0,     0,     0,
     118,   120,   122,     0,     0,   164,     0,     0,     0,    47,
     160,   140,   139,   142,   143,   141,     0,     0,     0,     7,
      49,    49,     6,   124,     9,     8,    40,   137,   145,     0,
       0,     0,    23,    26,    30,     0,    29,     0,     0,     0,
       0,    33,     0,   134,   135,   133,   132,   128,   129,   130,
     131,    42,    43,   165,     0,   161,   162,   101,   124,    17,
      16,    19,    14,     0,     0,    55,    53,    54,    52,   126,
       0,   163,     0,    27,    28,     0,   166,   127
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short yydefgoto[] =
{
      -1,     1,     2,   112,   109,   110,   182,   121,   122,   104,
     184,   185,    76,    77,    78,    79,   135,   136,   137,   105,
      81,    82,    98,   100,   153,   154,    83,    84,   166,    85,
      86,    87,    88,   159,   160,   204
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -176
static const short yypact[] =
{
    -176,    16,   199,  -176,   -17,     8,    15,  -176,  -176,  -176,
    -176,  -176,  -176,  -176,  -176,  -176,  -176,  -176,  -176,  -176,
    -176,  -176,  -176,  -176,  -176,  -176,  -176,  -176,   -15,     9,
      39,    50,   -16,    40,  -176,  -176,  -176,  -176,  -176,  -176,
     -21,   -21,  -176,  -176,  -176,  -176,  -176,  -176,  -176,  -176,
    -176,  -176,  -176,  -176,  -176,  -176,  -176,  -176,  -176,  -176,
    -176,  -176,  -176,  -176,  -176,  -176,  -176,  -176,  -176,  -176,
    -176,   462,  -176,   -64,   375,   375,  -176,    10,  -176,   535,
       1,  -176,  -176,   457,  -176,  -176,  -176,  -176,   100,  -176,
    -176,    57,  -176,  -176,  -176,  -176,  -176,  -176,  -176,  -176,
    -176,   -21,  -176,  -176,   462,   -18,  -176,  -176,  -176,   287,
     287,  -176,   -70,    -7,    -3,  -176,  -176,    -4,     2,  -176,
    -176,  -176,    10,    10,  -176,   -50,   -44,  -176,  -176,  -176,
    -176,  -176,  -176,  -176,  -176,   147,  -176,  -176,   462,  -176,
    -176,  -176,   462,   462,   462,   462,   462,   462,   462,   462,
    -176,  -176,  -176,   462,   462,  -176,    68,    69,    75,  -176,
    -176,  -176,  -176,  -176,  -176,  -176,    77,    -3,    95,  -176,
     287,   287,  -176,     5,  -176,  -176,  -176,  -176,  -176,    65,
      84,    96,  -176,  -176,    30,   -64,    -3,   128,   132,   138,
     141,  -176,    89,    67,    67,   -42,   -31,   -28,   -28,  -176,
    -176,    95,    95,  -176,   -63,  -176,  -176,  -176,   -66,  -176,
    -176,  -176,  -176,    10,    10,  -176,  -176,  -176,  -176,  -176,
     118,  -176,    68,  -176,  -176,    49,  -176,  -176
};

/* YYPGOTO[NTERM-NUM].  */
static const short yypgoto[] =
{
    -176,  -176,  -176,   155,   -27,  -175,   -74,  -108,     4,    -2,
    -176,  -176,   -61,  -176,  -176,  -176,  -176,    31,  -176,     7,
    -176,  -176,  -176,  -176,    79,    82,   -20,   -73,  -176,   -33,
    -176,  -176,  -176,  -176,  -139,  -176
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -42
static const short yytable[] =
{
      75,   -41,   113,   120,   179,   -13,    74,   102,   103,    80,
     214,   142,   143,   111,    25,   183,     3,   203,    89,    96,
     107,   108,   142,   143,   -29,   -29,   107,   108,   108,   222,
     176,   113,    97,    92,   178,   169,   174,   221,   101,   101,
     187,   188,   114,    90,   124,    25,   189,   190,   172,   175,
      91,   106,   145,   146,   147,   148,   149,    93,   115,   116,
     117,   118,   119,   183,   146,   147,   148,   149,   167,   148,
     149,   114,    75,    75,    94,   123,   173,   173,    74,    74,
      72,    80,    80,   226,   168,    95,   156,   138,    99,   124,
     186,   -41,   -41,   177,   180,   -13,   -13,   178,   113,   101,
     181,   -41,    70,   155,   205,   -13,   138,   171,   171,   111,
     206,    72,   207,   170,   170,   209,    80,    80,   192,   210,
     123,   101,   193,   194,   195,   196,   197,   198,   199,   200,
     212,   211,   215,   201,   202,   155,   216,   173,   208,   223,
     224,   217,   142,   143,   218,   139,   140,   141,   142,   143,
     161,   162,   127,   225,   129,   227,   130,    73,   213,   163,
     164,   165,   146,   147,   148,   149,   191,   157,   171,    75,
     158,     0,     0,     0,   170,   170,     0,    80,    80,     0,
     124,   124,   144,   145,   146,   147,   148,   149,   144,   145,
     146,   147,   148,   149,     0,   219,   220,     0,     0,    -3,
       0,    72,   150,   151,   152,     0,     0,     0,     0,     4,
       5,   123,   123,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,     0,     0,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,     0,
       0,    70,     0,     0,     0,    71,     0,     4,     5,     0,
      72,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
       0,     0,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,     0,     0,     0,   115,   116,   117,   118,   119,
       0,     0,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,     0,     0,    70,
       0,     0,     0,    71,     0,     4,     5,     0,    72,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,     0,     0,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,     0,     0,    70,     0,     0,
       0,    71,     0,     0,     0,     0,    72,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,     0,     0,    25,     0,     0,
       0,     0,   139,   140,   141,     0,    34,     0,     0,     0,
     142,   143,     0,     0,     0,     0,     0,    35,    36,    37,
      38,    39,     0,     0,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,   125,   126,
     127,   128,   129,     0,   130,     0,     0,   131,   132,     0,
     144,   145,   146,   147,   148,   149,     0,     0,    71,   150,
     151,   152,     0,    72,     0,     0,     0,     0,   133,   134
};

static const short yycheck[] =
{
       2,     0,    75,    77,     8,     0,     2,    40,    41,     2,
     185,    53,    54,    74,    35,   123,     0,   156,    35,    35,
      90,    91,    53,    54,    90,    91,    90,    91,    91,   204,
     100,   104,    48,    48,   100,   109,   110,   100,    40,    41,
      90,    91,    75,    35,    77,    35,    90,    91,   109,   110,
      35,    71,    94,    95,    96,    97,    98,    48,    48,    49,
      50,    51,    52,   171,    95,    96,    97,    98,   101,    97,
      98,   104,    74,    75,    35,    77,   109,   110,    74,    75,
     101,    74,    75,   222,   104,    35,    88,   105,    48,   122,
     123,    90,    91,   100,    98,    90,    91,   100,   171,   101,
      98,   100,    92,    35,    35,   100,   105,   109,   110,   170,
      35,   101,    35,   109,   110,    50,   109,   110,   138,    35,
     122,   123,   142,   143,   144,   145,   146,   147,   148,   149,
     100,    35,     4,   153,   154,    35,     4,   170,   171,   213,
     214,     3,    53,    54,     3,    45,    46,    47,    53,    54,
      93,    94,     5,    35,     7,   106,     9,     2,   185,   102,
     103,   104,    95,    96,    97,    98,   135,    88,   170,   171,
      88,    -1,    -1,    -1,   170,   171,    -1,   170,   171,    -1,
     213,   214,    93,    94,    95,    96,    97,    98,    93,    94,
      95,    96,    97,    98,    -1,   106,   107,    -1,    -1,     0,
      -1,   101,   102,   103,   104,    -1,    -1,    -1,    -1,    10,
      11,   213,   214,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    -1,    -1,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    -1,
      -1,    92,    -1,    -1,    -1,    96,    -1,    10,    11,    -1,
     101,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      -1,    -1,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    -1,    -1,    -1,    48,    49,    50,    51,    52,
      -1,    -1,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    -1,    -1,    92,
      -1,    -1,    -1,    96,    -1,    10,    11,    -1,   101,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    -1,    -1,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    -1,    -1,    92,    -1,    -1,
      -1,    96,    -1,    -1,    -1,    -1,   101,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    -1,    -1,    35,    -1,    -1,
      -1,    -1,    45,    46,    47,    -1,    44,    -1,    -1,    -1,
      53,    54,    -1,    -1,    -1,    -1,    -1,    55,    56,    57,
      58,    59,    -1,    -1,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,     3,     4,
       5,     6,     7,    -1,     9,    -1,    -1,    12,    13,    -1,
      93,    94,    95,    96,    97,    98,    -1,    -1,    96,   102,
     103,   104,    -1,   101,    -1,    -1,    -1,    -1,    33,    34
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,   109,   110,     0,    10,    11,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      92,    96,   101,   111,   116,   117,   120,   121,   122,   123,
     127,   128,   129,   134,   135,   137,   138,   139,   140,    35,
      35,    35,    48,    48,    35,    35,    35,    48,   130,    48,
     131,   117,   137,   137,   117,   127,   134,    90,    91,   112,
     113,   120,   111,   135,   137,    48,    49,    50,    51,    52,
     114,   115,   116,   117,   137,     3,     4,     5,     6,     7,
       9,    12,    13,    33,    34,   124,   125,   126,   105,    45,
      46,    47,    53,    54,    93,    94,    95,    96,    97,    98,
     102,   103,   104,   132,   133,    35,   117,   132,   133,   141,
     142,    93,    94,   102,   103,   104,   136,   137,   134,   114,
     116,   117,   120,   137,   114,   120,   100,   100,   100,     8,
      98,    98,   114,   115,   118,   119,   137,    90,    91,    90,
      91,   125,   134,   134,   134,   134,   134,   134,   134,   134,
     134,   134,   134,   142,   143,    35,    35,    35,   137,    50,
      35,    35,   100,   112,   113,     4,     4,     3,     3,   106,
     107,   100,   113,   114,   114,    35,   142,   106
};

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrlab1


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)         \
  Current.first_line   = Rhs[1].first_line;      \
  Current.first_column = Rhs[1].first_column;    \
  Current.last_line    = Rhs[N].last_line;       \
  Current.last_column  = Rhs[N].last_column;
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (cinluded).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short *bottom, short *top)
#else
static void
yy_stack_print (bottom, top)
    short *bottom;
    short *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylineno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylineno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 156 "grammar.y"
    {
	finish_parse(yyvsp[0].blk.b);
}
    break;

  case 4:
#line 161 "grammar.y"
    { yyval.blk.q = qerr; }
    break;

  case 6:
#line 164 "grammar.y"
    { gen_and(yyvsp[-2].blk.b, yyvsp[0].blk.b); yyval.blk = yyvsp[0].blk; }
    break;

  case 7:
#line 165 "grammar.y"
    { gen_and(yyvsp[-2].blk.b, yyvsp[0].blk.b); yyval.blk = yyvsp[0].blk; }
    break;

  case 8:
#line 166 "grammar.y"
    { gen_or(yyvsp[-2].blk.b, yyvsp[0].blk.b); yyval.blk = yyvsp[0].blk; }
    break;

  case 9:
#line 167 "grammar.y"
    { gen_or(yyvsp[-2].blk.b, yyvsp[0].blk.b); yyval.blk = yyvsp[0].blk; }
    break;

  case 10:
#line 169 "grammar.y"
    { yyval.blk = yyvsp[-1].blk; }
    break;

  case 11:
#line 171 "grammar.y"
    { yyval.blk = yyvsp[-1].blk; }
    break;

  case 13:
#line 174 "grammar.y"
    { yyval.blk.b = gen_ncode(NULL, (bpf_u_int32)yyvsp[0].i,
						   yyval.blk.q = yyvsp[-1].blk.q); }
    break;

  case 14:
#line 176 "grammar.y"
    { yyval.blk = yyvsp[-1].blk; }
    break;

  case 15:
#line 178 "grammar.y"
    { yyval.blk.b = gen_scode(yyvsp[0].s, yyval.blk.q = yyvsp[-1].blk.q); }
    break;

  case 16:
#line 179 "grammar.y"
    { yyval.blk.b = gen_mcode(yyvsp[-2].s, NULL, yyvsp[0].i,
				    yyval.blk.q = yyvsp[-3].blk.q); }
    break;

  case 17:
#line 181 "grammar.y"
    { yyval.blk.b = gen_mcode(yyvsp[-2].s, yyvsp[0].s, 0,
				    yyval.blk.q = yyvsp[-3].blk.q); }
    break;

  case 18:
#line 183 "grammar.y"
    {
				  /* Decide how to parse HID based on proto */
				  yyval.blk.q = yyvsp[-1].blk.q;
				  yyval.blk.b = gen_ncode(yyvsp[0].s, 0, yyval.blk.q);
				}
    break;

  case 19:
#line 188 "grammar.y"
    {
#ifdef INET6
				  yyval.blk.b = gen_mcode6(yyvsp[-2].s, NULL, yyvsp[0].i,
				    yyval.blk.q = yyvsp[-3].blk.q);
#else
				  bpf_error("'ip6addr/prefixlen' not supported "
					"in this configuration");
#endif /*INET6*/
				}
    break;

  case 20:
#line 197 "grammar.y"
    {
#ifdef INET6
				  yyval.blk.b = gen_mcode6(yyvsp[0].s, 0, 128,
				    yyval.blk.q = yyvsp[-1].blk.q);
#else
				  bpf_error("'ip6addr' not supported "
					"in this configuration");
#endif /*INET6*/
				}
    break;

  case 21:
#line 206 "grammar.y"
    { 
				  yyval.blk.b = gen_ecode(yyvsp[0].e, yyval.blk.q = yyvsp[-1].blk.q);
				  /*
				   * $1 was allocated by "pcap_ether_aton()",
				   * so we must free it now that we're done
				   * with it.
				   */
				  free(yyvsp[0].e);
				}
    break;

  case 22:
#line 215 "grammar.y"
    {
				  yyval.blk.b = gen_acode(yyvsp[0].e, yyval.blk.q = yyvsp[-1].blk.q);
				  /*
				   * $1 was allocated by "pcap_ether_aton()",
				   * so we must free it now that we're done
				   * with it.
				   */
				  free(yyvsp[0].e);
				}
    break;

  case 23:
#line 224 "grammar.y"
    { gen_not(yyvsp[0].blk.b); yyval.blk = yyvsp[0].blk; }
    break;

  case 24:
#line 226 "grammar.y"
    { yyval.blk = yyvsp[-1].blk; }
    break;

  case 25:
#line 228 "grammar.y"
    { yyval.blk = yyvsp[-1].blk; }
    break;

  case 27:
#line 231 "grammar.y"
    { gen_and(yyvsp[-2].blk.b, yyvsp[0].blk.b); yyval.blk = yyvsp[0].blk; }
    break;

  case 28:
#line 232 "grammar.y"
    { gen_or(yyvsp[-2].blk.b, yyvsp[0].blk.b); yyval.blk = yyvsp[0].blk; }
    break;

  case 29:
#line 234 "grammar.y"
    { yyval.blk.b = gen_ncode(NULL, (bpf_u_int32)yyvsp[0].i,
						   yyval.blk.q = yyvsp[-1].blk.q); }
    break;

  case 32:
#line 239 "grammar.y"
    { gen_not(yyvsp[0].blk.b); yyval.blk = yyvsp[0].blk; }
    break;

  case 33:
#line 241 "grammar.y"
    { QSET(yyval.blk.q, yyvsp[-2].i, yyvsp[-1].i, yyvsp[0].i); }
    break;

  case 34:
#line 242 "grammar.y"
    { QSET(yyval.blk.q, yyvsp[-1].i, yyvsp[0].i, Q_DEFAULT); }
    break;

  case 35:
#line 243 "grammar.y"
    { QSET(yyval.blk.q, yyvsp[-1].i, Q_DEFAULT, yyvsp[0].i); }
    break;

  case 36:
#line 244 "grammar.y"
    { QSET(yyval.blk.q, yyvsp[-1].i, Q_DEFAULT, Q_PROTO); }
    break;

  case 37:
#line 245 "grammar.y"
    { QSET(yyval.blk.q, yyvsp[-1].i, Q_DEFAULT, Q_PROTOCHAIN); }
    break;

  case 38:
#line 246 "grammar.y"
    { QSET(yyval.blk.q, yyvsp[-1].i, Q_DEFAULT, yyvsp[0].i); }
    break;

  case 39:
#line 248 "grammar.y"
    { yyval.blk = yyvsp[0].blk; }
    break;

  case 40:
#line 249 "grammar.y"
    { yyval.blk.b = yyvsp[-1].blk.b; yyval.blk.q = yyvsp[-2].blk.q; }
    break;

  case 41:
#line 250 "grammar.y"
    { yyval.blk.b = gen_proto_abbrev(yyvsp[0].i); yyval.blk.q = qerr; }
    break;

  case 42:
#line 251 "grammar.y"
    { yyval.blk.b = gen_relation(yyvsp[-1].i, yyvsp[-2].a, yyvsp[0].a, 0);
				  yyval.blk.q = qerr; }
    break;

  case 43:
#line 253 "grammar.y"
    { yyval.blk.b = gen_relation(yyvsp[-1].i, yyvsp[-2].a, yyvsp[0].a, 1);
				  yyval.blk.q = qerr; }
    break;

  case 44:
#line 255 "grammar.y"
    { yyval.blk.b = yyvsp[0].rblk; yyval.blk.q = qerr; }
    break;

  case 45:
#line 256 "grammar.y"
    { yyval.blk.b = gen_atmtype_abbrev(yyvsp[0].i); yyval.blk.q = qerr; }
    break;

  case 46:
#line 257 "grammar.y"
    { yyval.blk.b = gen_atmmulti_abbrev(yyvsp[0].i); yyval.blk.q = qerr; }
    break;

  case 47:
#line 258 "grammar.y"
    { yyval.blk.b = yyvsp[0].blk.b; yyval.blk.q = qerr; }
    break;

  case 49:
#line 262 "grammar.y"
    { yyval.i = Q_DEFAULT; }
    break;

  case 50:
#line 265 "grammar.y"
    { yyval.i = Q_SRC; }
    break;

  case 51:
#line 266 "grammar.y"
    { yyval.i = Q_DST; }
    break;

  case 52:
#line 267 "grammar.y"
    { yyval.i = Q_OR; }
    break;

  case 53:
#line 268 "grammar.y"
    { yyval.i = Q_OR; }
    break;

  case 54:
#line 269 "grammar.y"
    { yyval.i = Q_AND; }
    break;

  case 55:
#line 270 "grammar.y"
    { yyval.i = Q_AND; }
    break;

  case 56:
#line 273 "grammar.y"
    { yyval.i = Q_HOST; }
    break;

  case 57:
#line 274 "grammar.y"
    { yyval.i = Q_NET; }
    break;

  case 58:
#line 275 "grammar.y"
    { yyval.i = Q_PORT; }
    break;

  case 59:
#line 278 "grammar.y"
    { yyval.i = Q_GATEWAY; }
    break;

  case 60:
#line 280 "grammar.y"
    { yyval.i = Q_LINK; }
    break;

  case 61:
#line 281 "grammar.y"
    { yyval.i = Q_IP; }
    break;

  case 62:
#line 282 "grammar.y"
    { yyval.i = Q_ARP; }
    break;

  case 63:
#line 283 "grammar.y"
    { yyval.i = Q_RARP; }
    break;

  case 64:
#line 284 "grammar.y"
    { yyval.i = Q_SCTP; }
    break;

  case 65:
#line 285 "grammar.y"
    { yyval.i = Q_TCP; }
    break;

  case 66:
#line 286 "grammar.y"
    { yyval.i = Q_UDP; }
    break;

  case 67:
#line 287 "grammar.y"
    { yyval.i = Q_ICMP; }
    break;

  case 68:
#line 288 "grammar.y"
    { yyval.i = Q_IGMP; }
    break;

  case 69:
#line 289 "grammar.y"
    { yyval.i = Q_IGRP; }
    break;

  case 70:
#line 290 "grammar.y"
    { yyval.i = Q_PIM; }
    break;

  case 71:
#line 291 "grammar.y"
    { yyval.i = Q_VRRP; }
    break;

  case 72:
#line 292 "grammar.y"
    { yyval.i = Q_ATALK; }
    break;

  case 73:
#line 293 "grammar.y"
    { yyval.i = Q_AARP; }
    break;

  case 74:
#line 294 "grammar.y"
    { yyval.i = Q_DECNET; }
    break;

  case 75:
#line 295 "grammar.y"
    { yyval.i = Q_LAT; }
    break;

  case 76:
#line 296 "grammar.y"
    { yyval.i = Q_SCA; }
    break;

  case 77:
#line 297 "grammar.y"
    { yyval.i = Q_MOPDL; }
    break;

  case 78:
#line 298 "grammar.y"
    { yyval.i = Q_MOPRC; }
    break;

  case 79:
#line 299 "grammar.y"
    { yyval.i = Q_IPV6; }
    break;

  case 80:
#line 300 "grammar.y"
    { yyval.i = Q_ICMPV6; }
    break;

  case 81:
#line 301 "grammar.y"
    { yyval.i = Q_AH; }
    break;

  case 82:
#line 302 "grammar.y"
    { yyval.i = Q_ESP; }
    break;

  case 83:
#line 303 "grammar.y"
    { yyval.i = Q_ISO; }
    break;

  case 84:
#line 304 "grammar.y"
    { yyval.i = Q_ESIS; }
    break;

  case 85:
#line 305 "grammar.y"
    { yyval.i = Q_ISIS; }
    break;

  case 86:
#line 306 "grammar.y"
    { yyval.i = Q_ISIS_L1; }
    break;

  case 87:
#line 307 "grammar.y"
    { yyval.i = Q_ISIS_L2; }
    break;

  case 88:
#line 308 "grammar.y"
    { yyval.i = Q_ISIS_IIH; }
    break;

  case 89:
#line 309 "grammar.y"
    { yyval.i = Q_ISIS_LSP; }
    break;

  case 90:
#line 310 "grammar.y"
    { yyval.i = Q_ISIS_SNP; }
    break;

  case 91:
#line 311 "grammar.y"
    { yyval.i = Q_ISIS_PSNP; }
    break;

  case 92:
#line 312 "grammar.y"
    { yyval.i = Q_ISIS_CSNP; }
    break;

  case 93:
#line 313 "grammar.y"
    { yyval.i = Q_CLNP; }
    break;

  case 94:
#line 314 "grammar.y"
    { yyval.i = Q_STP; }
    break;

  case 95:
#line 315 "grammar.y"
    { yyval.i = Q_IPX; }
    break;

  case 96:
#line 316 "grammar.y"
    { yyval.i = Q_NETBEUI; }
    break;

  case 97:
#line 318 "grammar.y"
    { yyval.rblk = gen_broadcast(yyvsp[-1].i); }
    break;

  case 98:
#line 319 "grammar.y"
    { yyval.rblk = gen_multicast(yyvsp[-1].i); }
    break;

  case 99:
#line 320 "grammar.y"
    { yyval.rblk = gen_less(yyvsp[0].i); }
    break;

  case 100:
#line 321 "grammar.y"
    { yyval.rblk = gen_greater(yyvsp[0].i); }
    break;

  case 101:
#line 322 "grammar.y"
    { yyval.rblk = gen_byteop(yyvsp[-1].i, yyvsp[-2].i, yyvsp[0].i); }
    break;

  case 102:
#line 323 "grammar.y"
    { yyval.rblk = gen_inbound(0); }
    break;

  case 103:
#line 324 "grammar.y"
    { yyval.rblk = gen_inbound(1); }
    break;

  case 104:
#line 325 "grammar.y"
    { yyval.rblk = gen_vlan(yyvsp[0].i); }
    break;

  case 105:
#line 326 "grammar.y"
    { yyval.rblk = gen_vlan(-1); }
    break;

  case 106:
#line 327 "grammar.y"
    { yyval.rblk = gen_mpls(yyvsp[0].i); }
    break;

  case 107:
#line 328 "grammar.y"
    { yyval.rblk = gen_mpls(-1); }
    break;

  case 108:
#line 329 "grammar.y"
    { yyval.rblk = yyvsp[0].rblk; }
    break;

  case 109:
#line 332 "grammar.y"
    { yyval.rblk = gen_pf_ifname(yyvsp[0].s); }
    break;

  case 110:
#line 333 "grammar.y"
    { yyval.rblk = gen_pf_ruleset(yyvsp[0].s); }
    break;

  case 111:
#line 334 "grammar.y"
    { yyval.rblk = gen_pf_rnr(yyvsp[0].i); }
    break;

  case 112:
#line 335 "grammar.y"
    { yyval.rblk = gen_pf_srnr(yyvsp[0].i); }
    break;

  case 113:
#line 336 "grammar.y"
    { yyval.rblk = gen_pf_reason(yyvsp[0].i); }
    break;

  case 114:
#line 337 "grammar.y"
    { yyval.rblk = gen_pf_action(yyvsp[0].i); }
    break;

  case 115:
#line 340 "grammar.y"
    { yyval.i = yyvsp[0].i; }
    break;

  case 116:
#line 341 "grammar.y"
    { const char *reasons[] = PFRES_NAMES;
				  int i;
				  for (i = 0; reasons[i]; i++) {
					  if (pcap_strcasecmp(yyvsp[0].s, reasons[i]) == 0) {
						  yyval.i = i;
						  break;
					  }
				  }
				  if (reasons[i] == NULL)
					  bpf_error("unknown PF reason");
				}
    break;

  case 117:
#line 354 "grammar.y"
    { if (pcap_strcasecmp(yyvsp[0].s, "pass") == 0 ||
				      pcap_strcasecmp(yyvsp[0].s, "accept") == 0)
					yyval.i = PF_PASS;
				  else if (pcap_strcasecmp(yyvsp[0].s, "drop") == 0 ||
				      pcap_strcasecmp(yyvsp[0].s, "block") == 0)
					yyval.i = PF_DROP;
				  else
					  bpf_error("unknown PF action");
				}
    break;

  case 118:
#line 365 "grammar.y"
    { yyval.i = BPF_JGT; }
    break;

  case 119:
#line 366 "grammar.y"
    { yyval.i = BPF_JGE; }
    break;

  case 120:
#line 367 "grammar.y"
    { yyval.i = BPF_JEQ; }
    break;

  case 121:
#line 369 "grammar.y"
    { yyval.i = BPF_JGT; }
    break;

  case 122:
#line 370 "grammar.y"
    { yyval.i = BPF_JGE; }
    break;

  case 123:
#line 371 "grammar.y"
    { yyval.i = BPF_JEQ; }
    break;

  case 124:
#line 373 "grammar.y"
    { yyval.a = gen_loadi(yyvsp[0].i); }
    break;

  case 126:
#line 376 "grammar.y"
    { yyval.a = gen_load(yyvsp[-3].i, yyvsp[-1].a, 1); }
    break;

  case 127:
#line 377 "grammar.y"
    { yyval.a = gen_load(yyvsp[-5].i, yyvsp[-3].a, yyvsp[-1].i); }
    break;

  case 128:
#line 378 "grammar.y"
    { yyval.a = gen_arth(BPF_ADD, yyvsp[-2].a, yyvsp[0].a); }
    break;

  case 129:
#line 379 "grammar.y"
    { yyval.a = gen_arth(BPF_SUB, yyvsp[-2].a, yyvsp[0].a); }
    break;

  case 130:
#line 380 "grammar.y"
    { yyval.a = gen_arth(BPF_MUL, yyvsp[-2].a, yyvsp[0].a); }
    break;

  case 131:
#line 381 "grammar.y"
    { yyval.a = gen_arth(BPF_DIV, yyvsp[-2].a, yyvsp[0].a); }
    break;

  case 132:
#line 382 "grammar.y"
    { yyval.a = gen_arth(BPF_AND, yyvsp[-2].a, yyvsp[0].a); }
    break;

  case 133:
#line 383 "grammar.y"
    { yyval.a = gen_arth(BPF_OR, yyvsp[-2].a, yyvsp[0].a); }
    break;

  case 134:
#line 384 "grammar.y"
    { yyval.a = gen_arth(BPF_LSH, yyvsp[-2].a, yyvsp[0].a); }
    break;

  case 135:
#line 385 "grammar.y"
    { yyval.a = gen_arth(BPF_RSH, yyvsp[-2].a, yyvsp[0].a); }
    break;

  case 136:
#line 386 "grammar.y"
    { yyval.a = gen_neg(yyvsp[0].a); }
    break;

  case 137:
#line 387 "grammar.y"
    { yyval.a = yyvsp[-1].a; }
    break;

  case 138:
#line 388 "grammar.y"
    { yyval.a = gen_loadlen(); }
    break;

  case 139:
#line 390 "grammar.y"
    { yyval.i = '&'; }
    break;

  case 140:
#line 391 "grammar.y"
    { yyval.i = '|'; }
    break;

  case 141:
#line 392 "grammar.y"
    { yyval.i = '<'; }
    break;

  case 142:
#line 393 "grammar.y"
    { yyval.i = '>'; }
    break;

  case 143:
#line 394 "grammar.y"
    { yyval.i = '='; }
    break;

  case 145:
#line 397 "grammar.y"
    { yyval.i = yyvsp[-1].i; }
    break;

  case 146:
#line 399 "grammar.y"
    { yyval.i = A_LANE; }
    break;

  case 147:
#line 400 "grammar.y"
    { yyval.i = A_LLC; }
    break;

  case 148:
#line 401 "grammar.y"
    { yyval.i = A_METAC;	}
    break;

  case 149:
#line 402 "grammar.y"
    { yyval.i = A_BCC; }
    break;

  case 150:
#line 403 "grammar.y"
    { yyval.i = A_OAMF4EC; }
    break;

  case 151:
#line 404 "grammar.y"
    { yyval.i = A_OAMF4SC; }
    break;

  case 152:
#line 405 "grammar.y"
    { yyval.i = A_SC; }
    break;

  case 153:
#line 406 "grammar.y"
    { yyval.i = A_ILMIC; }
    break;

  case 154:
#line 408 "grammar.y"
    { yyval.i = A_OAM; }
    break;

  case 155:
#line 409 "grammar.y"
    { yyval.i = A_OAMF4; }
    break;

  case 156:
#line 410 "grammar.y"
    { yyval.i = A_CONNECTMSG; }
    break;

  case 157:
#line 411 "grammar.y"
    { yyval.i = A_METACONNECT; }
    break;

  case 158:
#line 414 "grammar.y"
    { yyval.blk.atmfieldtype = A_VPI; }
    break;

  case 159:
#line 415 "grammar.y"
    { yyval.blk.atmfieldtype = A_VCI; }
    break;

  case 161:
#line 418 "grammar.y"
    { yyval.blk.b = gen_atmfield_code(yyvsp[-2].blk.atmfieldtype, (u_int)yyvsp[0].i, (u_int)yyvsp[-1].i, 0); }
    break;

  case 162:
#line 419 "grammar.y"
    { yyval.blk.b = gen_atmfield_code(yyvsp[-2].blk.atmfieldtype, (u_int)yyvsp[0].i, (u_int)yyvsp[-1].i, 1); }
    break;

  case 163:
#line 420 "grammar.y"
    { yyval.blk.b = yyvsp[-1].blk.b; yyval.blk.q = qerr; }
    break;

  case 164:
#line 422 "grammar.y"
    {
	yyval.blk.atmfieldtype = yyvsp[-1].blk.atmfieldtype;
	if (yyval.blk.atmfieldtype == A_VPI ||
	    yyval.blk.atmfieldtype == A_VCI)
		yyval.blk.b = gen_atmfield_code(yyval.blk.atmfieldtype, (u_int) yyvsp[0].i, BPF_JEQ, 0);
	}
    break;

  case 166:
#line 430 "grammar.y"
    { gen_or(yyvsp[-2].blk.b, yyvsp[0].blk.b); yyval.blk = yyvsp[0].blk; }
    break;


    }

/* Line 999 of yacc.c.  */
#line 2410 "y.tab.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("syntax error, unexpected ") + 1;
	  yysize += yystrlen (yytname[yytype]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			const char *yyq = ! yycount ? ", expecting " : " or ";
			yyp = yystpcpy (yyp, yyq);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yycount++;
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* Return failure if at end of input.  */
      if (yychar == YYEOF)
        {
	  /* Pop the error token.  */
          YYPOPSTACK;
	  /* Pop the rest of the stack.  */
	  while (yyss < yyssp)
	    {
	      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
	      yydestruct (yystos[*yyssp], yyvsp);
	      YYPOPSTACK;
	    }
	  YYABORT;
        }

      YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
      yydestruct (yytoken, &yylval);
      yychar = YYEMPTY;

    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*----------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action.  |
`----------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      yyvsp--;
      yystate = *--yyssp;

      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 432 "grammar.y"


