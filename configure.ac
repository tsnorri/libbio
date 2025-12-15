AC_INIT([libbio], [0.1])
AC_PREREQ([2.64])
AC_CONFIG_MACRO_DIR([m4])
m4_include([m4/ax_check_compile_flag.m4])

AC_CONFIG_FILES([local.mk])

# Check for the required tools.
AC_PROG_CC
AC_PROG_CXX
# AC_PROG_AR is available in autoconf 2.72 and newer, so we use this workaround.
AC_CHECK_TOOL([AR], [ar], [:])
AC_PATH_PROG([RAGEL], [ragel])

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
