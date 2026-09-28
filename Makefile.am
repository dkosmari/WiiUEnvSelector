EXTRA_DIST = \
	bootstrap \
	COPYING.GPLv3 \
	README.md

SUBDIRS = external

external/%:
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C external


AM_CXXFLAGS = -Wall -Wextra -Werror
AM_CFLAGS = -Wall -Wextra


noinst_PROGRAMS = WiiUEnvSelector.elf


WiiUEnvSelector_elf_SOURCES = \
	external/wiiu-stdout/wiiu-stdout.c \
	external/wiiu-stdout/wiiu-stdout.h \
	src/ButtonHBox.cpp \
	src/ButtonHBox.hpp \
	src/main.cpp


WiiUEnvSelector_elf_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	$(LIBMOCHA_CPPFLAGS) \
	$(FT_CFLAGS) \
	-DIMGUI_USER_CONFIG='"$(srcdir)/external/custom_imconfig.h"' \
	-I$(srcdir)/external/cafe_glyphs/include \
	-I$(srcdir)/external/imgui \
	-I$(srcdir)/external/imgui/backends \
	-I$(srcdir)/external/imgui/misc/cpp \
	-I$(srcdir)/external/imgui/misc/freetype


WiiUEnvSelector_elf_LDADD = \
	external/libimgui.la \
	$(LIBMOCHA_LIB) \
	$(FT_LIBS) \
	$(LDADD)


WiiUEnvSelector_elf_DEPENDENCIES = $(LIBMOCHA_LIB)


WiiUEnvSelector_elf_LDFLAGS = \
	$(AM_LDFLAGS) \
	-Wl,-Map,WiiUEnvSelector.map


RPX_FILE = WiiUEnvSelector.rpx

WUHB_FILE = WiiUEnvSelector.wuhb

$(WUHB_FILE): $(RPX_FILE) Makefile
	$(WUHBTOOL) \
		$< $@ \
		--name="$(PACKAGE_STRING)" \
		--short-name="$(PACKAGE_TARNAME)" \
		--author="Daniel K. O."


all-local: $(WUHB_FILE)


.PHONY: run run-rpx

run: all
	WIILOAD=tcp:wiiu wiiload $(WUHB_FILE)

run-rpx: all
	WIILOAD=tcp:wiiu wiiload $(RPX_FILE)


.PHONY: wiiu-install wiiu-uninstall

wiiu-install: all
	curl --upload-file $(WUHB_FILE) "ftp://wiiu:/fs/vol/external01/wiiu/apps/"


wiiu-uninstall:
	curl "ftp://wiiu" --quote "DELE /fs/vol/external01/wiiu/apps/$(WUHB_FILE)"


CLEANFILES = \
	$(WUHB_FILE) \
	WiiUEnvSelector.map


@INC_AMINCLUDE@


.PHONY: company
company: compile_flags.txt

compile_flags.txt: Makefile
	printf "%s" "$(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS)" | xargs -n1 | sort -u > compile_flags.txt
	$(CPP) -xc++ /dev/null -E -Wp,-v 2>&1 | sed -n 's,^ ,-I,p' >> compile_flags.txt


