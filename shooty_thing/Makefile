ifdef EMSCRIPTEN
.PHONY: all
all: index.js

CELIKA_DIR = ../celika/
include $(CELIKA_DIR)Makefile

index.js: $(CELIKA_DEPS)
	@$(CC) shooty_thing.c $(CELIKA_LDFLAGS) $(CELIKA_CFLAGS) -o index.js -Oz \
	       --embed-file SpaceShooterRedux/PNG/playerShip3_green.png \
	       --embed-file SpaceShooterRedux/PNG/ufoRed.png \
	       --embed-file SpaceShooterRedux/PNG/ufoGreen.png \
	       --embed-file SpaceShooterRedux/PNG/ufoBlue.png \
	       --embed-file SpaceShooterRedux/PNG/ufoYellow.png \
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
	       --embed-file SpaceShooterRedux/Bonus/sfx_shieldUp.ogg \
	       --embed-file SpaceShooterRedux/Bonus/sfx_shieldDown.ogg \
	       --embed-file SpaceShooterRedux/Bonus/sfx_twoTone.ogg \
	       --embed-file SpaceShooterRedux/Bonus/sfx_zap.ogg \
           --embed-file SpaceShooterRedux/PNG/Power-ups/shield_bronze.png \
           --embed-file SpaceShooterRedux/PNG/Power-ups/shield_silver.png \
           --embed-file SpaceShooterRedux/PNG/Power-ups/shield_gold.png \
           --embed-file SpaceShooterRedux/PNG/Effects/shield1.png \
	       --memory-init-file 0

.PHONY: clean
clean:
	@rm -f index.js
else
.PHONY: all
all: shooty_thing

CELIKA_DIR = ../celika/
include $(CELIKA_DIR)Makefile

shooty_thing: $(CELIKA_DEPS)
	@$(CC) shooty_thing.c $(CELIKA_LDFLAGS) $(CELIKA_CFLAGS) -o shooty_thing -g

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
	@qapitrace shooty_thing.trace
	@rm -f shooty_thing.trace
endif
