
# To compile on SunOS: add "-lsocket -lnsl" to LDFLAGS
# To compile with PKCS11: add "-lpkcs11-helper" to LDFLAGS

CFLAGS	?= -O2
WARNING_CFLAGS ?= -Wall -W -Wdeclaration-after-statement
LDFLAGS ?=

LOCAL_CFLAGS = $(WARNING_CFLAGS) -I../include -D_FILE_OFFSET_BITS=64
LOCAL_LDFLAGS = -L../library 			\
		-ltime_tools$(SHARED_SUFFIX)	

ifndef SHARED
DEP=../library/libtime_tools.a
else
DEP=../library/libtime_tools.$(DLEXT)
endif

ifdef DEBUG
LOCAL_CFLAGS += -g3
endif

# if we're running on Windows, build for Windows
ifdef WINDOWS
WINDOWS_BUILD=1
endif

ifdef WINDOWS_BUILD
DLEXT=dll
EXEXT=.exe
LOCAL_LDFLAGS += -lws2_32
ifdef SHARED
SHARED_SUFFIX=.$(DLEXT)
endif
else
DLEXT=so
EXEXT=
SHARED_SUFFIX=
endif

# Zlib shared library extensions:
ifdef ZLIB
LOCAL_LDFLAGS += -lz
endif

APPS =	time_zone$(EXEXT) 
#ssl/ssl_client2$(EXEXT) ssl/mbedtls_wrapper$(EXEXT)		 

.SILENT:

.PHONY: all clean list

all: $(APPS)

$(DEP):
	$(MAKE) -C ../library


time_zone$(EXEXT): time_zone.c $(DEP)
	echo "  CC    time_zone.c"
	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) time_zone.c  $(LOCAL_LDFLAGS) $(LDFLAGS) -o $@


#ssl/ssl_client2$(EXEXT): ssl/ssl_client2.c $(DEP)
#	echo "  CC    ssl/ssl_client2.c"
#	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) ssl/ssl_client2.c  $(LOCAL_LDFLAGS) $(LDFLAGS) -o $@
#
#ssl/mbedtls_wrapper$(EXEXT): ssl/mbedtls_wrapper.c $(DEP)
#	echo "  CC    ssl/mbedtls_wrapper.c"
#	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) ssl/mbedtls_wrapper.c  $(LOCAL_LDFLAGS) $(LDFLAGS) -o $@


clean:
ifndef WINDOWS
	rm -f $(APPS)
else
	del /S /Q /F *.o *.exe
endif

list:
	echo $(APPS)
