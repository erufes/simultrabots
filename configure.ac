AC_PREREQ(2.69)
AC_INIT([erus_sim_iron_2020], 1.0) 
AC_CONFIG_SRCDIR(src/main.cpp)
AM_INIT_AUTOMAKE
AC_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIRS([m4])

#check for programs
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL 

# Checks for libraries.
AC_CHECK_LIB(pthread, pthread_create)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h stdlib.h string.h strings.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_SELECT_ARGTYPES
AC_CHECK_FUNCS([floor gethostbyname gettimeofday inet_ntoa pow rint select socket sqrt strcasecmp strdup strstr])


AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
