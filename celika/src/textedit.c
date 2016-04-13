#include "textedit.h"

#include "aabb.h"
#include "str.h"

#define STB_TEXTEDIT_CHARTYPE uint32_t
#include "stb_textedit.h"

typedef struct textedit_str_t {
    textedit_t* textedit;
    size_t len;
    uint32_t* codepoints;
} textedit_str_t;

struct textedit_t {
    font_t* font;
    size_t height;
    STB_TexteditState state;
    textedit_str_t str;
};

int delete_chars(textedit_str_t* obj, int i, int n) {
    uint32_t* new = malloc((obj->len-n+1)*4);
    if (!new) return 0;
    
    memcpy(new, obj->codepoints, i*4);
    memcpy(new+i, obj->codepoints+i+n, (obj->len-n-i)*4);
    
    obj->len -= n;
    free(obj->codepoints);
    obj->codepoints = new;
    obj->codepoints[obj->len] = 0;
    
    return 1;
}

int insert_chars(textedit_str_t* obj, int i, uint32_t* c, int n) {
    uint32_t* new = malloc((obj->len+n+1)*4);
    if (!new) return 0;
    
    memcpy(new, obj->codepoints, i*4);
    memcpy(new+i, c, n*4);
    memcpy(new+i+n, obj->codepoints+i, (obj->len-i)*4);
    
    obj->len += n;
    free(obj->codepoints);
    obj->codepoints = new;
    obj->codepoints[obj->len] = 0;
    
    return 1;
}

void get_row_layout(StbTexteditRow* row, textedit_str_t* obj, int n) {
    if (n >= obj->len) n = obj->len - 1;
    uint32_t* codepoints = obj->codepoints + n;
    
    size_t line = 0;
    for (uint32_t* c = obj->codepoints; c!=codepoints; c++)
        if (*c == '\n') line++;
    
    size_t num_codepoints = 0;
    for (uint32_t* c = codepoints; *c && *c!='\n'; c++)
        num_codepoints++;
    
    uint32_t last = codepoints[num_codepoints];
    codepoints[num_codepoints] = 0;
    
    row->x0 = 0;
    row->x1 = font_drawn_width(obj->textedit->font, codepoints,
                                   obj->textedit->height);
    row->baseline_y_delta = obj->textedit->height;
    row->ymin = line*obj->textedit->height;
    //TODO: For some reason adding '(int)' fixes a bug where
    //when you click on the text it does not move the cursor
    row->ymax = (int)row->ymin - obj->textedit->height;
    row->num_chars = num_codepoints + 1;
    
    codepoints[num_codepoints] = last;
}

float get_width(textedit_str_t* obj, int n, int i) {
    return font_get_advance(obj->textedit->font, obj->textedit->height,
                            i?obj->codepoints[i-1]:0, obj->codepoints[i]);
}

#define STB_TEXTEDIT_STRING textedit_str_t
#define STB_TEXTEDIT_STRINGLEN(obj) ((obj)->len)
#define STB_TEXTEDIT_LAYOUTROW(r, obj, n) get_row_layout((r), (obj), (n))
#define STB_TEXTEDIT_GETWIDTH(obj, n, i) get_width((obj), (n), (i));
#define STB_TEXTEDIT_KEYTOTEXT(k) (k&~(STB_TEXTEDIT_K_SHIFT))
#define STB_TEXTEDIT_GETCHAR(obj, i) ((obj)->codepoints[(i)])
#define STB_TEXTEDIT_NEWLINE ((uint32_t)'\n')
#define STB_TEXTEDIT_DELETECHARS(obj, i, n) delete_chars((obj), (i), (n))
#define STB_TEXTEDIT_INSERTCHARS(obj, i, c, n) insert_chars((obj), (i), (c), (n))
#define STB_TEXTEDIT_K_SHIFT (1<<30)
#define STB_TEXTEDIT_K_LEFT ((1<<30)-1)
#define STB_TEXTEDIT_K_RIGHT ((1<<30)-2)
#define STB_TEXTEDIT_K_UP ((1<<30)-3)
#define STB_TEXTEDIT_K_DOWN ((1<<30)-4)
#define STB_TEXTEDIT_K_LINESTART ((1<<30)-5)
#define STB_TEXTEDIT_K_LINEEND ((1<<30)-6)
#define STB_TEXTEDIT_K_TEXTSTART ((1<<30)-7)
#define STB_TEXTEDIT_K_TEXTEND ((1<<30)-8)
#define STB_TEXTEDIT_K_DELETE ((1<<30)-9)
#define STB_TEXTEDIT_K_BACKSPACE ((1<<30)-10)
#define STB_TEXTEDIT_K_UNDO ((1<<30)-11)
#define STB_TEXTEDIT_K_REDO ((1<<30)-12)
#define STB_TEXTEDIT_K_INSERT ((1<<30)-13)
#define STB_TEXTEDIT_IS_SPACE(ch) isspace(ch)
#define STB_TEXTEDIT_K_WORDLEFT ((1<<30)-14)
#define STB_TEXTEDIT_K_WORDRIGHT ((1<<30)-15)

#define STB_TEXTEDIT_IMPLEMENTATION
#include "stb_textedit.h"

textedit_t* textedit_create(font_t* font, size_t height, bool single_line) {
    textedit_t* res = malloc(sizeof(textedit_t));
    res->font = font;
    res->height = height;
    stb_textedit_initialize_state(&res->state, single_line);
    res->str.textedit = res;
    res->str.len = 0;
    res->str.codepoints = malloc(4);
    res->str.codepoints[0] = 0;
    return res;
}

void textedit_del(textedit_t* textedit) {
    free(textedit->str.codepoints);
    free(textedit);
}

void textedit_event(textedit_t* edit, float left, float bottom, SDL_Event event) {
    aabb_t aabb = textedit_calc_aabb(edit, left, bottom);
    
    switch (event.type) {
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEMOTION: {
        SDL_Window* win = SDL_GetWindowFromID(event.button.windowID);
        int x = event.type==SDL_MOUSEBUTTONDOWN ? event.button.x : event.motion.x;
        int y = event.type==SDL_MOUSEBUTTONDOWN ? event.button.y : event.motion.y;
        int h;
        SDL_GetWindowSize(win, NULL, &h);
        y = h - event.button.y - 1;
        bool intersect = x>aabb.left && x<aabb_right(aabb) &&
                         y>aabb.bottom && y<aabb_top(aabb);
        if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_LEFT && intersect)
            stb_textedit_click(&edit->str, &edit->state,
                               event.button.x-aabb.left, y-aabb.bottom);
        else if (event.type == SDL_MOUSEMOTION && event.motion.state&SDL_BUTTON_LMASK && intersect)
            stb_textedit_drag(&edit->str, &edit->state, x, y);
        break;
    }
    case SDL_TEXTINPUT: {
        uint32_t* utf32 = utf8_to_utf32((uint8_t*)event.text.text);
        for (uint32_t* c = utf32; *c; c++)
            stb_textedit_key(&edit->str, &edit->state, *c);
        free(utf32);
        break;
    }
    case SDL_KEYDOWN: {
        int key = event.key.keysym.mod&KMOD_SHIFT ? STB_TEXTEDIT_K_SHIFT : 0;
        switch (event.key.keysym.sym) {
        case SDLK_LEFT:
            key |= event.key.keysym.mod&KMOD_CTRL ?
                   STB_TEXTEDIT_K_WORDLEFT :
                   STB_TEXTEDIT_K_LEFT;
            break;
        case SDLK_RIGHT:
            key |= event.key.keysym.mod&KMOD_CTRL ?
                   STB_TEXTEDIT_K_WORDRIGHT :
                   STB_TEXTEDIT_K_RIGHT;
            break;
        case SDLK_UP:
            key |= STB_TEXTEDIT_K_UP;
            break;
        case SDLK_DOWN:
            key |= STB_TEXTEDIT_K_DOWN;
            break;
        case SDLK_DELETE:
            key |= STB_TEXTEDIT_K_DELETE;
            break;
        case SDLK_BACKSPACE:
            key |= STB_TEXTEDIT_K_BACKSPACE;
            break;
        case SDLK_UNDO:
            key |= STB_TEXTEDIT_K_UNDO;
            break;
        case SDLK_AGAIN:
            key |= STB_TEXTEDIT_K_REDO;
            break;
        case SDLK_INSERT:
            key |= STB_TEXTEDIT_K_INSERT;
            break;
        case SDLK_z:
            key |= event.key.keysym.mod&KMOD_CTRL ?
                   STB_TEXTEDIT_K_UNDO : 0;
            break;
        case SDLK_y:
            key |= event.key.keysym.mod&KMOD_CTRL ?
                   STB_TEXTEDIT_K_REDO : 0;
            break;
        case SDLK_RETURN:
            key |= '\n';
            break;
        default:
            return;
        }
        stb_textedit_key(&edit->str, &edit->state, key);
        break;
    }
    }
    
    if (false) {
        stb_textedit_cut(&edit->str, &edit->state);
        stb_textedit_paste(&edit->str, &edit->state, NULL, 0);
    }
}

void textedit_draw(textedit_t* edit, textedit_draw_callbacks_t callbacks) {
    uint32_t* text = edit->str.codepoints;
    font_t* font = edit->font;
    
    size_t sel_start=edit->state.select_start, sel_end=edit->state.select_end;
    bool selection = sel_start != sel_end;
    if (sel_end < sel_start) {
        size_t temp = sel_end;
        sel_end = sel_start;
        sel_start = temp;
    }
    
    size_t cursor = selection ? sel_start : edit->state.cursor;
    float pos = 0;
    size_t cur_line = 0;
    for (size_t i = 0; i < cursor; i++) {
        if (text[i] == '\n') {
            pos = 0;
            cur_line++;
        } else {
            pos += font_get_advance(font, edit->height, i==0?0:text[i-1], text[i]);
        }
    }
    
    aabb_t aabb = textedit_calc_aabb(edit, 0, 0);
    if (selection) {
        float left = pos;
        float width = 0;
        for (uint32_t* c = text+sel_start; c<(text+sel_end); c++) {
            if (*c == '\n') {
                callbacks.draw_sel(left, aabb.width-left, cur_line, callbacks.userdata);
                cur_line++;
                left = width = 0;
            } else {
                width += font_get_advance(font, edit->height, c==text?0:c[-1], c[0]);
            }
        }
        
        callbacks.draw_sel(left, width, cur_line, callbacks.userdata);
    } else if (textedit_is_insert(edit)) {
        callbacks.draw_insert(pos, font_get_advance(font, edit->height, cursor?text[cursor-1]:0, text[cursor]),
                              cur_line, callbacks.userdata);
    } else {
        callbacks.draw_cur(pos, cur_line, callbacks.userdata);
    }
    
    size_t line = 0;
    for (uint32_t* c = text;; line++) {
        uint32_t* end = c;
        for (; *end!='\n' && *end; end++);
        uint32_t last = *end;
        *end = 0;
        callbacks.draw_text(c, line, callbacks.userdata);
        *end = last;
        if(!*end) break;
        c = end + 1;
    }
}

const uint32_t* textedit_get(textedit_t* edit) {
    return edit->str.codepoints;
}

size_t textedit_get_cursor(textedit_t* edit) {
    return edit->state.cursor;
}

bool textedit_is_insert(textedit_t* edit) {
    return edit->state.insert_mode;
}

bool textedit_get_selection(textedit_t* edit, size_t* start, size_t* end) {
    if (start) *start = edit->state.select_start;
    if (end) *end = edit->state.select_end;
    return edit->state.select_start != edit->state.select_end;
}

aabb_t textedit_calc_aabb(textedit_t* edit, float left, float bottom) {
    float width = 0;
    float row_width = 0;
    float height = 0;
    for (uint32_t* c = edit->str.codepoints; *c; c++) {
        if (*c == '\n') {
            width = fmax(width, row_width);
            row_width = 0;
            height += edit->height;
        } else {
            row_width += font_get_advance(edit->font, edit->height, c==edit->str.codepoints?0:c[-1], c[0]);
        }
    }
    width = fmax(width, row_width);
    height += edit->height;
    
    return aabb_create_lbwh(left, bottom, width, height);
}
