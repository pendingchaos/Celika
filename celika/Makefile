ifdef EMSCRIPTEN
CELIKA_TARGET = release
else
CELIKA_TARGET ?= debug
endif
CELIKA_CFLAGS = -I$(CELIKA_DIR)inc -DCELIKA_TARGET=$(CELIKA_TARGET)
CELIKA_INTERNAL_CFLAGS = -pedantic -Wall -std=c11 `pkg-config vorbisfile --cflags` \
                         -DGL_GLEXT_PROTOTYPES -I$(CELIKA_DIR)inc/celika \
			 -DCELIKA_TARGET=$(CELIKA_TARGET) `freetype-config --cflags`
ifdef EMSCRIPTEN
CELIKA_LDFLAGS = -lGL -lm $(CELIKA_DIR)libcelika.emscripten.$(CELIKA_TARGET).a \
                 $(EMSCRIPTEN)/system/local/lib/libvorbisfile.a \
                 $(EMSCRIPTEN)/system/local/lib/libvorbis.a \
                 $(EMSCRIPTEN)/system/local/lib/libogg.a \
                 $(EMSCRIPTEN)/system/local/lib/libSDL2.a \
		 `freetype-config --libs` `libpng-config --libs` \
		 `pkg-config zlib --libs`
CELIKA_REL_CFLAGS = -Oz
else
CELIKA_LDFLAGS = -lGL -lm $(CELIKA_DIR)libcelika.$(CELIKA_TARGET).a \
                 `pkg-config vorbisfile --libs` `sdl2-config --libs` \
		 `freetype-config --libs`
CELIKA_REL_CFLAGS = -Os
endif
CELIKA_DEPS = libcelika$(CELIKA_EM).$(CELIKA_TARGET).a
CELIKA_DBG_CFLAGS = -g

ifdef EMSCRIPTEN
CELIKA_EM = .emscripten
else
CELIKA_EM =
endif

celika_src = $(wildcard $(CELIKA_DIR)src/*.c)
celika_obj = $(join $(dir $(celika_src)), \
                    $(addprefix ., $(notdir $(celika_src:.c=.celika$(CELIKA_EM).$(CELIKA_TARGET).o))))
celika_dep = $(celika_obj:.o=.d)

libcelika$(CELIKA_EM).$(CELIKA_TARGET).a: $(celika_obj)
	@$(AR) rcs $(CELIKA_DIR)libcelika$(CELIKA_EM).$(CELIKA_TARGET).a $(celika_obj)

-include $(celika_dep)

.%.celika$(CELIKA_EM).$(CELIKA_TARGET).d: %.c
	@# -M:  generate Makefile rule
	@# -MT: specify the rule's target
	@$(CPP) $(CELIKA_INTERNAL_CFLAGS) $< -M -MT $(@:.d=.o) >$@

.%.celika$(CELIKA_EM).debug.o: %.c Makefile
	@$(CC) -c $(CELIKA_INTERNAL_CFLAGS) $< -o $@ $(CELIKA_DBG_CFLAGS)

.%.celika$(CELIKA_EM).release.o: %.c Makefile
	@$(CC) -c $(CELIKA_INTERNAL_CFLAGS) $< -o $@ $(CELIKA_REL_CFLAGS)

.PHONY: clean
celika_clean:
	@rm -f $(celika_dep) $(celika_obj) $(CELIKA_DIR)libcelika.$(CELIKA_TARGET)$(CELIKA_EM).a
