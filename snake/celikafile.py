celika_dir = '../celika/'
celika_cflags = '-pedantic -Wall -std=c11'
celika_ldflags = ''
celika_cflags_native = '-g'
celika_ldflags_native = '-g'
celika_cflags_emscripten = '-Oz'
celika_ldflags_emscripten = '-Oz'
celika_runopt = ''
celika_src = '$(wildcard *.c)'
celika_embedded_files = ''
celika_projname = 'snake'
