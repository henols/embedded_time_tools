
CFLAGS	?= -O2
WARNING_CFLAGS ?=  -Wall -W -Wdeclaration-after-statement
LDFLAGS ?=

LOCAL_CFLAGS = $(WARNING_CFLAGS) -I../include -D_FILE_OFFSET_BITS=64
LOCAL_LDFLAGS =

ifdef DEBUG
LOCAL_CFLAGS += -g3
endif

# MicroBlaze specific options:
# CFLAGS += -mno-xl-soft-mul -mxl-barrel-shift

# To compile on Plan9:
# CFLAGS += -D_BSD_EXTENSION

# if were running on Windows build for Windows
ifdef WINDOWS
WINDOWS_BUILD=1
endif

# To compile as a shared library:
ifdef SHARED
# all code is position-indep with mingw, avoid warning about useless flag
ifndef WINDOWS_BUILD
LOCAL_CFLAGS += -fPIC -fpic
endif
endif

SOEXT_TLS=so.10
SOEXT_X509=so.0
SOEXT_CRYPTO=so.0

DLEXT=so
# OSX shared library extension:
# DLEXT=dylib

# Windows shared library extension:
ifdef WINDOWS_BUILD
DLEXT=dll
endif

OBJS_TLS=	asctime.o 	cron.o		\
			difftime.o	localtime.o		\
			scheduler.o	strftime.o		\
			tzdata.o

.SILENT:

.PHONY: all static shared clean

ifndef SHARED
all: static
else
all: shared static
endif

static: libtime_tools.a

shared:  libtime_tools.$(DLEXT)

# tls
libtime_tools.a: $(OBJS_TLS)
	echo "  AR    $@"
	$(AR) -rc $@ $(OBJS_TLS)
	echo "  RL    $@"
	$(AR) -s $@

libtime_tools.$(SOEXT_TLS): $(OBJS_TLS) 
	echo "  LD    $@"
	$(CC) -shared -Wl,-soname,$@ -L. $(LOCAL_LDFLAGS) $(LDFLAGS) -o $@ $(OBJS_TLS)

libtime_tools.so: libtime_tools.$(SOEXT_TLS)
	echo "  LN    $@ -> $<"
	ln -sf $< $@

libtime_tools.dylib: $(OBJS_TLS)
	echo "  LD    $@"
	$(CC) -dynamiclib $(LOCAL_LDFLAGS) $(LDFLAGS) -o $@ $(OBJS_TLS)

libtime_tools.dll: $(OBJS_TLS) 
	echo "  LD    $@"
	$(CC) -shared -Wl,-soname,$@ -Wl,--out-implib,$@.a -o $@ $(OBJS_TLS) -lws2_32 -lwinmm -lgdi32 -L. -static-libgcc $(LOCAL_LDFLAGS) $(LDFLAGS)


.c.o:
	echo "  CC    $<"
	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) -c $<

clean:
ifndef WINDOWS
	rm -f *.o libtime_tools*
else
	del /Q /F *.o libtime_tools*
endif
