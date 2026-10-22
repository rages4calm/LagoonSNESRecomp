#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/lagoon_achievement_ui.c"

static size_t count=12;
static LagoonAchievementRow definitions[64];
size_t LagoonAchievements_Count(void){return count;}
bool LagoonAchievements_GetRow(size_t index,LagoonAchievementRow *out){if(index>=count||!out)return false;*out=definitions[index];return true;}
size_t LagoonAchievements_Unlocked(void){size_t n=0;for(size_t i=0;i<count;i++)n+=definitions[i].unlocked_at!=0;return n;}
const char *LagoonAchievements_Status(void){return "Tracking ready - local progress";}
int main(void){
  for(unsigned i=0;i<64;i++)definitions[i]=(LagoonAchievementRow){"goal","A remembered journey","A description that wraps safely without hiding the controls or the tracking status.",i,100,0,false};
  definitions[1].unlocked_at=1700000000;
  definitions[2].secret=true;definitions[2].title="Spoiler: a dancing visitor";
  LagoonAchievementRow unchanged[64];memcpy(unchanged,definitions,sizeof definitions);
  const uint32_t *pixels=NULL;int width=0,height=0;
  LagoonAchievementUI_Reset();assert(!LagoonAchievementUI_IsOpen()&&!LagoonAchievementUI_Image(&pixels,&width,&height));
  LagoonAchievementUI_Command(LAGOON_ACHIEVEMENT_UI_DOWN);assert(selected==0);
  LagoonAchievementUI_Open();assert(LagoonAchievementUI_PollOpen(SNES_PAD_DOWN));
  LagoonAchievementUI_Tick(SNES_PAD_DOWN,0);assert(selected==0);
  LagoonAchievementUI_Tick(0,1);LagoonAchievementUI_Tick(SNES_PAD_DOWN,2);assert(selected==1);
  LagoonAchievementUI_Tick(SNES_PAD_DOWN,351);assert(selected==1);
  LagoonAchievementUI_Tick(SNES_PAD_DOWN,352);assert(selected==2);
  LagoonAchievementUI_Tick(SNES_PAD_DOWN,441);assert(selected==2);
  LagoonAchievementUI_Tick(SNES_PAD_DOWN,442);assert(selected==3);
  LagoonAchievementUI_Command(LAGOON_ACHIEVEMENT_UI_FIRST);LagoonAchievementUI_Command(LAGOON_ACHIEVEMENT_UI_UP);assert(selected==11);
  LagoonAchievementUI_Command(LAGOON_ACHIEVEMENT_UI_NEXT);assert(selected==1);
  LagoonAchievementUI_Command(LAGOON_ACHIEVEMENT_UI_PREVIOUS);assert(selected==11);
  LagoonAchievementUI_Command(LAGOON_ACHIEVEMENT_UI_DOWN);assert(selected==0);
  assert(!strcmp(row_title(&definitions[2]),"A familiar stranger"));
  assert(!strstr(row_description(&definitions[2]),"dancing"));
  assert(LagoonAchievementUI_Image(&pixels,&width,&height)&&pixels&&width==640&&height==360&&pixels[0]==ACCENT);
  LagoonAchievementUI_Tick(0,500);LagoonAchievementUI_Tick(SNES_PAD_B,501);assert(!LagoonAchievementUI_IsOpen());
  LagoonAchievementUI_StartToast(0,1000);assert(!LagoonAchievementUI_ToastActive());
  LagoonAchievementUI_StartToast(64,1000);assert(!LagoonAchievementUI_ToastActive());
  LagoonAchievementUI_StartToast(1,1000);assert(LagoonAchievementUI_ToastActive()&&LagoonAchievementUI_ToastImage(&pixels,&width,&height)&&width==144&&height==32);
  /* The native toast occupies less than one third of the old panel's area;
   * keep every border opaque and long copy clipped inside the compact shape. */
  assert(width*height<240*60/3);
  for(int x=0;x<width;x++)assert(pixels[x]==GOLD&&pixels[(height-1)*width+x]==GOLD);
  for(int y=0;y<height;y++)assert(pixels[y*width]==GOLD&&pixels[y*width+width-1]==GOLD);
  const char *title=definitions[1].title;
  definitions[1].title="A World Beyond Darkness";
  assert(LagoonAchievementUI_ToastImage(&pixels,&width,&height));
  definitions[1].title="ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ";
  assert(LagoonAchievementUI_ToastImage(&pixels,&width,&height));
  for(int y=0;y<height;y++)assert(pixels[y*width+width-1]==GOLD);
  definitions[1].title=title;
  LagoonAchievementUI_Open();assert(!LagoonAchievementUI_ToastImage(&pixels,&width,&height));LagoonAchievementUI_Close();
  LagoonAchievementUI_Tick(0,6499);assert(LagoonAchievementUI_ToastActive());LagoonAchievementUI_Tick(0,6500);assert(!LagoonAchievementUI_ToastActive());
  LagoonAchievementUI_StartToast(1,10000);LagoonAchievementUI_PauseToast(1,11000);LagoonAchievementUI_Tick(0,21000);
  assert(LagoonAchievementUI_ToastActive()&&!LagoonAchievementUI_ToastImage(&pixels,&width,&height));
  LagoonAchievementUI_PauseToast(0,21000);LagoonAchievementUI_Tick(0,25499);assert(LagoonAchievementUI_ToastActive());
  LagoonAchievementUI_Tick(0,25500);assert(!LagoonAchievementUI_ToastActive());
  LagoonAchievementUI_StartToast(1,0xfffffff0u);LagoonAchievementUI_Tick(0,0xfffffff0u+5500u);assert(!LagoonAchievementUI_ToastActive());
  count=1;LagoonAchievementUI_Open();LagoonAchievementUI_Command(LAGOON_ACHIEVEMENT_UI_PREVIOUS);assert(selected==0);
  count=0;LagoonAchievementUI_Command(LAGOON_ACHIEVEMENT_UI_NEXT);assert(selected==0&&LagoonAchievementUI_Image(&pixels,&width,&height));
  count=64;LagoonAchievementUI_Command(LAGOON_ACHIEVEMENT_UI_LAST);assert(selected==63);LagoonAchievementUI_Command(LAGOON_ACHIEVEMENT_UI_NEXT);assert(selected==3);
  assert(!memcmp(unchanged,definitions,sizeof definitions));
  puts("PASS achievement UI: read-only rows, heldopener/repeat/navigation, pagebounds0/1/12/64, secretmask, browserdismissal, committed-only toast and wrap-safe expiry; no achievement/guest writes");
  return 0;
}
