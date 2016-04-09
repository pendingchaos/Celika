celika_dir = '../celika/'
celika_cflags = '-pedantic -Wall -std=c11'
celika_ldflags = ''
celika_cflags_native = '-g'
celika_ldflags_native = '-g'
celika_cflags_emscripten = '-Oz'
celika_ldflags_emscripten = '-Oz'
celika_runopt = ''
celika_src = '$(wildcard src/*.c)'
celika_embedded_files = 'SpaceShooterRedux/PNG/playerShip3_green.png \
SpaceShooterRedux/PNG/ufoRed.png \
SpaceShooterRedux/PNG/ufoGreen.png \
SpaceShooterRedux/PNG/ufoBlue.png \
SpaceShooterRedux/PNG/ufoYellow.png \
SpaceShooterRedux/PNG/Lasers/laserGreen10.png \
SpaceShooterRedux/PNG/Lasers/laserRed16.png \
SpaceShooterRedux/PNG/Power-ups/pill_yellow.png \
SpaceShooterRedux/PNG/Power-ups/bolt_gold.png \
SpaceShooterRedux/Backgrounds/black.png \
SpaceShooterRedux/Backgrounds/blue.png \
SpaceShooterRedux/Backgrounds/darkPurple.png \
SpaceShooterRedux/Backgrounds/purple.png \
shaders/passthough.glsl \
SpaceShooterRedux/Bonus/sfx_laser1.ogg \
SpaceShooterRedux/Bonus/sfx_laser2.ogg \
SpaceShooterRedux/Bonus/sfx_lose.ogg \
SpaceShooterRedux/Bonus/sfx_shieldUp.ogg \
SpaceShooterRedux/Bonus/sfx_shieldDown.ogg \
SpaceShooterRedux/Bonus/sfx_twoTone.ogg \
SpaceShooterRedux/Bonus/sfx_zap.ogg \
SpaceShooterRedux/PNG/Power-ups/shield_bronze.png \
SpaceShooterRedux/PNG/Power-ups/shield_silver.png \
SpaceShooterRedux/PNG/Power-ups/shield_gold.png \
SpaceShooterRedux/PNG/Effects/shield1.png \
SpaceShooterRedux/PNG/UI/buttonGreen.png \
SpaceShooterRedux/Bonus/kenvector_future_thin.ttf'
celika_projname = 'shooty-thing' 
