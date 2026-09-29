EXTRA_DIST = \
	assets \
	bootstrap \
	COPYING.GPLv3 \
	README.md

SUBDIRS = external

external/%:
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C external


AM_CXXFLAGS = -Wall -Wextra -Werror
AM_CFLAGS = -Wall -Wextra


APP_NAME := WiiUEnvSelector


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


WiiUEnvSelector_elf_DEPENDENCIES = \
	$(LIBMOCHA_LIB) \
	external/libimgui.la


WiiUEnvSelector_elf_LDFLAGS = \
	$(AM_LDFLAGS) \
	-Wl,-Map,$(APP_NAME).map


RPX_FILE = $(APP_NAME).rpx

WUHB_FILE = $(APP_NAME).wuhb


$(WUHB_FILE): 	$(RPX_FILE) \
		Makefile \
		assets/icon.png \
		assets/splash-tv.png \
		assets/splash-drc.png
	$(WUHBTOOL) \
		$< $@ \
		--name="$(PACKAGE_STRING)" \
		--short-name="$(PACKAGE_TARNAME)" \
		--author="Daniel K. O." \
		--icon=assets/icon.png \
		--tv-image=assets/splash-tv.png \
		--drc-image=assets/splash-drc.png


BUILD_TIMESTAMP := $(shell \
if [ -n "$$SOURCE_DATE_EPOCH" ]; then \
    date -u -d "@$$SOURCE_DATE_EPOCH" +'%Y%m%d%H%M%S' 2>/dev/null || date -u -r "$$SOURCE_DATE_EPOCH" +'%Y%m%d%H%M%S'; \
else \
    date -u +'%Y%m%d%H%M%S'; \
fi \
)

$(info BUILD_TIMESTAMP=$(BUILD_TIMESTAMP))


HBL_ICON_FILE := assets/hbl/icon.png

HBL_META_XML_FILE := meta.xml

$(HBL_META_XML_FILE): $(HBL_META_XML_FILE).tmp $(RPX_FILE)
	sed 's/@BUILD_TIMESTAMP@/$(BUILD_TIMESTAMP)/g' $< > $@


APP_ZIP_FILE := $(APP_NAME).zip


STAGING_DIR := wiiu/apps/$(APP_NAME)

$(APP_ZIP_FILE):	$(WUHB_FILE) \
			$(RPX_FILE) \
			$(HBL_ICON_FILE) \
			$(HBL_META_XML_FILE)
	$(RM) -r wiiu
	$(RM) $@
	$(MKDIR_P) $(STAGING_DIR)
	cp --target-directory=$(STAGING_DIR) $^
	zip -r $@ wiiu
	$(RM) -r wiiu


all-local: $(WUHB_FILE) $(APP_ZIP_FILE)


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


BUILT_SOURCES = $(HBL_META_XML_FILE)


CLEANFILES = \
	$(HBL_META_XML_FILE) \
	$(HBL_META_XML_FILE).tmp \
	$(WUHB_FILE) \
	$(APP_ZIP_FILE) \
	WiiUEnvSelector.map


@INC_AMINCLUDE@


.PHONY: company
company: compile_flags.txt

compile_flags.txt: Makefile
	printf "%s" "$(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS)" | xargs -n1 | sort -u > compile_flags.txt
	$(CPP) -xc++ /dev/null -E -Wp,-v 2>&1 | sed -n 's,^ ,-I,p' >> compile_flags.txt


