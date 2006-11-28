AC_INIT
AC_PREREQ(2.57)
AM_INIT_AUTOMAKE(vermont, 0.9)
AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB

AM_PATH_XML2(2.4.0,,AC_MSG_ERROR([*** LibXML version $XML_VERSION not found!]))
ACX_PTHREAD

MYSQL_FOUND=AX_LIB_MYSQL([4.1])


if test "$MYSQL_VERSION" = ""; then
	AC_MSG_ERROR([*** mysql client library not found. Please install it.])
fi


AC_MSG_WARN([We lack a check for libpcap. Please install the library, if you haven't done this already.])

AC_CONFIG_FILES([Makefile concentrator/Makefile sampler/Makefile ipfixlolib/Makefile tools/Makefile])

AC_OUTPUT
