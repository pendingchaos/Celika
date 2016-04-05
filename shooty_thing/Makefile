.PHONY: all
all: shooty_thing

CELIKA_DIR = ../celika/
include $(CELIKA_DIR)Makefile

shooty_thing: $(CELIKA_DEPS)
	gcc src/shooty_thing.c $(CELIKA_LDFLAGS) -I$(CELIKA_DIR)inc -o shooty_thing -g

.PHONY: clean
clean:
	@rm -f shooty_thing

.PHONY: valgrind
valgrind: shooty_thing
	@valgrind --track-origins=yes --leak-check=full ./shooty_thing

.PHONY: run
run: shooty_thing
	@./shooty_thing

.PHONY: apitrace
apitrace: shooty_thing
	@apitrace trace ./shooty_thing
	@qaptrace shooty_thing.trace
	@rm -f shooty_thing.trace
