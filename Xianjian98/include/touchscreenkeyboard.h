#ifndef _H_TOUCHSCREENKEYBOARD
#define _H_TOUCHSCREENKEYBOARD
#ifdef __cplusplus
extern "C" {
#endif


#include "SDL_video.h"
#include "SDL_opengles.h"

//#include "SDL_pixels_c.h"
//#include "SDL_rect_c.h"
//#include "SDL_yuv_sw_c.h"


#define MAX_BUTTONS 20

enum MOUSE_ACTION { MOUSE_DOWN = 0, MOUSE_UP=1, MOUSE_MOVE=2 };
enum { ARROW_LEFT = 1, ARROW_RIGHT = 2, ARROW_UP = 4, ARROW_DOWN = 8 };

typedef struct
{
    GLuint id;
    GLfloat w;
    GLfloat h;
} GLTexture_t;


 static inline int InsideRect(const SDL_Rect * r, int x, int y)
{
	return ( x >= r->x && x <= r->x + r->w ) && ( y >= r->y && y <= r->y + r->h );
}
extern int processTouchscreenKeyboard(int x, int y, int action, int pointerId);
extern int drawTouchscreenKeyboard();

extern void init_screen_key_size(int oritation, int width, int heigth);
  extern void keybutton( unsigned char* charBuf, int len,int w, int h);
    //set the button scan code end with 0
    //fire strafe gun jump
extern void keyboard_set_button_key(int num, ...);
    //set  the button position
    //buttonIndex [0 3]
    //-1 arrow
    //0 fire
    //1 strafe
    //2 gun
    //3 jump
    //4 escape
    //when button index is 1. and the w == h. we think that the strafe button is only used for  
    //ont key , not 2
    extern void setButtonPosition(int buttonIndex, int x, int y, int w, int h);
        extern void  enable_turn_send_alt();
extern void set_keyboard(int);
extern void set_arrow_key_isometric(int enable);
#define LOGI(...) printf("%s\n", __VA_ARGS__)

#ifdef __cplusplus
}
#endif
#endif
