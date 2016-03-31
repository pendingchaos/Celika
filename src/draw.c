#include "draw.h"

#include <SDL2/SDL_opengl.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static size_t vert_count = 0;
static float* pos = NULL;
static float* col = NULL;
static size_t width = 0;
static size_t height = 0;

void draw_begin(size_t w, size_t h) {
    glViewport(0, 0, w, h);
    width = w;
    height = h;
}

void draw_clear(float* col) {
    glClearColor(col[0], col[1], col[2], 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
}

void draw_add_tri(float* tpos, float* tcol) {
    pos = realloc(pos, (vert_count+3)*8);
    col = realloc(col, (vert_count+3)*12);
    memcpy(pos+vert_count*2, tpos, 24);
    memcpy(col+vert_count*3, tcol, 36);
    vert_count += 3;
}

void draw_add_quad(float* qpos, float* qcol) {
    float tri1_pos[] = {qpos[0*2+0], qpos[0*2+1],
                        qpos[1*2+0], qpos[1*2+1],
                        qpos[2*2+0], qpos[2*2+1]};
    float tri1_col[] = {qcol[0*3+0], qcol[0*3+1], qcol[0*3+2],
                        qcol[1*3+0], qcol[1*3+1], qcol[1*3+2],
                        qcol[2*3+0], qcol[2*3+1], qcol[2*3+2]};
    draw_add_tri(tri1_pos, tri1_col);
    
    float tri2_pos[] = {qpos[0*2+0], qpos[0*2+1],
                        qpos[3*2+0], qpos[3*2+1],
                        qpos[2*2+0], qpos[2*2+1]};
    float tri2_col[] = {qcol[0*3+0], qcol[0*3+1], qcol[0*3+2],
                        qcol[3*3+0], qcol[3*3+1], qcol[3*3+2],
                        qcol[2*3+0], qcol[2*3+1], qcol[2*3+2]};
    draw_add_tri(tri2_pos, tri2_col);
}

void draw_add_rect(float* bl, float* size, float* col) {
    float qpos[] = {bl[0], bl[1], bl[0]+size[0], bl[1],
                    bl[0]+size[0], bl[1]+size[1], bl[0], bl[1]+size[1]};
    float qcol[] = {col[0], col[1], col[2], col[0], col[1], col[2],
                    col[0], col[1], col[2], col[0], col[1], col[2]};
    draw_add_quad(qpos, qcol);
}

void draw_prims() {
    glViewport(0, 0, width, height);
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, pos);
    glColorPointer(3, GL_FLOAT, 0, col);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(-1, -1, 0);
    glScalef(1.0/width*2, 1.0/height*2, 1);
    
    glDrawArrays(GL_TRIANGLES, 0, vert_count);
    
    glPopMatrix();
    
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    
    vert_count = 0;
    
    free(pos);
    free(col);
    pos = col = NULL;
}
