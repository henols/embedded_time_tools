
DESTDIR=/usr/local
PREFIX=time_tools_

.SILENT:

.PHONY: all programs lib install uninstall clean 

# all: programs 
all: programs

no_test: programs

programs: lib
	$(MAKE) -C programs

lib:
	$(MAKE) -C library


ifndef WINDOWS
install: no_test
	mkdir -p $(DESTDIR)/include/time_tools
	cp -r include/time_tools $(DESTDIR)/include
	
	mkdir -p $(DESTDIR)/lib
	cp -RP library/libtime_tools.*    $(DESTDIR)/lib
	
	mkdir -p $(DESTDIR)/bin
	for p in programs/*/* ; do              \
	    if [ -x $$p ] && [ ! -d $$p ] ;     \
	    then                                \
	        f=$(PREFIX)`basename $$p` ;     \
	        cp $$p $(DESTDIR)/bin/$$f ;     \
	    fi                                  \
	done

uninstall:
	rm -rf $(DESTDIR)/include/time_tools
	rm -f $(DESTDIR)/lib/libtime_tools.*
	
	for p in programs/*/* ; do              \
	    if [ -x $$p ] && [ ! -d $$p ] ;     \
	    then                                \
	        f=$(PREFIX)`basename $$p` ;     \
	        rm -f $(DESTDIR)/bin/$$f ;      \
	    fi                                  \
	done
endif


clean:
	$(MAKE) -C library clean
	$(MAKE) -C programs clean
ifndef WINDOWS
	find . \( -name \*.gcno -o -name \*.gcda -o -name \*.info \) -exec rm {} +
endif


