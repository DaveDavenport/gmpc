if PLATFORM_WIN32
platform_sources = eggsmclient-win32.c
platform_logout_test_ldflags = -mwindows
else
if PLATFORM_OSX
platform_defines = -xobjective-c
platform_ldflags = -framework Carbon
platform_session_end_ldflags = -framework Carbon
platform_logout_test_ldflags = -framework Carbon
platform_sources = eggsmclient-osx.c
else
platform_defines = -DEGG_SM_CLIENT_BACKEND_XSMP
platform_ltlibraries =                   \
	libeggdesktopfile.la             
platform_libs = libeggdesktopfile.la -lSM -lICE
platform_sources = eggsmclient-xsmp.c
platform_programs = egg-launch
endif
endif

INCLUDES =                               \
	-DG_LOG_DOMAIN=\""EggSMClient"\" \
	$(platform_defines)              \
	$(glib_CFLAGS)           \
    $(gtk_CFLAGS)

noinst_LTLIBRARIES =                     \
	libeggsmclient.la                \
	$(platform_ltlibraries)

libeggsmclient_la_LIBADD =               \
	$(glib_LIBS)             \
	$(gtk_LIBS)             \
	$(platform_libs)

libeggsmclient_la_LDFLAGS =              \
	$(platform_ldflags)

libeggsmclient_la_SOURCES =              \
	eggsmclient.c                    \
	eggsmclient.h                    \
	eggsmclient-private.h            \
	$(platform_sources)

libeggdesktopfile_la_LIBADD =            \
	$(EGG_LIBS)

libeggdesktopfile_la_SOURCES =           \
	eggdesktopfile.c                 \
	eggdesktopfile.h


egg_session_end_LDADD =                  \
	libeggsmclient.la

egg_session_end_LDFLAGS =                \
	$(platform_session_end_ldflags)

logout_test_SOURCES =                    \
	logout-test.c

logout_test_LDADD =                      \
	libeggsmclient.la

logout_test_LDFLAGS =                    \
	$(platform_logout_test_ldflags)

egg_launch_SOURCES =                     \
	egg-launch.c

egg_launch_LDADD =                       \
	libeggdesktopfile.la

EXTRA_DIST =                             \
	eggsmclient-osx.c                \
	eggsmclient-win32.c              \
	eggsmclient-xsmp.c
