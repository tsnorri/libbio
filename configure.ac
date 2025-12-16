AC_INIT([libbio], [0.1])
AC_PREREQ([2.64])
AC_CONFIG_MACRO_DIR([configuration/m4])
m4_include([configuration/m4/ax_check_compile_flag.m4])

# Helper for the --enable options.
AC_DEFUN([libbio_enable_arg], [
	AC_ARG_ENABLE(
		[$1],
		AS_HELP_STRING([--enable-$1], [$3 (default: $2)]),
		[],
		[enable_]AS_TR_SH([$1])[=$2]
	)
	AS_IF(
		[test "x${enable_]AS_TR_SH([$1])[}" = xyes],
		[libbio_define_val=1],
		[libbio_define_val=0]
	)
	AC_DEFINE_UNQUOTED(
		[LIBBIO_ENABLE_]AS_TR_CPP(m4_toupper([$1])),
		[${libbio_define_val}],
		[$3]
	)
])

AC_CONFIG_FILES([local.mk:configuration/local.mk.in])
AC_CONFIG_HEADERS([configuration/config.h])

# Check for the required tools.
AC_PROG_CC
AC_PROG_CXX
# AC_PROG_AR is available in autoconf 2.72 and newer, so we use this workaround.
AC_CHECK_TOOL([AR], [ar], [:])
AC_PATH_PROG([RAGEL], [ragel])

libbio_enable_arg([memory-logger-support], [yes], [enable memory logger support])
libbio_enable_arg([bgzf-decompressor], [yes], [enable BGZF decompressor, requires libdeflate])

AC_LANG([C++])

# Check for the supported warning flags. (Clang++ as of version 20 does neither support nor need -Wno-error=interference-size.)
AX_CHECK_COMPILE_FLAG([-Wall], [WARNING_FLAGS="${WARNING_FLAGS} -Wall"])
AX_CHECK_COMPILE_FLAG([-Werror], [WARNING_FLAGS="${WARNING_FLAGS} -Werror"])
AX_CHECK_COMPILE_FLAG([-Wno-deprecated-declarations], [WARNING_FLAGS="${WARNING_FLAGS} -Wno-deprecated-declarations"])
AX_CHECK_COMPILE_FLAG([-Wno-unknown-pragmas], [WARNING_FLAGS="${WARNING_FLAGS} -Wno-unknown-pragmas"])
AX_CHECK_COMPILE_FLAG([-Wno-unused], [WARNING_FLAGS="${WARNING_FLAGS} -Wno-unused"])
AX_CHECK_COMPILE_FLAG([-Wno-misleading-indentation], [WARNING_FLAGS="${WARNING_FLAGS} -Wno-misleading-indentation"])
AX_CHECK_COMPILE_FLAG([-Wno-error=interference-size], [WARNING_FLAGS="${WARNING_FLAGS} -Wno-error=interference-size"])

# Check for Boost.Iostreams.
AC_CHECK_HEADERS([boost/iostreams/device/file_descriptor.hpp], [], [AC_MSG_ERROR(Boost.Iostreams is required.)])

AC_SUBST([WARNING_FLAGS])
AC_OUTPUT
