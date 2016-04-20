#include "draw.h"
#include "list.h"
#include "builtinfont.h"
#include "stb_image.h"
#include "stb_image_resize.h"

#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

struct draw_tex_t {
    GLuint tex;
    aabb_t aabb;
    int flags;
    
    //Framebuffer stuff
    GLuint fb; //Zero if it is not a framebuffer texture
    bool used;
};

typedef struct effect_param_t {
    char name[64];
    bool is_tex;
    union {
        struct {
            GLuint id;
            size_t w;
            size_t h;
        } tex;
        float fval;
    };
} effect_param_t;

typedef struct draw_program_t {
    GLuint program_lin;
    GLuint program_srgb;
} draw_program_t;

struct draw_effect_t {
    draw_program_t program;
    list_t* params; //list of effect_param_t
};

typedef struct fb_pool_t {
    list_t* fbs; //list_t of draw_tex_t
} fb_pool_t;

typedef struct batch_t {
    draw_tex_t* tex;
    size_t vert_count;
    blend_t blend;
    float* pos;
    float* col;
    float* uv;
} batch_t;

static bool srgb_textures;
static bool lin_texture_read;
static bool lin_texture_write;
static bool lin_fb_write;
static draw_tex_t* cur_tex = 0;
static blend_t cur_blend = BLEND_ALPHA;
static size_t vert_count = 0;
static float* pos = NULL;
static float* col = NULL;
static float* uv = NULL;
static list_t* batches; //list of batch_t
static draw_tex_t* builtin_font_tex;
static float orientation = 0;
static float orientation_sin = 0;
static float orientation_cos = 1;
static float orientation_origin_x = 0;
static float orientation_origin_y = 0;
static float scale_x = 1;
static float scale_y = 1;
static float scale_origin_x = 0;
static float scale_origin_y = 0;
static GLuint pos_buf;
static GLuint col_buf;
static GLuint uv_buf;
static draw_program_t batch_program;
static draw_program_t batch_program_tex;
static fb_pool_t fb_pool;

static draw_tex_t create_fb(size_t width, size_t height) {
    draw_tex_t res;
    res.aabb.width = width;
    res.aabb.height = height;
    res.used = true;
    res.flags = 0;
    
    glGenTextures(1, &res.tex);
    glBindTexture(GL_TEXTURE_2D, res.tex);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    #ifdef __EMSCRIPTEN__
    glTexImage2D(GL_TEXTURE_2D, 0, srgb_textures ? GL_SRGB_ALPHA : GL_RGBA, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    #else
    glTexImage2D(GL_TEXTURE_2D, 0, srgb_textures ? GL_SRGB8_ALPHA8 : GL_RGBA8, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    #endif
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glGenFramebuffers(1, &res.fb);
    glBindFramebuffer(GL_FRAMEBUFFER, res.fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, res.tex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    return res;
}

static draw_tex_t* get_fb(size_t width, size_t height) {
    for (size_t i = 0; i < list_len(fb_pool.fbs); i++) {
        draw_tex_t* fb = list_nth(fb_pool.fbs, i);
        if (!fb->used && fb->aabb.width==width && fb->aabb.height==height) {
            fb->used = true;
            return fb;
        }
    }
    
    draw_tex_t fb = create_fb(width, height);
    list_append(fb_pool.fbs, &fb);
    return list_nth(fb_pool.fbs, list_len(fb_pool.fbs)-1);
}

static size_t get_fb_used_count() {
    size_t used_count = 0;
    for (size_t i = 0; i < list_len(fb_pool.fbs); i++)
        used_count += ((draw_tex_t*)list_nth(fb_pool.fbs, i))->used ? 1 : 0;
    return used_count;
}

static void rel_fb(draw_tex_t* fb) {
    fb->used = false;
    
    while (get_fb_used_count() < list_len(fb_pool.fbs)/2) {
        for (size_t i = 0; i < list_len(fb_pool.fbs); i++) {
            draw_tex_t* fb = list_nth(fb_pool.fbs, i);
            if (!fb->used) {
                glDeleteFramebuffers(1, &fb->fb);
                glDeleteTextures(1, &fb->tex);
                list_remove(fb);
                break;
            }
        }
    }
}

static GLuint _create_program(const char* name, const char* vsource, const char* fsource, bool output_srgb) {
    const char* vsources[] = {
#ifdef __EMSCRIPTEN__
"precision highp float;\n"
#else
"#version 100\n"
#endif
"#line 1\n", vsource};
    
    const char* fsources[] = {
#ifdef __EMSCRIPTEN__
"#version 100\n"
"precision highp float;\n"
#else
"#version 120\n"
#endif
"vec4 to_srgb(vec4 c) {\n"
"    vec3 s1 = sqrt(c.rgb);\n"
"    vec3 s2 = sqrt(s1);\n"
"    vec3 s3 = sqrt(s2);\n"
"    return vec4(0.662002687*s1 + 0.684122060*s2 - 0.323583601*s3 - 0.0225411470*c.rgb, c.a);\n"
"}\n"
"vec4 to_lin(vec4 c) {\n"
"    return vec4(c.rgb*(c.rgb*(c.rgb*0.305306011+0.682171111)+0.012522878), c.a);\n"
"}\n",
lin_texture_read ? "#define TEXTURE2D(tex, coord) texture2D(tex, coord)\n" :
                   "#define TEXTURE2D(tex, coord) to_lin(texture2D(tex, coord))\n",
lin_texture_read ? "#define TEXELFETCH(tex, tex_dim, coord) texture2D(tex, (coord)/vec2(tex_dim))\n" :
                   "#define TEXELFETCH(tex, tex_dim, coord) to_lin(texture2D(tex, (coord)/vec2(tex_dim)))\n",
"#line 1\n",
fsource,
"void main() {\n",
output_srgb ? "    gl_FragColor = to_srgb(celika_main());\n" :
              "    gl_FragColor = celika_main();\n",
"}\n"};
    
    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 2, (const GLchar**)vsources, NULL);
    glCompileShader(vert);
    char log[1024];
    glGetShaderInfoLog(vert, sizeof(log), NULL, log);
    printf("%s vertex compile info log: %s\n", name, log);
    
    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 8, (const GLchar**)fsources, NULL);
    glCompileShader(frag);
    memset(log, 0, sizeof(log));
    glGetShaderInfoLog(frag, sizeof(log), NULL, log);
    printf("%s fragment compile info log: %s\n", name, log);
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);
    glDeleteShader(vert);
    glDeleteShader(frag);
    memset(log, 0, sizeof(log));
    glGetProgramInfoLog(program, sizeof(log), NULL, log);
    printf("%s link info log: %s\n", name, log);
    
    glValidateProgram(program);
    memset(log, 0, sizeof(log));
    glGetProgramInfoLog(program, sizeof(log), NULL, log);
    printf("%s validate info log: %s\n", name, log);
    
    return program;
}

static draw_program_t create_program(const char* name, const char* vsource, const char* fsource) {
    draw_program_t prog;
    prog.program_lin = _create_program(name, vsource, fsource, false);
    prog.program_srgb = _create_program(name, vsource, fsource, true);
    return prog;
}

void draw_init() {
    fb_pool.fbs = list_create(sizeof(draw_tex_t));
    
    #ifdef __EMSCRIPTEN__
    srgb_textures = lin_texture_read = lin_texture_write = lin_fb_write = false;
    #else
    lin_fb_write = SDL_GL_ExtensionSupported("GL_EXT_framebuffer_sRGB");
    srgb_textures = lin_texture_read = lin_texture_write =
    SDL_GL_ExtensionSupported("GL_EXT_texture_sRGB");
    lin_texture_write = lin_fb_write && lin_texture_read;
    SDL_assert_release(SDL_GL_ExtensionSupported("GL_ARB_framebuffer_object"));
    if (lin_fb_write) glEnable(GL_FRAMEBUFFER_SRGB_EXT);
    #endif
    
    glGenBuffers(1, &pos_buf);
    glGenBuffers(1, &col_buf);
    glGenBuffers(1, &uv_buf);
    
    uint8_t data[BUILTIN_FONT_WIDTH*BUILTIN_FONT_HEIGHT*128*4];
    
    for (size_t i = 0; i < 128; i++) {
        for (size_t x = 0; x < BUILTIN_FONT_WIDTH; x++) {
            for (size_t y = 0; y < BUILTIN_FONT_HEIGHT; y++) {
                size_t index = y*BUILTIN_FONT_WIDTH*128 +
                               x+i*BUILTIN_FONT_WIDTH;
                bool opaque = get_builtin_font_at(i, x, BUILTIN_FONT_HEIGHT-y-1);
                data[index*4+0] = data[index*4+1] = data[index*4+2] = 255;
                data[index*4+3] = opaque ? 255 : 0;
            }
        }
    }
    
    builtin_font_tex = draw_create_tex_data(data, BUILTIN_FONT_WIDTH*128,
                                            BUILTIN_FONT_HEIGHT,
                                            TEX_MAG_NEAREST|TEX_MIN_NEAREST);
    
    batches = list_create(sizeof(batch_t));
    
    printf("OpenGL version: %s\n", glGetString(GL_VERSION));
    printf("OpenGL vendor: %s\n", glGetString(GL_VENDOR));
    printf("OpenGL renderer: %s\n", glGetString(GL_RENDERER));
    printf("GLSL version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    
    const char* vsource =
    "attribute vec2 aPos;\n"
    "attribute vec4 aCol;\n"
    "attribute vec2 aUv;\n"
    "varying vec2 vfUv;\n"
    "varying vec4 vfCol;\n"
    "void main() {\n"
    "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
    "    vfUv = aUv;\n"
    "    vfCol = aCol;\n"
    "}\n";
    
    const char* fsource =
    "varying vec4 vfCol;\n"
    "vec4 celika_main() {\n"
    "    return vfCol;\n"
    "}\n";
    
    const char* fsource_tex =
    "#extension GL_EXT_gpu_shader4 : enable\n"
    "varying vec2 vfUv;\n"
    "varying vec4 vfCol;\n"
    "uniform sampler2D uTex;\n"
    "vec4 celika_main() {\n"
    "    vec4 col = TEXTURE2D(uTex, vfUv);\n"
    "    return vfCol * col;\n"
    "}\n";
    
    batch_program = create_program("batch program", vsource, fsource);
    batch_program_tex = create_program("textured batch program", vsource, fsource_tex);
}

void draw_deinit() {
    glDeleteProgram(batch_program.program_lin);
    glDeleteProgram(batch_program.program_srgb);
    glDeleteProgram(batch_program_tex.program_lin);
    glDeleteProgram(batch_program_tex.program_srgb);
    
    for (size_t i = 0; i < list_len(batches); i++) {
        batch_t* batch = list_nth(batches, i);
        free(batch->pos);
        free(batch->col);
        free(batch->uv);
    }
    list_del(batches);
    
    free(pos);
    free(col);
    free(uv);
    
    glDeleteTextures(1, &builtin_font_tex->tex);
    free(builtin_font_tex);
    
    glDeleteBuffers(1, &uv_buf);
    glDeleteBuffers(1, &col_buf);
    glDeleteBuffers(1, &pos_buf);
    
    for (size_t i = 0; i < list_len(fb_pool.fbs); i++) {
        draw_tex_t* fb = list_nth(fb_pool.fbs, i);
        glDeleteFramebuffers(1, &fb->fb);
        glDeleteTextures(1, &fb->tex);
    }
    list_del(fb_pool.fbs);
}

draw_tex_t* draw_create_tex(const char* filename, int* w, int* h, int flags) {
    int width, height, comp;
    stbi_uc* data = stbi_load(filename, &width, &height, &comp, 4);
    if (!data) {
        fprintf(stderr, "Error loading %s: %s\n", filename, stbi_failure_reason());
        exit(1);
    }
    
    uint32_t* data32 = (uint32_t*)data;
    for (size_t y = 0; y < height/2; y++) {
        for (size_t x = 0; x < width; x++) {
            uint32_t temp = data32[y*width+x];
            data32[y*width+x] = data32[(height-y-1)*width+x];
            data32[(height-y-1)*width+x] = temp;
        }
    }
    
    draw_tex_t* res = draw_create_tex_data(data, width, height, flags);
    stbi_image_free(data);
    
    if (w) *w = width;
    if (h) *h = height;
    
    return res;
}

float srgb_to_linear(float v) {
    if (v < 0.04045) return v / 12.92;
    else return powf((v+0.055)/1.055, 2.4);
}

float linear_to_srgb(float v) {
    if (v <= 0.0031308) return v * 12.92;
    else return 1.055 * powf(v, 1/2.4) - 0.055;
}

uint8_t* downscale(uint8_t* data, size_t width, size_t height, size_t* res_width_, size_t* res_height_) {
    size_t res_width = fmax(width / 2, 1);
    size_t res_height = fmax(height / 2, 1);
    uint8_t* res = malloc(res_width*res_height*4);
    
    if (!stbir_resize_uint8_generic(data, width, height, width*4,
                                    res, res_width, res_height, res_width*4,
                                    4, 3, 0, STBIR_EDGE_CLAMP, STBIR_FILTER_DEFAULT,
                                    STBIR_COLORSPACE_SRGB, NULL)) {
        fprintf(stderr, "Failed to downscale image\n");
        exit(1);
    }
    
    *res_width_ = res_width;
    *res_height_ = res_height;
    
    return res;
}

draw_tex_t* draw_create_tex_data(uint8_t* data, size_t w, size_t h, int flags) {
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    #ifdef __EMSCRIPTEN__
    GLuint internal_format = srgb_textures ? GL_SRGB_ALPHA : GL_RGBA;
    #else
    GLuint internal_format = srgb_textures ? GL_SRGB8_ALPHA8 : GL_RGBA8;
    #endif
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    if (flags & TEX_MAG_NEAREST)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    if (flags & TEX_MIN_NEAREST)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    else
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    
    if (w && h && !(flags&TEX_MIN_NEAREST)) {
        size_t prev_w = w;
        size_t prev_h = h;
        uint8_t* prev_data = data;
        size_t level = 1;
        while (true) {
            size_t mipmap_width;
            size_t mipmap_height;
            uint8_t* mipmap = downscale(prev_data, prev_w, prev_h, &mipmap_width, &mipmap_height);
            glTexImage2D(GL_TEXTURE_2D, level, internal_format,
                         mipmap_width, mipmap_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, mipmap);
            
            level++;
            prev_w = mipmap_width;
            prev_h = mipmap_height;
            if (prev_data != data) free(prev_data);
            prev_data = mipmap;
            
            if (mipmap_width==1 && mipmap_height==1) break;
        }
        
        if (prev_data != data) free(prev_data);
    }
    
    draw_tex_t* res = malloc(sizeof(draw_tex_t));
    res->tex = tex;
    res->aabb = aabb_create_lbwh(0, 0, w, h);
    res->flags = flags;
    
    return res;
}

draw_tex_t* draw_create_tex_scaled(const char* filename, int reqw, int reqh, int* destw, int* desth, int flags) {
    int w, h;
    draw_tex_t* tex = draw_create_tex(filename, &w, &h, flags);
    
    if (reqw && reqh) {
        w = reqw;
        h = reqh;
    } else if (reqw) {
        h *= reqw/(double)w;
        w = reqw;
    } else if (reqh) {
        w *= reqh/(double)h;
        h = reqh;
    }
    
    if (destw) *destw = w;
    if (desth) *desth = h;
    
    tex->aabb.width = w;
    tex->aabb.height = h;
    
    return tex;
}

draw_tex_t* draw_create_tex_scaled_aabb(const char* filename, int reqw, int reqh, aabb_t* aabb, int flags) {
    draw_tex_t* tex = draw_create_tex_scaled(filename, reqw, reqh, NULL, NULL, flags);
    if (aabb) *aabb = tex->aabb;
    return tex;
}

void draw_del_tex(draw_tex_t* tex) {
    if (tex->fb) {
        rel_fb(tex);
    } else {
        glDeleteTextures(1, &tex->tex);
        free(tex);
    }
}

aabb_t draw_get_tex_aabb(draw_tex_t* tex) {
    return tex->aabb;
}

draw_effect_t* draw_create_effect(const char* shdr_fname) {
    FILE* file = fopen(shdr_fname, "r");
    if (!file) {
        fprintf(stderr, "Failed to open %s: %s\n", shdr_fname, strerror(errno));
        exit(1);
    }
    
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* fsource = malloc(size+1);
    fsource[size] = 0;
    fread(fsource, size, 1, file);
    
    fclose(file);
    
    static const char* vsource =
    "attribute vec2 aPos;\n"
    "varying vec2 vfUv;\n"
    "void main() {\n"
    "    gl_Position = vec4(aPos, 0, 1);\n"
    "    vfUv = aPos*0.5+0.5;\n"
    "}\n";
    
    draw_effect_t* effect = malloc(sizeof(draw_effect_t));
    effect->program = create_program(shdr_fname, vsource, fsource);
    
    free(fsource);
    
    effect->params = list_create(sizeof(effect_param_t));
    
    return effect;
}

void draw_del_effect(draw_effect_t* effect) {
    list_del(effect->params);
    glDeleteProgram(effect->program.program_lin);
    glDeleteProgram(effect->program.program_srgb);
    free(effect);
}

static void submit_batch() {
    if (vert_count) {
        batch_t batch;
        batch.tex = cur_tex;
        batch.blend = cur_blend;
        batch.vert_count = vert_count;
        batch.pos = pos;
        batch.col = col;
        batch.uv = uv;
        list_append(batches, &batch);
        
        vert_count = 0;
        pos = NULL;
        col = NULL;
        uv = NULL;
    }
}

void draw_set_tex(draw_tex_t* tex) {
    if (tex == cur_tex) return;
    submit_batch();
    cur_tex = tex;
}

draw_tex_t* draw_get_tex() {
    return cur_tex;
}

void draw_set_blend(blend_t blend) {
    if (blend == cur_blend) return;
    submit_batch();
    cur_blend = blend;
}

void draw_set_orientation(float degrees, float ox, float oy) {
    orientation = degrees / 180 * 3.14159;
    orientation_sin = sin(orientation);
    orientation_cos = cos(orientation);
    orientation_origin_x = ox;
    orientation_origin_y = oy;
}

void draw_set_scale(float x, float y, float ox, float oy) {
    scale_x = x;
    scale_y = y;
    scale_origin_x = ox;
    scale_origin_y = oy;
}

void draw_add_tri(const float* tpos, const draw_col_t* tcol, const float* tuv) {
    float new_tpos[6];
    float o1x = scale_origin_x - orientation_origin_x;
    float o1y = scale_origin_y - orientation_origin_y;
    for (size_t i=0; i<6; i+=2) {
        float x = tpos[i];
        float y = tpos[i+1];
        
        //Scale
        x = (x-scale_origin_x)*scale_x + o1x;
        y = (y-scale_origin_y)*scale_y + o1y;
        
        //Rotate
        new_tpos[i] = x*orientation_cos - y*orientation_sin + orientation_origin_x;
        new_tpos[i+1] = x*orientation_sin + y*orientation_cos + orientation_origin_y;
    }
    
    pos = realloc(pos, (vert_count+3)*8);
    col = realloc(col, (vert_count+3)*16);
    uv = realloc(uv, (vert_count+3)*8);
    
    memcpy(pos+vert_count*2, new_tpos, 24);
    
    for (size_t i = 0; i < 12; i++)
        col[vert_count*4+i] = tcol[i/4].rgba[i%4];
    
    if (cur_tex) {
        aabb_t aabb = cur_tex->aabb;
        for (size_t i = 0; i < 6; i+=2) {
            float u = tuv[i];
            float v = tuv[i+1];
            uv[vert_count*2+i] = (u*(aabb.width-1)*2+1) / (aabb.width*2);
            uv[vert_count*2+i+1] = (v*(aabb.height-1)*2+1) / (aabb.height*2);
        }
    } else {
        memcpy(uv+vert_count*2, tuv, 24);
    }
    
    vert_count += 3;
}

void draw_add_quad(const float* qpos, const draw_col_t* qcol, const float* quv) {
    float tri1_pos[] = {qpos[0*2+0], qpos[0*2+1],
                        qpos[1*2+0], qpos[1*2+1],
                        qpos[2*2+0], qpos[2*2+1]};
    draw_col_t tri1_col[] = {qcol[0], qcol[1], qcol[2]};
    float tri1_uv[] = {quv[0*2+0], quv[0*2+1],
                       quv[1*2+0], quv[1*2+1],
                       quv[2*2+0], quv[2*2+1]};
    draw_add_tri(tri1_pos, tri1_col, tri1_uv);
    
    float tri2_pos[] = {qpos[0*2+0], qpos[0*2+1],
                        qpos[3*2+0], qpos[3*2+1],
                        qpos[2*2+0], qpos[2*2+1]};
    draw_col_t tri2_col[] = {qcol[0], qcol[3], qcol[2]};
    float tri2_uv[] = {quv[0*2+0], quv[0*2+1],
                       quv[3*2+0], quv[3*2+1],
                       quv[2*2+0], quv[2*2+1]};
    draw_add_tri(tri2_pos, tri2_col, tri2_uv);
}

void draw_add_rect(const float* bl, const float* size, const draw_col_t col) {
    float qpos[] = {bl[0], bl[1], bl[0]+size[0], bl[1],
                    bl[0]+size[0], bl[1]+size[1], bl[0], bl[1]+size[1]};
    draw_col_t qcol[] = {col, col, col, col};
    float quv[] = {0, 0, 1, 0, 1, 1, 0, 1};
    draw_add_quad(qpos, qcol, quv);
}

void draw_add_aabb(aabb_t aabb, draw_col_t col) {
    float pos[] = {aabb.left, aabb.bottom};
    float size[] = {aabb.width, aabb.height};
    draw_add_rect(pos, size, col);
}

static bool framebuffer_bound = false;

static void draw_batch(batch_t batch, size_t viewport_width, size_t viewport_height) {
    if (!batch.vert_count) return;
    
    if (batch.blend == BLEND_NONE) glDisable(GL_BLEND);
    else glEnable(GL_BLEND);
    switch (batch.blend) {
    case BLEND_NONE:
        break;
    case BLEND_ALPHA:
        glBlendEquation(GL_FUNC_ADD);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        break;
    case BLEND_ADD:
        glBlendEquation(GL_FUNC_ADD);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ONE, GL_ONE);
        break;
    case BLEND_SUB:
        glBlendEquation(GL_FUNC_SUBTRACT);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ONE, GL_ONE);
        break;
    case BLEND_MULT:
        glBlendEquation(GL_FUNC_ADD);
        glBlendFuncSeparate(GL_DST_COLOR, GL_ZERO, GL_DST_ALPHA, GL_ZERO);
        break;
    }
    
    bool output_lin = framebuffer_bound ? lin_texture_write : lin_fb_write;
    GLint program = output_lin ? (batch.tex ? batch_program_tex.program_lin : batch_program.program_lin)
                               : (batch.tex ? batch_program_tex.program_srgb : batch_program.program_srgb);
    glUseProgram(program);
    
    if (batch.tex) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, batch.tex->tex);
        glUniform1i(glGetUniformLocation(program, "uTex"), 0);
    }
    
    GLint pos_loc = glGetAttribLocation(program, "aPos");
    GLint col_loc = glGetAttribLocation(program, "aCol");
    GLint uv_loc = glGetAttribLocation(program, "aUv");
    
    glEnableVertexAttribArray(pos_loc);
    glEnableVertexAttribArray(col_loc);
    if (batch.tex) glEnableVertexAttribArray(uv_loc);
    
    for (size_t i = 0; i < batch.vert_count*2; i+=2) {
        batch.pos[i] = (batch.pos[i]-viewport_width/2.0) / (viewport_width/2.0);
        batch.pos[i+1] = (batch.pos[i+1]-viewport_height/2.0) / (viewport_height/2.0);
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, pos_buf);
    glBufferData(GL_ARRAY_BUFFER, batch.vert_count*8, batch.pos, GL_STREAM_DRAW);
    glVertexAttribPointer(pos_loc, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    
    glBindBuffer(GL_ARRAY_BUFFER, col_buf);
    glBufferData(GL_ARRAY_BUFFER, batch.vert_count*16, batch.col, GL_STREAM_DRAW);
    glVertexAttribPointer(col_loc, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
    
    if (batch.tex) {
        glBindBuffer(GL_ARRAY_BUFFER, uv_buf);
        glBufferData(GL_ARRAY_BUFFER, batch.vert_count*8, batch.uv, GL_STREAM_DRAW);
        glVertexAttribPointer(uv_loc, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    }
    
    glDrawArrays(GL_TRIANGLES, 0, batch.vert_count);
    
    free(batch.pos);
    free(batch.col);
    free(batch.uv);
}

void draw_prims(size_t viewport_width, size_t viewport_height) {
    glViewport(0, 0, viewport_width, viewport_height);
    
    for (size_t i = 0; i < list_len(batches); i++)
        draw_batch(*(batch_t*)list_nth(batches, i), viewport_width, viewport_height);
    
    batch_t batch;
    batch.tex = cur_tex;
    batch.blend = cur_blend;
    batch.vert_count = vert_count;
    batch.pos = pos;
    batch.col = col;
    batch.uv = uv;
    draw_batch(batch, viewport_width, viewport_height);
    
    list_del(batches);
    batches = list_create(sizeof(batch_t));
    
    vert_count = 0;
    pos = col = uv = NULL;
}

draw_tex_t* draw_prims_fb(size_t viewport_width, size_t viewport_height) {
    draw_tex_t* res = get_fb(viewport_width, viewport_height);
    glBindFramebuffer(GL_FRAMEBUFFER, res->fb);
    framebuffer_bound = true;
    
    draw_prims(viewport_width, viewport_height);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    framebuffer_bound = false;
    
    return res;
}

static void effect_param(draw_effect_t* effect, const char* name, effect_param_t param) {
    strncpy(param.name, name, sizeof(param.name));
    
    for (size_t i = 0; i < list_len(effect->params); i++) {
        effect_param_t* other_param = list_nth(effect->params, i);
        if (!strcmp(name, other_param->name)) {
            *other_param = param;
            return;
        }
    }
    
    list_append(effect->params, &param);
}

void draw_effect_param_float(draw_effect_t* effect, const char* name, float val) {
    effect_param_t param;
    param.is_tex = false;
    param.fval = val;
    effect_param(effect, name, param);
}

void draw_effect_param_tex(draw_effect_t* effect, const char* name, draw_tex_t* val) {
    effect_param_t param;
    param.is_tex = true;
    param.tex.id = val->tex;
    param.tex.w = val->aabb.width;
    param.tex.h = val->aabb.height;
    effect_param(effect, name, param);
}

void draw_do_effect(draw_effect_t* effect, size_t viewport_width, size_t viewport_height) {
    glViewport(0, 0, viewport_width, viewport_height);
    glDisable(GL_BLEND);
    
    bool output_lin = framebuffer_bound ? lin_texture_write : lin_fb_write;
    GLuint program = output_lin ? effect->program.program_lin
                                : effect->program.program_srgb;
    glUseProgram(program);
    
    size_t next_unit = 0;
    for (size_t i = 0; i < list_len(effect->params); i++) {
        effect_param_t* param = list_nth(effect->params, i);
        GLint loc = glGetUniformLocation(program, param->name);
        if (loc < 0) {
            printf("warning: param \"%s\" not found\n", param->name);
            continue;
        }
        
        if (param->is_tex) {
            glActiveTexture(GL_TEXTURE0+next_unit);
            glBindTexture(GL_TEXTURE_2D, param->tex.id);
            glUniform1i(loc, next_unit);
            next_unit++;
            
            char name[256];
            snprintf(name, sizeof(name), "%s_dim", param->name);
            
            GLint dim_loc = glGetUniformLocation(program, name);
            if (dim_loc >= 0)
                glUniform2f(dim_loc, param->tex.w, param->tex.h);
        } else {
            glUniform1f(loc, param->fval);
        }
    }
    
    GLint pos_loc = glGetAttribLocation(program, "aPos");
    
    glBindBuffer(GL_ARRAY_BUFFER, pos_buf);
    static float pos[] = {-1, -1,
                           1, -1,
                           1,  1,
                           
                          -1, -1,
                          -1,  1,
                           1,  1};
    glBufferData(GL_ARRAY_BUFFER, sizeof(pos), pos, GL_STREAM_DRAW);
    glEnableVertexAttribArray(pos_loc);
    glVertexAttribPointer(pos_loc, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

draw_tex_t* draw_do_effect_fb(draw_effect_t* effect, size_t viewport_width, size_t viewport_height) {
    draw_tex_t* res = get_fb(viewport_width, viewport_height);
    glBindFramebuffer(GL_FRAMEBUFFER, res->fb);
    framebuffer_bound = true;
    
    draw_do_effect(effect, viewport_width, viewport_height);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    framebuffer_bound = false;
    
    return res;
}

void draw_text(const char* text, const float* pos, draw_col_t col, float height) {
    float scale = height / BUILTIN_FONT_HEIGHT;
    float width = BUILTIN_FONT_WIDTH * scale;
    
    draw_set_tex(builtin_font_tex);
    
    float cur_pos[] = {pos[0], pos[1]};
    for (const char* cur = text; *cur; cur++) {
        float qpos[] = {cur_pos[0], cur_pos[1],
                        cur_pos[0]+width, cur_pos[1],
                        cur_pos[0]+width, cur_pos[1]+height,
                        cur_pos[0], cur_pos[1]+height};
        draw_col_t qcol[] = {col, col, col, col};
        float quv[] = {*cur*BUILTIN_FONT_WIDTH, 0,
                       *cur*BUILTIN_FONT_WIDTH+BUILTIN_FONT_WIDTH, 0,
                       *cur*BUILTIN_FONT_WIDTH+BUILTIN_FONT_WIDTH, 1,
                       *cur*BUILTIN_FONT_WIDTH, 1};
        for (size_t i = 0; i < 8; i+=2)
            quv[i] /= (double)BUILTIN_FONT_WIDTH * 128;
        draw_add_quad(qpos, qcol, quv);
        
        cur_pos[0] += width;
    }
    
    draw_set_tex(0);
}

float draw_text_width(const char* text, float height) {
    return height/BUILTIN_FONT_HEIGHT * BUILTIN_FONT_WIDTH * strlen(text);
}

draw_col_t draw_create_rgb(float r, float g, float b) {
    return draw_create_rgba(r, g, b, 1);
}

draw_col_t draw_create_rgba(float r, float g, float b, float a) {
    draw_col_t res;
    res.r = r;
    res.g = g;
    res.b = b;
    res.a = a;
    return res;
}
