.PHONY: all
ifdef EMSCRIPTEN
all: shooty_thing.html
else
all: shooty_thing
endif

CELIKA_DIR = ../celika/
include $(CELIKA_DIR)Makefile

shooty_thing: $(CELIKA_DEPS)
	@$(CC) shooty_thing.c $(CELIKA_LDFLAGS) -I$(CELIKA_DIR)inc -o shooty_thing -Os

ifdef EMSCRIPTEN
shooty_thing.html: $(CELIKA_DEPS)
	@$(CC) shooty_thing.c $(CELIKA_LDFLAGS) -I$(CELIKA_DIR)inc -o shooty_thing.html -Oz \
	       --embed-file SpaceShooterRedux/PNG/playerShip3_green.png \
	       --embed-file SpaceShooterRedux/PNG/ufoRed.png \
	       --embed-file SpaceShooterRedux/PNG/Lasers/laserGreen10.png \
	       --embed-file SpaceShooterRedux/PNG/Lasers/laserRed16.png \
	       --embed-file SpaceShooterRedux/PNG/Power-ups/pill_yellow.png \
	       --embed-file SpaceShooterRedux/PNG/Power-ups/bolt_gold.png \
	       --embed-file SpaceShooterRedux/Backgrounds/black.png \
	       --embed-file SpaceShooterRedux/Backgrounds/blue.png \
	       --embed-file SpaceShooterRedux/Backgrounds/darkPurple.png \
	       --embed-file SpaceShooterRedux/Backgrounds/purple.png \
	       --embed-file shaders/passthough.glsl \
	       --embed-file SpaceShooterRedux/Bonus/sfx_laser1.ogg \
	       --embed-file SpaceShooterRedux/Bonus/sfx_laser2.ogg \
	       --embed-file SpaceShooterRedux/Bonus/sfx_lose.ogg \
	       --memory-init-file 0
endif

.PHONY: clean
clean:
ifdef EMSCRIPTEN
	@rm -f shooty_thing.html
	@rm -f shooty_thing.js
else
	@rm -f shooty_thing
endif

.PHONY: valgrind
valgrind: shooty_thing
	@valgrind --track-origins=yes --leak-check=full ./shooty_thing

.PHONY: run
run: shooty_thing
	@./shooty_thing

.PHONY: apitrace
apitrace: shooty_thing
	@apitrace trace ./shooty_thing
	@qapitrace shooty_thing.trace
	@rm -f shooty_thing.trace
