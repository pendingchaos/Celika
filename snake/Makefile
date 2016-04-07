ifdef EMSCRIPTEN
.PHONY: all
all: index.js

CELIKA_DIR = ../celika/
include $(CELIKA_DIR)Makefile

index.js: $(CELIKA_DEPS)
	@$(CC) snake.c $(CELIKA_LDFLAGS) $(CELIKA_CFLAGS) -o index.js \
	       --memory-init-file 0 -Oz

.PHONY: clean
clean:
	@rm -f index.js
else
.PHONY: all
all: snake

CELIKA_DIR = ../celika/
include $(CELIKA_DIR)Makefile

snake: $(CELIKA_DEPS)
	@$(CC) snake.c $(CELIKA_LDFLAGS) $(CELIKA_CFLAGS) -o snake

.PHONY: clean
clean:
	@rm -f snake

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
endif
