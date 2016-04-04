#include "draw.h"
#include "builtinfont.h"
#include "stb_image.h"

#include <SDL2/SDL_opengl.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

struct draw_tex_t {
    GLuint id;
    aabb_t aabb;
};

static draw_tex_t* cur_tex = 0;
static size_t vert_count = 0;
static float* pos = NULL;
static float* col = NULL;
static float* uv = NULL;
static size_t width = 0;
static size_t height = 0;
static draw_tex_t* builtin_font_tex;
static float orientation = 0;
static float orientation_origin_x = 0;
static float orientation_origin_y = 0;
static float scale_x = 1;
static float scale_y = 1;
static float scale_origin_x = 0;
static float scale_origin_y = 0;

void draw_init() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    GLuint font_tex_id;
    glGenTextures(1, &font_tex_id);
    glBindTexture(GL_TEXTURE_2D, font_tex_id);
    
    uint8_t data[BUILTIN_FONT_WIDTH*BUILTIN_FONT_HEIGHT*128];
    
    for (size_t i = 0; i < 128; i++) {
        for (size_t x = 0; x < BUILTIN_FONT_WIDTH; x++) {
            for (size_t y = 0; y < BUILTIN_FONT_HEIGHT; y++) {
                size_t index = y*BUILTIN_FONT_WIDTH*128 +
                               x+i*BUILTIN_FONT_WIDTH;
                bool opaque = get_builtin_font_at(i, x, BUILTIN_FONT_HEIGHT-y-1);
                data[index] = opaque ? 255 : 0;
            }
        }
    }
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA8, BUILTIN_FONT_WIDTH*128,
                 BUILTIN_FONT_HEIGHT, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    builtin_font_tex = malloc(sizeof(draw_tex_t));
    builtin_font_tex->id = font_tex_id;
    builtin_font_tex->aabb = create_aabb_lbwh(0, 0, BUILTIN_FONT_WIDTH*128,
                                              BUILTIN_FONT_HEIGHT);
}

void draw_deinit() {
    glDeleteTextures(1, &builtin_font_tex->id);
    free(builtin_font_tex);
}

draw_tex_t* draw_create_tex(const char* filename, int* w, int* h) {
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
    
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    stbi_image_free(data);
    
    if (w || h) {
        if (w)
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, w);
        if (h)
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, h);
    }
    
    if (cur_tex) glBindTexture(GL_TEXTURE_2D, cur_tex->id);
    else glBindTexture(GL_TEXTURE_2D, 0);
    
    draw_tex_t* res = malloc(sizeof(draw_tex_t));
    res->id = tex;
    res->aabb = create_aabb_lbwh(0, 0, width, height);
    
    return res;
}

draw_tex_t* draw_create_scaled_tex(const char* filename, int reqw, int reqh, int* destw, int* desth) {
    int w, h;
    draw_tex_t* tex = draw_create_tex(filename, &w, &h);
    
    if (reqw && reqh) {
        w = reqw;
        h = reqh;
    } else if (reqw) {
        w = reqw;
        h *= reqw/(double)w;
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

draw_tex_t* draw_create_scaled_tex_aabb(const char* filename, int reqw, int reqh, aabb_t* aabb) {
    draw_tex_t* tex = draw_create_scaled_tex(filename, reqw, reqh, NULL, NULL);
    if (aabb) *aabb = tex->aabb;
    return tex;
}

void draw_del_tex(draw_tex_t* tex) {
    glDeleteTextures(1, &tex->id);
    free(tex);
}

aabb_t draw_get_tex_aabb(draw_tex_t* tex) {
    return tex->aabb;
}

void draw_set_tex(draw_tex_t* tex) {
    if (tex == cur_tex) return;
    if (vert_count) draw_prims();
    if (tex) glBindTexture(GL_TEXTURE_2D, tex->id);
    else glBindTexture(GL_TEXTURE_2D, 0);
    cur_tex = tex;
}

void draw_begin(size_t w, size_t h) {
    glViewport(0, 0, w, h);
    width = w;
    height = h;
}

void draw_end() {
    draw_prims();
}

void draw_clear(draw_col_t col) {
    glClearColor(col.r, col.g, col.b, col.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void draw_set_orientation(float degrees, float ox, float oy) {
    orientation = degrees / 180 * 3.14159;
    orientation_origin_x = ox;
    orientation_origin_y = oy;
}

void draw_set_scale(float x, float y, float ox, float oy) {
    scale_x = x;
    scale_y = y;
    scale_origin_x = ox;
    scale_origin_y = oy;
}

void draw_add_tri(float* tpos, draw_col_t* tcol, float* tuv) {
    float new_tpos[6];
    for (size_t i=0; i<6; i+=2) {
        float x = tpos[i];
        float y = tpos[i+1];
        
        //Scale
        x = (x-scale_origin_x)*scale_x + scale_origin_x;
        y = (y-scale_origin_y)*scale_y + scale_origin_y;
        
        //Rotate
        x -= orientation_origin_x;
        y -= orientation_origin_y;
        new_tpos[i] = x*cos(orientation) - y*sin(orientation);
        new_tpos[i+1] = x*sin(orientation) + y*cos(orientation);
        new_tpos[i] += orientation_origin_x;
        new_tpos[i+1] += orientation_origin_y;
    }
    
    pos = realloc(pos, (vert_count+3)*8);
    col = realloc(col, (vert_count+3)*16);
    uv = realloc(uv, (vert_count+3)*8);
    memcpy(pos+vert_count*2, new_tpos, 24);
    for (size_t i = 0; i < 12; i++)
        col[vert_count*4+i] = tcol[i/4].rgba[i%4];
    memcpy(uv+vert_count*2, tuv, 24);
    vert_count += 3;
}

void draw_add_quad(float* qpos, draw_col_t* qcol, float* quv) {
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

void draw_add_rect(float* bl, float* size, draw_col_t col) {
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

void draw_prims() {
    if (!vert_count) return;
    
    glViewport(0, 0, width, height);
    
    if (cur_tex) glEnable(GL_TEXTURE_2D);
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, pos);
    glColorPointer(4, GL_FLOAT, 0, col);
    glTexCoordPointer(2, GL_FLOAT, 0, uv);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(-1, -1, 0);
    glScalef(1.0/width*2, 1.0/height*2, 1);
    
    glDrawArrays(GL_TRIANGLES, 0, vert_count);
    
    glPopMatrix();
    
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    
    if (cur_tex) glDisable(GL_TEXTURE_2D);
    
    vert_count = 0;
    
    free(pos);
    free(col);
    free(uv);
    pos = col = uv = NULL;
}

void draw_text(const char* text, float* pos, draw_col_t col, float height) {
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

draw_col_t draw_rgb(float r, float g, float b) {
    return draw_rgba(r, g, b, 1);
}

draw_col_t draw_rgba(float r, float g, float b, float a) {
    draw_col_t res;
    res.r = r;
    res.g = g;
    res.b = b;
    res.a = a;
    return res;
}
