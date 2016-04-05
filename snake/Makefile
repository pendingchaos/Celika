.PHONY: all
all: snake

CELIKA_DIR = ../celika/
include $(CELIKA_DIR)Makefile

snake: $(CELIKA_DEPS)
	@gcc snake.c $(CELIKA_LDFLAGS) -o snake -g

.PHONY: clean
clean:
	@rm -f shooty_thing

.PHONY: valgrind
valgrind: snake
	@valgrind --track-origins=yes --leak-check=full ./snake

.PHONY: run
run: snake
	@./snake

.PHONY: apitrace
apitrace: snake
	@apitrace trace ./snake
	@qaptrace snake.trace
	@rm -f snake.trace
