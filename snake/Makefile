.PHONY: all
ifdef EMSCRIPTEN
all: index.js
else
all: snake
endif

CELIKA_DIR = ../celika/
include $(CELIKA_DIR)Makefile

ifdef EMSCRIPTEN
index.js: $(CELIKA_DEPS)
	@$(CC) snake.c $(CELIKA_LDFLAGS) -I$(CELIKA_DIR)inc -o index.js \
	       --memory-init-file 0 -Oz
else
snake: $(CELIKA_DEPS)
	@$(CC) snake.c $(CELIKA_LDFLAGS) -I$(CELIKA_DIR)inc -o snake
endif

.PHONY: clean
clean:
ifdef EMSCRIPTEN
	@rm -f index.js
else
	@rm -f snake
endif

.PHONY: valgrind
valgrind: snake
	@valgrind --track-origins=yes --leak-check=full ./snake

.PHONY: run
run: snake
	@./snake

.PHONY: apitrace
apitrace: snake
	@apitrace trace ./snake
	@qapitrace snake.trace
	@rm -f snake.trace
