#include "font.h"
#include "list.h"
#include "str.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <math.h>

static FT_Library ft;

static const char* error_messages[] = {
    #define FT_NOERRORDEF_(name, num, msg)
    #define FT_ERRORDEF_(name, num, msg) [num] = msg,
    #include FT_ERROR_DEFINITIONS_H
    #undef FT_ERRORDEF_
    #undef FT_NOERRORDEF_
    [256] = ""
};

typedef struct glyph_t {
    uint32_t codepoint;
    draw_tex_t* tex;
    size_t tex_w, tex_h;
    float advance;
    float bearing_x, bearing_y;
} glyph_t;

typedef struct font_face_t {
    size_t height;
    FT_Face face;
    list_t* glyphs; //list of glyph_t
} font_face_t;

struct font_t {
    char filename[4096];
    list_t* faces; //list of font_face_t
};

static font_face_t* get_face(font_t* font, size_t height) {
    for (size_t i = 0; i < list_len(font->faces); i++) {
        font_face_t* cur_face = list_nth(font->faces, i);
        if (cur_face->height == height)
            return cur_face;
    }
    
    font_face_t new_face;
    new_face.height = height;
    FT_Error error = FT_New_Face(ft, font->filename, 0, &new_face.face);
    if (error) {
        fprintf(stderr, "Failed to create face for %s: %s\n",
                font->filename, error_messages[error]);
        exit(1);
    }
    
    FT_Set_Char_Size(new_face.face, 0, height*64, 96, 96); //TODO: the dpi might not be correct
    
    new_face.glyphs = list_create(sizeof(glyph_t));
    return list_append(font->faces, &new_face);
}

static glyph_t* get_glyph(font_t* font, uint32_t codepoint, size_t height) {
    font_face_t* face = get_face(font, height);
    
    for (size_t i = 0; i < list_len(face->glyphs); i++) {
        glyph_t* glyph = list_nth(face->glyphs, i);
        if (glyph->codepoint == codepoint)
            return glyph;
    }
    
    glyph_t new_glyph;
    new_glyph.codepoint = codepoint;
    if (FT_Load_Char(face->face, codepoint, FT_LOAD_RENDER))
        return NULL;
    
    FT_GlyphSlot glyph = face->face->glyph;
    
    new_glyph.advance = (glyph->advance.x / 64.0) - (int)glyph->bitmap.width;
    new_glyph.bearing_x = glyph->bitmap_left;
    new_glyph.bearing_y = glyph->bitmap_top - (int)glyph->bitmap.rows;
    
    size_t w = glyph->bitmap.width;
    size_t h = glyph->bitmap.rows;
    
    new_glyph.tex_w = w;
    new_glyph.tex_h = h;
    
    uint8_t* tex_data = malloc(w*h*4);
    for (size_t y = 0; y < new_glyph.tex_h; y++) {
        for (size_t x = 0; x < new_glyph.tex_w; x++) {
            tex_data[(y*w+x)*4] = 255;
            tex_data[(y*w+x)*4+1] = 255;
            tex_data[(y*w+x)*4+2] = 255;
            tex_data[(y*w+x)*4+3] = glyph->bitmap.buffer[(h-y-1)*glyph->bitmap.pitch+x];
        }
    }
    new_glyph.tex = draw_create_tex_data(tex_data, w, h, TEX_MAG_NEAREST|TEX_MIN_NEAREST);
    free(tex_data);
    
    return list_append(face->glyphs, &new_glyph);
}

//chars = {prev, cur, next}
static void get_kerning(font_t* font, uint32_t* codepoints, size_t height, float* kern) {
    font_face_t* face = get_face(font, height);
    
    FT_UInt left = codepoints[0] ? FT_Get_Char_Index(face->face, codepoints[0]) : 0;
    FT_UInt right = codepoints[1] ? FT_Get_Char_Index(face->face, codepoints[1]) : 0;
    
    FT_Vector kerning;
    FT_Get_Kerning(face->face, left, right, FT_KERNING_DEFAULT, &kerning);
    
    kern[0] = kerning.x / 64.0;
    kern[1] = kerning.y / 64.0;
    kern[0] += get_glyph(font, codepoints[1], height)->advance;
}

void font_init() {
    FT_Error error = FT_Init_FreeType(&ft);
    if (error) {
        fprintf(stderr, "Failed to initialize FreeType library: %s\n",
                error_messages[error]);
        exit(1);
    }
}

void font_deinit() {
    FT_Done_FreeType(ft);
}

font_t* font_create(const char* filename) {
    font_t* font = malloc(sizeof(font_t));
    strncpy(font->filename, filename, sizeof(font->filename));
    font->faces = list_create(sizeof(font_face_t));
    return font;
}

void font_del(font_t* font) {
    for (size_t i = 0; i < list_len(font->faces); i++) {
        font_face_t* face = list_nth(font->faces, i);
        
        for (size_t j = 0; j < list_len(face->glyphs); j++)
            draw_del_tex(((glyph_t*)list_nth(face->glyphs, j))->tex);
        
        list_del(face->glyphs);
        FT_Done_Face(face->face);
    }
    
    list_del(font->faces);
    free(font);
}

float font_get_advance(font_t* font, size_t height, uint32_t prev, uint32_t codepoint) {
    glyph_t* glyph = get_glyph(font, codepoint, height);
    
    float kern[2];
    uint32_t codepoints[] = {prev, codepoint, 0};
    get_kerning(font, codepoints, height, kern);
    
    return glyph->tex_w + kern[0];
}

float font_get_bearing_x(font_t* font, size_t height, uint32_t codepoint) {
    return get_glyph(font, codepoint, height)->bearing_x;
}

float font_get_width(font_t* font, size_t height, uint32_t codepoint) {
    return get_glyph(font, codepoint, height)->tex_w;
}

float font_get_kerning_x(font_t* font, size_t height, uint32_t prev, uint32_t codepoint) {
    float kern[2];
    uint32_t codepoints[] = {prev, codepoint, 0};
    get_kerning(font, codepoints, height, kern);
    return kern[0];
}

void font_draw(font_t* font, const uint32_t* text, const float* pos,
                    draw_col_t col, size_t height) {
    float cur_pos[] = {pos[0], pos[1]};
    
    draw_tex_t* last_tex = draw_get_tex();
    
    for (const uint32_t* cur = text; *cur; cur++) {
        glyph_t* glyph = get_glyph(font, *cur, height);
        if (!glyph) {
            uint32_t utf32[] = {*cur, 0};
            uint8_t* utf = utf32_to_utf8(utf32);
            printf("Warning: Unable to get glyph for codepoint '%s' (U+%4x)\n",
                   utf, *cur);
            free(utf);
            continue;
        }
        
        draw_set_tex(glyph->tex);
        
        float bl[] = {cur_pos[0]+glyph->bearing_x, cur_pos[1]+glyph->bearing_y};
        float size[] = {glyph->tex_w, glyph->tex_h};
        bl[0] = floor(bl[0]);
        bl[1] = floor(bl[1]);
        draw_add_rect(bl, size, col);
        
        float kern[2];
        uint32_t codepoints[] = {cur==text?0:cur[-1], cur[0], cur[1]};
        get_kerning(font, codepoints, height, kern);
        cur_pos[0] += kern[0];
        
        cur_pos[0] += glyph->tex_w;
    }
    
    draw_set_tex(last_tex);
}

float font_drawn_width(font_t* font, const uint32_t* text, size_t height) {
    float width = 0;
    for (const uint32_t* cur = text; *cur; cur++) {
        width += get_glyph(font, *cur, height)->tex_w;
        uint32_t codepoints[] = {cur==text ? 0 : cur[-1], cur[0], cur[1]};
        float kern[2];
        get_kerning(font, codepoints, height, kern);
        width += kern[0];
    }
    
    return width;
}
