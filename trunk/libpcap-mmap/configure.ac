e Process this file with autoconf to produce a configure script.
# Based on work done at the University of California at Berkeley during
# 1994, 1995, 1996, 1997, as well as on going work at "tcpdump.org".

AC_INIT(libpcap,1.9,cpw@lanl.gov)
AC_CONFIG_AUX_DIR(config)
# AM isms
AM_INIT_AUTOMAKE(libpcap,0.9x.20070323)
AM_CONFIG_HEADER(config.h)
AC_CONFIG_SRCDIR(pcap.c)

AC_CANONICAL_HOST

# Initialize some conditionals

AM_CONDITIONAL(USE_DAG, false)
AM_CONDITIONAL(USE_SEPTEL, false)
AM_CONDITIONAL(USE_RING, false)

AC_LBL_C_INIT(V_CCOPT, V_INCLS, V_LIBS)

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_YACC
AC_DISABLE_SHARED
AC_PROG_LIBTOOL

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_DIRENT

dnl
dnl libpcap doesn't itself use <sys/ioccom.h>; however, the test program
dnl in "AC_LBL_FIXINCLUDES" in "aclocal.m4" uses it, so we have to
dnl test for it and set "HAVE_SYS_IOCCOM_H" if we have it, otherwise
dnl "AC_LBL_FIXINCLUDES" won't work on some platforms such as Solaris.
dnl
AC_CHECK_HEADERS(sys/ioccom.h sys/sockio.h ifaddrs.h limits.h)
AC_CHECK_HEADERS(netinet/if_ether.h, , , [#include <sys/types.h>
#include <sys/socket.h>])

# Checks for typedefs, structures, and compiler characteristics.
AC_LBL_C_INLINE
AC_C___ATTRIBUTE__
AC_LBL_FIXINCLUDES
AC_LBL_CHECK_TYPE(u_int8_t, u_char)
AC_LBL_CHECK_TYPE(u_int16_t, u_short)
AC_LBL_CHECK_TYPE(u_int32_t, u_int)


AC_C_CONST
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_HEADER_TIME
AC_STRUCT_TM

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_HEADER_MAJOR
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([ether_hostton strerror strlcpy])
if test "$ac_cv_func_ether_hostton" = yes; then
	#
	# OK, we have ether_hostton().  Do we have <netinet/if_ether.h>?
	#
	if test "$ac_cv_header_netinet_if_ether_h" = yes; then
		#
		# Yes.  Does it declare ether_hostton()?
		#
		AC_CHECK_DECL(ether_hostton,
		    [
			AC_DEFINE(NETINET_IF_ETHER_H_DECLARES_ETHER_HOSTTON,,
			    [Define to 1 if netinet/if_ether.h declares `ether_hostton'])
		    ],,
		    [
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
struct mbuf;
struct rtentry;
#include <net/if.h>
#include <netinet/if_ether.h>
		    ])
	fi
	#
	# Did that succeed?
	#
	if test "$ac_cv_have_decl_ether_hostton" != yes; then
		#
		# No, how about <netinet/ether.h>, as on Linux?
		#
		AC_CHECK_HEADERS(netinet/ether.h)
		if test "$ac_cv_header_netinet_ether_h" = yes; then
			#
			# We have it - does it declare ether_hostton()?
			# Unset ac_cv_have_decl_ether_hostton so we don't
			# treat the previous failure as a cached value and
			# suppress the next test.
			#
			unset ac_cv_have_decl_ether_hostton
			AC_CHECK_DECL(ether_hostton,
			    [
				AC_DEFINE(NETINET_ETHER_H_DECLARES_ETHER_HOSTTON,,
				    [Define to 1 if netinet/ether.h declares `ether_hostton'])
			    ],,
			    [
#include <netinet/ether.h>
			    ])
		fi
	fi
	#
	# Is ether_hostton() declared?
	#
	if test "$ac_cv_have_decl_ether_hostton" != yes; then
		#
		# No, we'll have to declare it ourselves.
		# Do we have "struct ether_addr"?
		#
		AC_CHECK_TYPES(struct ether_addr,,,
		    [
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
struct mbuf;
struct rtentry;
#include <net/if.h>
#include <netinet/if_ether.h>
		    ])
		AC_DEFINE(HAVE_DECL_ETHER_HOSTTON, 0,
		    [Define to 1 if you have the declaration of `ether_hostton', and to 0 if you
don't.])
	else
		AC_DEFINE(HAVE_DECL_ETHER_HOSTTON, 1,
		    [Define to 1 if you have the declaration of `ether_hostton', and to 0 if you don't.])
	fi
fi

needsnprintf=no
AC_CHECK_FUNCS(vsnprintf snprintf,,
	[needsnprintf=yes])
if test $needsnprintf = yes; then
        AC_LIBOBJ([snprintf])
fi

dnl to pacify those who hate protochain insn
AC_MSG_CHECKING(if --disable-protochain option is specified)
AC_ARG_ENABLE(protochain, [  --disable-protochain    disable \"protochain\" insn] )

case "x$enable_protochain" in
xyes)	enable_protochain=enabled	;;
xno)	enable_protochain=disabled	;;
x)	enable_protochain=enabled	;;
esac

if test "$enable_protochain" = "disabled"; then
	AC_DEFINE(NO_PROTOCHAIN,1,[do not use protochain])
fi
AC_MSG_RESULT(${enable_protochain})

dnl
dnl Not all versions of test support -c (character special) but it's a
dnl better way of testing since the device might be protected. So we
dnl check in our normal order using -r and then check the for the /dev
dnl guys again using -c.
dnl
dnl XXX This could be done for cross-compiling, but for now it's not.
dnl
if test -z "$with_pcap" && test "$cross_compiling" = yes; then
	AC_MSG_ERROR(pcap type not determined when cross-compiling; use --with-pcap=...)
fi
AC_ARG_WITH(pcap, [  --with-pcap=TYPE        use packet capture TYPE])
AC_MSG_CHECKING(packet capture type)
if test ! -z "$with_pcap" ; then
	V_PCAP="$withval"
elif test -r /dev/bpf0 ; then
	V_PCAP=bpf
elif test -r /usr/include/net/pfilt.h ; then
	V_PCAP=pf
elif test -r /dev/enet ; then
	V_PCAP=enet
elif test -r /dev/nit ; then
	V_PCAP=snit
elif test -r /usr/include/sys/net/nit.h ; then
	V_PCAP=nit
elif test -r /usr/include/linux/socket.h ; then
	V_PCAP=linux
elif test -r /usr/include/net/raw.h ; then
	V_PCAP=snoop
elif test -r /usr/include/odmi.h ; then
        #
        # On AIX, the BPF devices might not yet be present - they're
        # created the first time libpcap runs after booting.
        # We check for odmi.h instead.
        #
        V_PCAP=bpf
elif test -r /usr/include/sys/dlpi.h ; then
	V_PCAP=dlpi
elif test -c /dev/bpf0 ; then		# check again in case not readable
	V_PCAP=bpf
elif test -c /dev/enet ; then		# check again in case not readable
	V_PCAP=enet
elif test -c /dev/nit ; then		# check again in case not readable
	V_PCAP=snit
else
	V_PCAP=null
fi
AC_MSG_RESULT($V_PCAP)
AC_MSG_CHECKING(generating other os sources)
V_OTHERS="`grep -v $V_PCAP PCAPPERS | tr '\012' ' '`"
AC_MSG_RESULT($V_OTHERS)

dnl
dnl Now figure out how we get a list of interfaces and addresses.
dnl
AC_CHECK_FUNC(getifaddrs,[
	#
	# We have "getifaddrs()", so we use that to get the list
	#
	V_FINDALLDEVS=getad
],[
	#
	# Well, we don't have "getifaddrs()", so we have to use some
	# other mechanism; determine what that mechanism is.
	#
	# The first thing we use is the type of capture mechanism,
	# which is somewhat of a proxy for the OS we're using.
	#
	case "$V_PCAP" in

	dlpi)
		#
		# This might be Solaris 8 or later, with SIOCGLIFCONF,
		# or it might be some other OS, with just SIOCGIFCONF.
		#
		AC_MSG_CHECKING(whether we have SIOCGLIFCONF)
		AC_CACHE_VAL(ac_cv_lbl_have_siocglifconf,
		    AC_TRY_COMPILE(
			[#include <sys/param.h>
			#include <sys/file.h>
			#include <sys/ioctl.h>
			#include <sys/socket.h>
			#include <sys/sockio.h>],
			[ioctl(0, SIOCGLIFCONF, (char *)0);],
			ac_cv_lbl_have_siocglifconf=yes,
			ac_cv_lbl_have_siocglifconf=no))
		AC_MSG_RESULT($ac_cv_lbl_have_siocglifconf)
		if test $ac_cv_lbl_have_siocglifconf = yes ; then
			V_FINDALLDEVS=glifc
		else
			V_FINDALLDEVS=gifc
		fi
		;;

	null)
		#
		# We can't capture, so we can't open any capture
		# devices, so we won't return any interfaces.
		#
		V_FINDALLDEVS=null
		;;

	*)
		#
		# Assume we just have SIOCGIFCONF.
		# (XXX - on at least later Linux kernels, there's
		# another mechanism, and we should be using that
		# instead.)
		#
		V_FINDALLDEVS=gifc
		;;
	esac])

AC_MSG_CHECKING(if --enable-ipv6 option is specified)
AC_ARG_ENABLE(ipv6, [  --enable-ipv6           build IPv6-capable version])
if test "$enable_ipv6" = "yes"; then
	AC_DEFINE(INET6,1,[IPv6])
fi
AC_MSG_RESULT(${enable_ipv6-no})

AC_MSG_CHECKING(whether to build optimizer debugging code)
AC_ARG_ENABLE(optimizer-dbg, [  --enable-optimizer-dbg  build optimizer debugging code])
if test "$enable_optimizer_dbg" = "yes"; then
	AC_DEFINE(BDEBUG,1,[Enable optimizer debugging])
fi
AC_MSG_RESULT(${enable_optimizer_dbg-no})

AC_MSG_CHECKING(whether to build parser debugging code)
AC_ARG_ENABLE(yydebug, [  --enable-yydebug        build parser debugging code])
if test "$enable_yydebug" = "yes"; then
	AC_DEFINE(YYDEBUG,1,[Enable parser debugging])
fi
AC_MSG_RESULT(${enable_yydebug-no})

case "$V_PCAP" in

dlpi)
	AC_CHECK_HEADERS(sys/bufmod.h sys/dlpi_ext.h)
	AC_MSG_CHECKING(for /dev/dlpi device)
	if test -c /dev/dlpi ; then
		AC_MSG_RESULT(yes)
		AC_DEFINE(HAVE_DEV_DLPI, 1, [define if you have a /dev/dlpi])
	else
		AC_MSG_RESULT(no)
		dir="/dev/dlpi"
		AC_MSG_CHECKING(for $dir directory)
		if test -d $dir ; then
			AC_MSG_RESULT(yes)
			AC_DEFINE_UNQUOTED(PCAP_DEV_PREFIX, "$dir", [/dev/dlpi directory])
		else
			AC_MSG_RESULT(no)
		fi
	fi
	;;

linux)
	V_DEFS="$V_DEFS -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE"
	AC_MSG_CHECKING(Linux kernel version)
 	if test "$cross_compiling" = yes; then
 		AC_CACHE_VAL(ac_cv_linux_vers,
 		    ac_cv_linux_vers=unknown)
 	else
 		AC_CACHE_VAL(ac_cv_linux_vers,
 		    ac_cv_linux_vers=`uname -r 2>&1 | \
 			sed -n -e '$s/.* //' -e '$s/\..*//p'`)
 	fi
	AC_MSG_RESULT($ac_cv_linux_vers)
 	if test $ac_cv_linux_vers = unknown ; then
 		AC_MSG_ERROR(cannot determine linux version when cross-compiling)
 	fi
	if test $ac_cv_linux_vers -lt 2 ; then
		AC_MSG_ERROR(version 2 or higher required; see the INSTALL doc for more info)
	fi
	AC_LBL_TPACKET_STATS
	AC_CPW_DO_RING
        AM_CONDITIONAL(USE_RING, test $ac_cv_cpw_ring_method = yes)
	AC_CPW_TIMESPEC
	;;

dag)
        V_DEFS="$V_DEFS -DDAG_ONLY"
        ;;      

septel)
        V_DEFS="$V_DEFS -DSEPTEL_ONLY"
        ;;      

null)
	AC_MSG_WARN(cannot determine packet capture interface)
	AC_MSG_WARN((see the INSTALL doc for more info))
	;;

esac

AC_MSG_CHECKING(whether we have /proc/net/dev)
if test -r /proc/net/dev ; then
	ac_cv_lbl_proc_net_dev=yes
else
	ac_cv_lbl_proc_net_dev=no
fi
if test $ac_cv_lbl_proc_net_dev = yes; then
	AC_DEFINE(HAVE_PROC_NET_DEV, 1, [define if you have a /proc/net/dev])
fi
AC_MSG_RESULT($ac_cv_lbl_proc_net_dev)

case "$V_PCAP" in
linux|bpf|dag)
	#
	# We support the DAG API if we're on Linux or BSD, or if we're
	# building a DAG-only libpcap.
	#
	;;
*)
	#
	# If the user explicitly requested DAG, tell them it's not
	# supported.
	#
	# If they expressed no preference, don't include it.
	#
	if test $want_dag = yes; then
		AC_MSG_ERROR([DAG support is only available with 'linux' 'bpf' and 'dag' packet capture types])
	elif test $want_dag = yes; then
		want_dag=no
	fi
	;;
esac

ac_cv_lbl_dag_api=no
if test "$want_dag" != no; then

	AC_MSG_CHECKING([whether we have DAG API headers])

	# If necessary, set default paths for DAG API headers and libraries.
	if test -z "$dag_root"; then
	    dag_root=/usr/local
	fi

	if test -z "$dag_include_dir"; then
		dag_include_dir="$dag_root/include"
	fi

	if test -z "$dag_lib_dir"; then
	    dag_lib_dir="$dag_root/lib"
	fi
	
	if test -z "$dag_tools_dir"; then
	    dag_tools_dir="$dag_root/tools"
		fi

	if test -r $dag_include_dir/dagapi.h; then
		ac_cv_lbl_dag_api=yes
	fi
	AC_MSG_RESULT([$ac_cv_lbl_dag_api ($dag_include_dir)])
fi

if test $ac_cv_lbl_dag_api = yes; then

	AC_MSG_CHECKING([dagapi.o])	
	dagapi_obj=no
	if test -r $dag_tools_dir/dagapi.o; then
		# 2.4.x.
		dagapi_obj=$dag_tools_dir/dagapi.o
	elif test -r $dag_lib_dir/dagapi.o; then
		# 2.5.x.
		dagapi_obj=$dag_lib_dir/dagapi.o
	elif test -r $dag_lib_dir/libdag.a; then
		# 2.5.x.
		ar x $dag_lib_dir/libdag.a dagapi.o
		if test -r ./dagapi.o; then
		    dagapi_obj=./dagapi.o
		fi
	fi

	if test $dagapi_obj = no; then
		AC_MSG_RESULT([no (checked $dag_lib_dir  $dag_tools_dir  $dag_lib_dir/libdag.a)])
			ac_cv_lbl_dag_api=no
	else
		AC_MSG_RESULT([yes ($dagapi_obj)])
	fi
fi

if test $ac_cv_lbl_dag_api = yes; then

	AC_MSG_CHECKING([dagopts.o])	
	dagopts_obj=no
	if test -r $dag_tools_dir/dagopts.o; then
		# 2.4.x.
		dagopts_obj=$dag_tools_dir/dagopts.o
	elif test -r $dag_lib_dir/dagopts.o; then
		# 2.5.x.
		dagopts_obj=$dag_lib_dir/dagopts.o
	elif test -r $dag_lib_dir/libdag.a; then
		# 2.5.x.
		ar x $dag_lib_dir/libdag.a dagopts.o
		if test -r ./dagopts.o; then
		    dagopts_obj=./dagopts.o
		fi
	fi

	if test $dagopts_obj = no; then
		AC_MSG_RESULT([no (checked $dag_lib_dir  $dag_tools_dir  $dag_lib_dir/libdag.a)])
		ac_cv_lbl_dag_api=no
	else
		AC_MSG_RESULT([yes ($dagopts_obj)])
	fi
fi

if test $ac_cv_lbl_dag_api = yes; then
	# Under 2.5.x only we need to add dagreg.o.
	if test -r $dag_include_dir/dagreg.h; then
		AC_MSG_CHECKING([dagreg.o])	
		dagreg_obj=no
		if test -r $dag_lib_dir/dagreg.o; then
			# Object file is ready and waiting.
			dagreg_obj=$dag_lib_dir/dagreg.o
		elif test -r $dag_lib_dir/libdag.a; then
			# Extract from libdag.a.
			ar x $dag_lib_dir/libdag.a dagreg.o
			if test -r ./dagreg.o; then
				dagreg_obj=./dagreg.o
			fi
		fi

		if test $dagreg_obj = no; then
			AC_MSG_RESULT([no (checked $dag_lib_dir  $dag_lib_dir/libdag.a)])
			ac_cv_lbl_dag_api=no
		else
			AC_MSG_RESULT([yes ($dagreg_obj)])
		fi
	fi
fi

if test $ac_cv_lbl_dag_api = yes; then
	V_INCLS="$V_INCLS -I$dag_include_dir"
	V_LIBS="$V_LIBS $dagapi_obj $dagopts_obj $dagreg_obj"
	if test $V_PCAP != dag ; then
		 SSRC="pcap-dag.c"
	fi

	# See if we can find a general version string.
	# Don't need to save and restore LIBS to prevent -ldag being
	# included if there's a found-action (arg 3).
	saved_ldflags=$LDFLAGS
	LDFLAGS="-L$dag_lib_dir"
	AC_CHECK_LIB([dag], [dag_attach_stream], [dag_version="2.5.x"], [dag_version="2.4.x"])
	LDFLAGS=$saved_ldflags

	# See if we can find a specific version string.
	AC_MSG_CHECKING([the DAG API version])
	if test -r "$dag_root/VERSION"; then
		dag_version="`cat $dag_root/VERSION`"
	fi
	AC_MSG_RESULT([$dag_version])
	AC_DEFINE(HAVE_DAG_API, 1, [define if you have the DAG API])
fi

if test $ac_cv_lbl_dag_api = no; then
	if test "$want_dag" = yes; then
        	# User wanted DAG support but we couldn't find it.
		AC_MSG_ERROR([DAG API requested, but not found at $dag_root: use --without-dag])
	fi

	if test "$V_PCAP" = dag; then
		# User requested "dag" capture type but the DAG API wasn't
		# found.
		AC_MSG_ERROR([Specifying the capture type as "dag" requires the DAG API to be present; use the --with-dag options to specify the location. (Try "./configure --help" for more information.)])
	fi
fi

AC_ARG_WITH(septel, [  --with-septel[[=DIR]]     include Septel support (located in directory DIR, if supplied).  [default=yes, on Linux, if present]],
[
	if test "$withval" = no
	then
		want_septel=no
	elif test "$withval" = yes
	then
		want_septel=yes
		septel_root=
	else
		want_septel=yes
		septel_root=$withval
	fi
],[
	#
	# Use Septel API if present, otherwise don't
	#
	want_septel=ifpresent
	septel_root=./../septel
])
ac_cv_lbl_septel_api=no
case "$V_PCAP" in
linux|septel)
	#
	# We support the Septel API if we're on Linux, or if we're building
	# a Septel-only libpcap.
	#
	;;
*)
	#
	# If the user explicitly requested Septel, tell them it's not
	# supported.
	#
	# If they expressed no preference, don't include it.
	#
	if test $want_septel = yes; then
		AC_MSG_ERROR(Septel support only available with 'linux' and 'septel' packet capture types)
	elif test $want_septel = yes; then
		want_septel=no
	fi
	;;
esac

if test "$with_septel" != no; then
	AC_MSG_CHECKING(whether we have Septel API)

	if test -z "$septel_root"; then
		septel_root=$srcdir/../septel

	fi

	septel_tools_dir="$septel_root"
	septel_include_dir="$septel_root/INC"
	DEF="-DHAVE_SEPTEL_API"

	ac_cv_lbl_septel_api=no
	if test -r "$septel_include_dir/msg.h"; then
		V_INCLS="$V_INCLS -I$septel_include_dir"
		V_DEFS="$V_DEFS $DEF"
		V_LIBS="$V_LIBS $septel_tools_dir/asciibin.o $septel_tools_dir/bit2byte.o $septel_tools_dir/confirm.o $septel_tools_dir/fmtmsg.o $septel_tools_dir/gct_unix.o $septel_tools_dir/hqueue.o $septel_tools_dir/ident.o $septel_tools_dir/mem.o $septel_tools_dir/pack.o $septel_tools_dir/parse.o $septel_tools_dir/pool.o $septel_tools_dir/sdlsig.o $septel_tools_dir/strtonum.o $septel_tools_dir/timer.o $septel_tools_dir/trace.o "

		if test "$V_PCAP" != septel ; then
			 SSRC="pcap-septel.c"

		fi
		ac_cv_lbl_septel_api=yes
	fi

	AC_MSG_RESULT($ac_cv_lbl_septel_api)
	if test $ac_cv_lbl_septel_api = no; then
		if test "$want_septel" = yes; then
			AC_MSG_ERROR(Septel API not found under directory $septel_root; use --without-septel)
		fi
	else
		AC_DEFINE(HAVE_SEPTEL_API, 1, [define if you have a Septel API])
	fi
fi

if test "$V_PCAP" = septel -a "$ac_cv_lbl_septel_api" = no; then
	AC_MSG_ERROR(Specifying the capture type as 'septel' requires the Septel API to be present; use --with-septel=DIR)
fi

AC_LBL_LEX_AND_YACC(V_LEX, V_YACC, pcap_)
if test "$V_LEX" = lex ; then
# Some versions of lex can't handle the definitions section of scanner.l .
# Try lexing it and complain if it can't deal.
	AC_CACHE_CHECK([for capable lex], tcpdump_cv_capable_lex,
		if lex -t scanner.l > /dev/null 2>&1; then
			tcpdump_cv_capable_lex=yes
		else
			tcpdump_cv_capable_lex=insufficient
		fi)
	if test $tcpdump_cv_capable_lex = insufficient ; then
		AC_MSG_ERROR([Your operating system's lex is insufficient to compile
 libpcap.  flex is a lex replacement that has many advantages, including
 being able to compile libpcap.  For more information, see
 http://www.gnu.org/software/flex/flex.html .])
	fi
fi

DYEXT="so"
case "$host_os" in

aix*)
	dnl Workaround to enable certain features
	AC_DEFINE(_SUN,1,[define on AIX to get certain functions])
	;;

hpux9*)
	AC_DEFINE(HAVE_HPUX9,1,[on HP-UX 9.x])
	;;

hpux10.0*)
	;;

hpux10.1*)
	;;

hpux*)
	dnl HPUX 10.20 and above is similar to HPUX 9, but
	dnl not the same....
	dnl
	dnl XXX - DYEXT should be set to "sl" if this is building
	dnl for 32-bit PA-RISC, but should be left as "so" for
	dnl 64-bit PA-RISC or, I suspect, IA-64.
	AC_DEFINE(HAVE_HPUX10_20,1,[on HP-UX 10.20])
	;;

sinix*)
	AC_MSG_CHECKING(if SINIX compiler defines sinix)
	AC_CACHE_VAL(ac_cv_cc_sinix_defined,
		AC_TRY_COMPILE(
		    [],
		    [int i = sinix;],
		    ac_cv_cc_sinix_defined=yes,
		    ac_cv_cc_sinix_defined=no))
	    AC_MSG_RESULT($ac_cv_cc_sinix_defined)
	    if test $ac_cv_cc_sinix_defined = no ; then
		    AC_DEFINE(sinix,1,[on sinix])
	    fi
	;;

solaris*)
	AC_DEFINE(HAVE_SOLARIS,1,[On solaris])
	;;
darwin*)
        DYEXT="dylib"
        V_CCOPT="$V_CCOPT -fno-common"
        ;;
esac

V_FADS="`grep -v $V_FINDALLDEVS FADDERS | tr '\012' ' '`"


AC_LBL_DEVEL(V_CCOPT)

AC_LBL_SOCKADDR_SA_LEN

AC_LBL_SOCKADDR_STORAGE

AC_LBL_HP_PPA_INFO_T_DL_MODULE_ID_1

AC_LBL_UNALIGNED_ACCESS

#rm -f net
#ln -s  ${srcdir}/bpf/net net

AC_SUBST(V_CCOPT)
AC_SUBST(V_DEFS)
AC_SUBST(V_INCLS)
AC_SUBST(V_LIBS)
AC_SUBST(V_LEX)
AC_SUBST(V_PCAP)
AC_SUBST(V_FINDALLDEVS)
AC_SUBST(V_RANLIB)
AC_SUBST(V_YACC)
# AC_SUBST(SSRC)
# AC_SUBST(DYEXT)

AC_SUBST(V_RING_SRC)
AC_SUBST(V_FADS)
AC_SUBST(V_OTHERS)

AC_PROG_INSTALL

AC_OUTPUT(Makefile)

if test -f .devel; then
  make depend
fi
exit 0
