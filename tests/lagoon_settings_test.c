#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "../src/lagoon_settings.c"
static int master=73;static unsigned music=100,master_calls;
int snesrecomp_desktop_get_volume(void){return master;}
void snesrecomp_desktop_set_volume(int v){assert(v>=0&&v<=100);master=v;master_calls++;}
void LagoonSoundtrack_SetVolume(unsigned v){assert(v<=100);music=v;}
unsigned LagoonSoundtrack_GetVolume(void){return music;}
static void write(const char *path,const char *text){FILE*f=file_open(path,"wb");assert(f);assert(fwrite(text,1,strlen(text),f)==strlen(text));assert(!fclose(f));}
int main(int argc,char**argv){
 assert(argc==2);const char *path=argv[1];
 const char *original="\xef\xbb\xbf# keep comment\r\n[Sound]\r\nVolume = 73\r\n[Unknown]\r\nPrivate = keep\r\n[Lagoon]\r\nOther = retain\r\nSoundtrackVolume = 60 # old\r\n[KeyMap]\r\nPause = Shift+p\r\n";
 write(path,original);LagoonSettings_Configure(path);assert(music==60&&!LagoonSettings_IsOpen());
 LagoonSettings_Open();assert(LagoonSettings_PollOpen(SNES_PAD_RIGHT));LagoonSettings_Tick(SNES_PAD_RIGHT,100);assert(master==73);
 LagoonSettings_Tick(0,110);LagoonSettings_Tick(SNES_PAD_RIGHT,120);assert(master==78&&master_calls==1);
 LagoonSettings_Tick(SNES_PAD_RIGHT,460);assert(master==78);LagoonSettings_Tick(SNES_PAD_RIGHT,470);assert(master==83);
 LagoonSettings_Command(LAGOON_SETTINGS_DOWN);LagoonSettings_Command(LAGOON_SETTINGS_LEFT);assert(music==55&&!save_failed);
 size_t n=0;char *result=file_read(path,&n);assert(result&&strstr(result,"SoundtrackVolume = 55\r\n")&&strstr(result,"Private = keep\r\n")&&strstr(result,"[KeyMap]\r\nPause = Shift+p\r\n")&&!strncmp(result,"\xef\xbb\xbf",3));free(result);
 LagoonSettings_Command(LAGOON_SETTINGS_CLOSE);LagoonSettings_Configure(path);assert(music==55);
 LagoonSettings_Open();LagoonSettings_PollOpen(0);LagoonSettings_Tick(SNES_PAD_B,500);assert(!LagoonSettings_IsOpen());
 /* Insert into an existing Lagoon section and then create an absent one,
  * preserving LF/CRLF, BOM and all unrelated bytes in each case. */
 write(path,"[Lagoon]\nOther=42\n[Sound]\nVolume=73\n");LagoonSettings_Configure(path);assert(music==100);
 LagoonSettings_Open();LagoonSettings_Command(LAGOON_SETTINGS_DOWN);LagoonSettings_Command(LAGOON_SETTINGS_LEFT);assert(music==95&&!save_failed);
 result=file_read(path,&n);assert(!strcmp(result,"[Lagoon]\nOther=42\nSoundtrackVolume = 95\n[Sound]\nVolume=73\n"));free(result);
 write(path,"[Sound]\nVolume=73");LagoonSettings_Configure(path);LagoonSettings_Open();LagoonSettings_Command(LAGOON_SETTINGS_DOWN);LagoonSettings_Command(LAGOON_SETTINGS_LEFT);
 result=file_read(path,&n);assert(!strcmp(result,"[Sound]\nVolume=73\n[Lagoon]\nSoundtrackVolume = 95\n"));free(result);
 const char *bad[]={"[Lagoon]\nSoundtrackVolume=20\nSoundtrackVolume=30\n","[Lagoon]\r\nSoundtrackVolume=20\n","[Lagoon]\nSoundtrackVolume=101\n","[Lagoon]\nSoundtrackVolume=\n20\n"};
 for(unsigned i=0;i<sizeof bad/sizeof *bad;i++){
  write(path,bad[i]);LagoonSettings_Configure(path);assert(music==100);LagoonSettings_Open();LagoonSettings_Command(LAGOON_SETTINGS_DOWN);LagoonSettings_Command(LAGOON_SETTINGS_LEFT);assert(music==95&&save_failed);
  result=file_read(path,&n);assert(!strcmp(result,bad[i]));free(result);
 }
 write(path,"[Lagoon]\nSoundtrackVolume=0\n");LagoonSettings_Configure(path);LagoonSettings_Open();LagoonSettings_Command(LAGOON_SETTINGS_DOWN);LagoonSettings_Command(LAGOON_SETTINGS_LEFT);assert(music==0);
 const uint32_t *pixels=NULL;int width=0,height=0;assert(LagoonSettings_Image(&pixels,&width,&height)&&pixels&&width==640&&height==360);
 assert(pixels[0]==0xff65ddc3);LagoonSettings_Close();assert(!LagoonSettings_Image(&pixels,&width,&height));
 /* The read-only browser action is a one-shot host request. Selecting it
  * never changes either volume or the owner's settings document. */
 char *unchanged=file_read(path,&n);unsigned before_music=music,before_calls=master_calls;
 LagoonSettings_Open();LagoonSettings_Command(LAGOON_SETTINGS_UP);assert(selected==2);
 LagoonSettings_Command(LAGOON_SETTINGS_LEFT);LagoonSettings_Command(LAGOON_SETTINGS_RIGHT);
 assert(music==before_music&&master_calls==before_calls&&!LagoonSettings_TakeAchievementsRequest());
 LagoonSettings_PollOpen(SNES_PAD_A);LagoonSettings_Tick(SNES_PAD_A,800);
 assert(LagoonSettings_IsOpen()&&!LagoonSettings_TakeAchievementsRequest());
 LagoonSettings_Tick(0,810);LagoonSettings_Tick(SNES_PAD_A,820);
 assert(!LagoonSettings_IsOpen()&&LagoonSettings_TakeAchievementsRequest()&&!LagoonSettings_TakeAchievementsRequest());
 result=file_read(path,&n);assert(!strcmp(result,unchanged));free(result);free(unchanged);
 LagoonSettings_Open();LagoonSettings_Command(LAGOON_SETTINGS_ACTIVATE);assert(LagoonSettings_IsOpen()&&!LagoonSettings_TakeAchievementsRequest());
 LagoonSettings_Command(LAGOON_SETTINGS_DOWN);LagoonSettings_Command(LAGOON_SETTINGS_DOWN);assert(selected==2);
 LagoonSettings_Command(LAGOON_SETTINGS_DOWN);assert(selected==0);LagoonSettings_Close();
 puts("PASS settings overlay: master/music5steps, heldrepeat, opener latch, close, limits; targeted atomic UTF8/BOM/newline INI persistence, unknownfields, duplicate/malformed preservation; frozen640x360 image; one-shot browser request, heldopener protection and unchanged volume/settings");
 return 0;
}
