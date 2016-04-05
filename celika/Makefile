CELIKA_CFLAGS = -pedantic -Wall -std=c11 -g `sdl2-config --cflags` `pkg-config vorbisfile --cflags` -DGL_GLEXT_PROTOTYPES -I$(CELIKA_DIR)inc/celika
CELIKA_LDFLAGS = `sdl2-config --libs` `pkg-config vorbisfile --libs` -lGL -lm $(CELIKA_DIR)libcelika.a
CELIKA_DEPS = libcelika.a

CELIKA_TARGET = $@
CELIKA_PREREQ1 = $<

celika_src = $(wildcard $(CELIKA_DIR)src/*.c)
celika_obj = $(join $(dir $(celika_src)), \
                    $(addprefix ., $(notdir $(celika_src:.c=.celika.o))))
celika_dep = $(celika_obj:.o=.d)

libcelika.a: $(celika_obj)
	@ar rcs $(CELIKA_DIR)libcelika.a $(celika_obj)

-include $(celika_dep)

.%.celika.d: %.c
	@# -M:  generate Makefile rule
	@# -MT: specify the rule's target
	@$(CPP) $(CELIKA_CFLAGS) $(CELIKA_PREREQ1) -M -MT $(@:.d=.o) >$(CELIKA_TARGET)

.%.celika.o: %.c Makefile
	@$(CC) -c $(CELIKA_CFLAGS) $(CELIKA_PREREQ1) -o $(CELIKA_TARGET)

.PHONY: clean
celika_clean:
	@rm -f $(celika_dep) $(celika_obj) $(CELIKA_DIR)libcelika.a
