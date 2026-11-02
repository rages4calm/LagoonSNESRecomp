#ifndef LAGOON_UI_H
#define LAGOON_UI_H
#include <stdint.h>

/* Native host panels use their own licensed font, never mutable SNES glyphs. */
#define LAGOON_UI_BG       UINT32_C(0xff08090d)
#define LAGOON_UI_FG       UINT32_C(0xfff6f1df)
#define LAGOON_UI_DIM      UINT32_C(0xffbdb9ad)
#define LAGOON_UI_GOLD     UINT32_C(0xfff2ce7c)
#define LAGOON_UI_LINE     UINT32_C(0xff4e5056)
#define LAGOON_UI_SELECT   UINT32_C(0xff292a32)
enum { LAGOON_UI_LINE_HEIGHT=20, LAGOON_UI_SMALL_HEIGHT=12 };

void LagoonUI_Fill(uint32_t *dst,int width,int height,int x,int y,int w,int h,uint32_t colour);
void LagoonUI_Frame(uint32_t *dst,int width,int height,int x,int y,int w,int h);
void LagoonUI_Cursor(uint32_t *dst,int width,int height,int x,int y,uint32_t colour);
int LagoonUI_TextWidth(const char *text,int scale);
void LagoonUI_Text(uint32_t *dst,int width,int height,int x,int y,const char *text,uint32_t colour,int scale);
void LagoonUI_TextFit(uint32_t *dst,int width,int height,int x,int y,const char *text,uint32_t colour,int scale,int max_width);
int LagoonUI_SmallWidth(const char *text);
void LagoonUI_TextSmall(uint32_t *dst,int width,int height,int x,int y,const char *text,uint32_t colour);
void LagoonUI_SmallFit(uint32_t *dst,int width,int height,int x,int y,const char *text,uint32_t colour,int max_width);
#endif
