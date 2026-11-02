#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "../src/lagoon_settings.c"
static int master=73;static unsigned music=100,master_calls;
int snesrecomp_desktop_get_volume(void){return master;}
void snesrecomp_desktop_set_volume(int v){assert(v>=0&&v<=100);master=v;master_calls++;}
void LagoonSoundtrack_SetVolume(unsigned v){assert(v<=100);music=v;}
unsigned LagoonSoundtrack_GetVolume(void){return music;}
static size_t guide_count=15;static unsigned journal_reads,last_story_byte,owned_reads;
int LagoonJournal_Get(const uint8_t *r,size_t size,LagoonJournalEntry *entry){
 if(!r||size<LAGOON_JOURNAL_RAM_BYTES||!entry)return 0;journal_reads++;last_story_byte=r[0x4d0];
 *entry=(LagoonJournalEntry){.chapter="Voloh and Philips Castle",.title="The castle's spell",.detail="Speak with the elder in Voloh and bring the three tablets together. Thor can help you locate the remaining tablet.",.milestone={"Tablet of Peace found","Tablet of Faith found","Tablet of Hope found","Elder's spell learned"},.milestone_count=4,.completed_mask=3};return 1;
}
size_t LagoonItemGuide_Count(void){return guide_count;}
int LagoonItemGuide_GetAt(size_t index,LagoonItemGuideEntry *entry){
 if(index>=guide_count||!entry)return 0;
 *entry=(LagoonItemGuideEntry){.id=(uint8_t)(0x20+index),.name=index==2?"Shiny Ball":"Healing Pot",.kind="Consumable",.description=index==2?"A special item with an original shop price. This reference does not select or purchase an item.":"Restores health. The reference guide explains items without using anything in your inventory.",.atland_price=index==2?0:100,.voloh_price=index==2?10000:100};return 1;
}
int LagoonItemGuide_Owned(const uint8_t *r,size_t size,unsigned id){owned_reads++;return r&&size>=LAGOON_ITEM_GUIDE_RAM_BYTES&&id>=0x20&&id<=0x38&&!!(r[0x4d0+id/8]&(0x80u>>(id&7)));}
static void write_file(const char *path,const char *text){FILE*f=file_open(path,"wb");assert(f);assert(fwrite(text,1,strlen(text),f)==strlen(text));assert(!fclose(f));}
int main(int argc,char**argv){
 assert(argc==2||argc==3);const char *path=argv[1];
 const char *original="\xef\xbb\xbf# keep comment\r\n[Sound]\r\nVolume = 73\r\n[Unknown]\r\nPrivate = keep\r\n[Lagoon]\r\nOther = retain\r\nSoundtrackVolume = 60 # old\r\n[KeyMap]\r\nPause = Shift+p\r\n";
 write_file(path,original);LagoonSettings_Configure(path);assert(music==60&&!LagoonSettings_IsOpen());
 LagoonSettings_Open();assert(LagoonSettings_PollOpen(SNES_PAD_RIGHT));LagoonSettings_Tick(SNES_PAD_RIGHT,100);assert(master==73);
 LagoonSettings_Tick(0,110);LagoonSettings_Tick(SNES_PAD_RIGHT,120);assert(master==78&&master_calls==1);
 LagoonSettings_Tick(SNES_PAD_RIGHT,460);assert(master==78);LagoonSettings_Tick(SNES_PAD_RIGHT,470);assert(master==83);
 LagoonSettings_Command(LAGOON_SETTINGS_DOWN);LagoonSettings_Command(LAGOON_SETTINGS_LEFT);assert(music==55&&!save_failed);
 size_t n=0;char *result=file_read(path,&n);assert(result&&strstr(result,"SoundtrackVolume = 55\r\n")&&strstr(result,"Private = keep\r\n")&&strstr(result,"[KeyMap]\r\nPause = Shift+p\r\n")&&!strncmp(result,"\xef\xbb\xbf",3));free(result);
 LagoonSettings_Command(LAGOON_SETTINGS_CLOSE);LagoonSettings_Configure(path);assert(music==55);
 LagoonSettings_Open();LagoonSettings_PollOpen(0);LagoonSettings_Tick(SNES_PAD_B,500);assert(!LagoonSettings_IsOpen());
 /* Insert into an existing Lagoon section and then create an absent one,
  * preserving LF/CRLF, BOM and all unrelated bytes in each case. */
 write_file(path,"[Lagoon]\nOther=42\n[Sound]\nVolume=73\n");LagoonSettings_Configure(path);assert(music==100);
 LagoonSettings_Open();LagoonSettings_Command(LAGOON_SETTINGS_DOWN);LagoonSettings_Command(LAGOON_SETTINGS_LEFT);assert(music==95&&!save_failed);
 result=file_read(path,&n);assert(!strcmp(result,"[Lagoon]\nOther=42\nSoundtrackVolume = 95\n[Sound]\nVolume=73\n"));free(result);
 write_file(path,"[Sound]\nVolume=73");LagoonSettings_Configure(path);LagoonSettings_Open();LagoonSettings_Command(LAGOON_SETTINGS_DOWN);LagoonSettings_Command(LAGOON_SETTINGS_LEFT);
 result=file_read(path,&n);assert(!strcmp(result,"[Sound]\nVolume=73\n[Lagoon]\nSoundtrackVolume = 95\n"));free(result);
 const char *bad[]={"[Lagoon]\nSoundtrackVolume=20\nSoundtrackVolume=30\n","[Lagoon]\r\nSoundtrackVolume=20\n","[Lagoon]\nSoundtrackVolume=101\n","[Lagoon]\nSoundtrackVolume=\n20\n"};
 for(unsigned i=0;i<sizeof bad/sizeof *bad;i++){
  write_file(path,bad[i]);LagoonSettings_Configure(path);assert(music==100);LagoonSettings_Open();LagoonSettings_Command(LAGOON_SETTINGS_DOWN);LagoonSettings_Command(LAGOON_SETTINGS_LEFT);assert(music==95&&save_failed);
  result=file_read(path,&n);assert(!strcmp(result,bad[i]));free(result);
 }
 write_file(path,"[Lagoon]\nSoundtrackVolume=0\n");LagoonSettings_Configure(path);LagoonSettings_Open();LagoonSettings_Command(LAGOON_SETTINGS_DOWN);LagoonSettings_Command(LAGOON_SETTINGS_LEFT);assert(music==0);
 const uint32_t *pixels=NULL;int width=0,height=0;assert(LagoonSettings_Image(&pixels,&width,&height)&&pixels&&width==640&&height==360);
 assert(pixels[2*width+2]==LAGOON_UI_FG);
 if(argc>2){FILE*f=fopen(argv[2],"wb");assert(f);fprintf(f,"P6\n%d %d\n255\n",width,height);for(int i=0;i<width*height;i++){unsigned c=pixels[i];fputc((c>>16)&255,f);fputc((c>>8)&255,f);fputc(c&255,f);}assert(!fclose(f));}
 LagoonSettings_Close();assert(!LagoonSettings_Image(&pixels,&width,&height));
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
 /* Host tabs consume copied context, never a borrowed/mutable game view. */
 unsigned char source[8192]={0},source_before[8192];source[0x4d0]=0x80;memcpy(source_before,source,sizeof source);
 LagoonSettings_Observe(source,sizeof source);assert(!memcmp(source,source_before,sizeof source)&&context_size==8192);
 source[0x4d0]=0;LagoonSettings_Open();assert(active_tab==TAB_SOUND);
 unsigned old_music=music,old_calls=master_calls;int old_master=master;
 LagoonSettings_Command(LAGOON_SETTINGS_NEXT_TAB);assert(active_tab==TAB_JOURNAL);
 LagoonSettings_Command(LAGOON_SETTINGS_LEFT);LagoonSettings_Command(LAGOON_SETTINGS_RIGHT);LagoonSettings_Command(LAGOON_SETTINGS_ACTIVATE);
 assert(LagoonSettings_Image(&pixels,&width,&height)&&last_story_byte==0x80&&journal_reads);
 if(argc>2){char out[1024];snprintf(out,sizeof out,"%s.journal.ppm",argv[2]);FILE*f=fopen(out,"wb");assert(f);fprintf(f,"P6\n%d %d\n255\n",width,height);for(int i=0;i<width*height;i++){unsigned c=pixels[i];fputc((c>>16)&255,f);fputc((c>>8)&255,f);fputc(c&255,f);}fclose(f);}
 LagoonSettings_Command(LAGOON_SETTINGS_NEXT_TAB);assert(active_tab==TAB_ITEMS&&item_selected==0);
 LagoonSettings_Command(LAGOON_SETTINGS_UP);assert(item_selected==14);LagoonSettings_Command(LAGOON_SETTINGS_DOWN);assert(item_selected==0);
 LagoonSettings_Command(LAGOON_SETTINGS_DOWN);LagoonSettings_Command(LAGOON_SETTINGS_DOWN);assert(item_selected==2);
 LagoonSettings_Command(LAGOON_SETTINGS_LEFT);LagoonSettings_Command(LAGOON_SETTINGS_RIGHT);LagoonSettings_Command(LAGOON_SETTINGS_ACTIVATE);
 assert(LagoonSettings_IsOpen()&&!LagoonSettings_TakeAchievementsRequest()&&LagoonSettings_Image(&pixels,&width,&height)&&owned_reads);
 if(argc>2){char out[1024];snprintf(out,sizeof out,"%s.items.ppm",argv[2]);FILE*f=fopen(out,"wb");assert(f);fprintf(f,"P6\n%d %d\n255\n",width,height);for(int i=0;i<width*height;i++){unsigned c=pixels[i];fputc((c>>16)&255,f);fputc((c>>8)&255,f);fputc(c&255,f);}fclose(f);}
 assert(music==old_music&&master==old_master&&master_calls==old_calls);
 LagoonSettings_Command(LAGOON_SETTINGS_NEXT_TAB);assert(active_tab==TAB_DISPLAY);
 LagoonSettings_Command(LAGOON_SETTINGS_NEXT_TAB);assert(active_tab==TAB_TRAVEL);
 LagoonSettings_Command(LAGOON_SETTINGS_NEXT_TAB);assert(active_tab==TAB_SOUND);
 LagoonSettings_PollOpen(SNES_PAD_R);LagoonSettings_Tick(SNES_PAD_R,1000);assert(active_tab==TAB_SOUND);
 LagoonSettings_Tick(0,1010);LagoonSettings_Tick(SNES_PAD_R,1020);assert(active_tab==TAB_JOURNAL);
 LagoonSettings_Tick(SNES_PAD_R,1800);assert(active_tab==TAB_JOURNAL);
 LagoonSettings_Tick(0,1810);LagoonSettings_Tick(SNES_PAD_L,1820);assert(active_tab==TAB_SOUND);
 LagoonSettings_Command(LAGOON_SETTINGS_NEXT_TAB);unsigned reads=journal_reads;LagoonSettings_Observe(NULL,0);
 assert(!context_size&&LagoonSettings_Image(&pixels,&width,&height)&&journal_reads==reads);
 LagoonSettings_Observe(source,LAGOON_JOURNAL_RAM_BYTES-1);assert(!context_size);
 LagoonSettings_Observe(source,sizeof source);LagoonSettings_Configure(path);assert(!context_size&&!opened&&active_tab==TAB_SOUND);
 assert(music==old_music&&master==old_master);LagoonSettings_Open();LagoonSettings_Command(LAGOON_SETTINGS_PREVIOUS_TAB);assert(active_tab==TAB_TRAVEL);
 LagoonSettings_Command(LAGOON_SETTINGS_PREVIOUS_TAB);assert(active_tab==TAB_DISPLAY);
 LagoonSettings_Command(LAGOON_SETTINGS_PREVIOUS_TAB);assert(active_tab==TAB_ITEMS);
 guide_count=25;LagoonSettings_Command(LAGOON_SETTINGS_UP);assert(item_selected==24);LagoonSettings_Command(LAGOON_SETTINGS_DOWN);assert(item_selected==0&&LagoonSettings_Image(&pixels,&width,&height));
 guide_count=0;LagoonSettings_Command(LAGOON_SETTINGS_DOWN);assert(item_selected==0&&LagoonSettings_Image(&pixels,&width,&height));
 LagoonSettings_Close();LagoonSettings_Command(LAGOON_SETTINGS_NEXT_TAB);assert(active_tab==TAB_ITEMS);
 /* The optional range cue is OFF until a deliberate Display-page action.
  * Viewing, tab/navigation and preference reload never write the document. */
 write_file(path,original);LagoonSettings_Configure(path);assert(!LagoonSettings_SwordRangeEnabled()&&music==60);
 unchanged=file_read(path,&n);old_calls=master_calls;
 LagoonSettings_Open();for(unsigned i=0;i<3;i++)LagoonSettings_Command(LAGOON_SETTINGS_NEXT_TAB);assert(active_tab==TAB_DISPLAY);
 LagoonSettings_Command(LAGOON_SETTINGS_UP);LagoonSettings_Command(LAGOON_SETTINGS_DOWN);assert(!LagoonSettings_SwordRangeEnabled());
 assert(LagoonSettings_Image(&pixels,&width,&height));result=file_read(path,&n);assert(!strcmp(result,unchanged));free(result);free(unchanged);
 if(argc>2){char out[1024];snprintf(out,sizeof out,"%s.display.ppm",argv[2]);FILE*f=fopen(out,"wb");assert(f);fprintf(f,"P6\n%d %d\n255\n",width,height);for(int i=0;i<width*height;i++){unsigned c=pixels[i];fputc((c>>16)&255,f);fputc((c>>8)&255,f);fputc(c&255,f);}fclose(f);}
 LagoonSettings_Command(LAGOON_SETTINGS_ACTIVATE);assert(LagoonSettings_SwordRangeEnabled()&&!range_save_failed&&music==60&&master_calls==old_calls);
 result=file_read(path,&n);assert(strstr(result,"ShowSwordRange = 1\r\n[KeyMap]")&&strstr(result,"SoundtrackVolume = 60 # old\r\n")&&strstr(result,"Private = keep\r\n")&&!strncmp(result,"\xef\xbb\xbf",3));free(result);
 LagoonSettings_Configure(path);assert(LagoonSettings_SwordRangeEnabled());LagoonSettings_Open();for(unsigned i=0;i<3;i++)LagoonSettings_Command(LAGOON_SETTINGS_NEXT_TAB);
 LagoonSettings_Command(LAGOON_SETTINGS_RIGHT);assert(!LagoonSettings_SwordRangeEnabled());
 LagoonSettings_PollOpen(SNES_PAD_A);LagoonSettings_Tick(SNES_PAD_A,1000);assert(!LagoonSettings_SwordRangeEnabled());
 LagoonSettings_Tick(0,1010);LagoonSettings_Tick(SNES_PAD_A|SNES_PAD_LEFT,1020);assert(LagoonSettings_SwordRangeEnabled()); /* combined input toggles once */
 LagoonSettings_Tick(SNES_PAD_A|SNES_PAD_LEFT,1300);assert(LagoonSettings_SwordRangeEnabled());
 LagoonSettings_Tick(0,1310);LagoonSettings_Tick(SNES_PAD_LEFT,1320);assert(!LagoonSettings_SwordRangeEnabled());
 LagoonSettings_Tick(SNES_PAD_LEFT,1669);assert(!LagoonSettings_SwordRangeEnabled());LagoonSettings_Tick(SNES_PAD_LEFT,1670);assert(LagoonSettings_SwordRangeEnabled());
 LagoonSettings_Close();LagoonSettings_Command(LAGOON_SETTINGS_ACTIVATE);assert(LagoonSettings_SwordRangeEnabled());
 LagoonSettings_Configure(path);assert(LagoonSettings_SwordRangeEnabled());LagoonSettings_Open();LagoonSettings_Command(LAGOON_SETTINGS_DOWN);LagoonSettings_Command(LAGOON_SETTINGS_LEFT);
 result=file_read(path,&n);assert(music==55&&strstr(result,"ShowSwordRange = 1\r\n"));free(result);
 const char *bad_range[]={"[Lagoon]\nShowSwordRange=0\nShowSwordRange=1\n","[Lagoon]\nShowSwordRange=2\n","[Lagoon]\nShowSwordRange=true\n","[Lagoon]\r\nShowSwordRange=0\n"};
 for(unsigned i=0;i<sizeof bad_range/sizeof *bad_range;i++){
  write_file(path,bad_range[i]);LagoonSettings_Configure(path);assert(!LagoonSettings_SwordRangeEnabled());LagoonSettings_Open();for(unsigned t=0;t<3;t++)LagoonSettings_Command(LAGOON_SETTINGS_NEXT_TAB);
  LagoonSettings_Command(LAGOON_SETTINGS_LEFT);assert(LagoonSettings_SwordRangeEnabled()&&range_save_failed);
  result=file_read(path,&n);assert(!strcmp(result,bad_range[i]));free(result);
 }
 /* Travel is a host request only. Eligibility text is owned by this panel;
  * entry/confirmation/navigation never touches guest RAM or preferences. */
 write_file(path,original);LagoonSettings_Configure(path);assert(!travel_available&&!travel_confirming&&!LagoonSettings_TakeReturnRequest());
 LagoonSettings_Open();LagoonSettings_Command(LAGOON_SETTINGS_PREVIOUS_TAB);assert(active_tab==TAB_TRAVEL);
 old_calls=master_calls;old_music=music;old_master=master;unchanged=file_read(path,&n);memcpy(source_before,source,sizeof source);
 LagoonSettings_Command(LAGOON_SETTINGS_ACTIVATE);assert(opened&&!travel_confirming&&!LagoonSettings_TakeReturnRequest());
 char available_message[]="Ready to return to the Gold Cave entrance.";
 LagoonSettings_SetTravelStatus(available_message,1);available_message[0]='X';assert(travel_status[0]=='R'&&travel_available);
 assert(LagoonSettings_Image(&pixels,&width,&height));
 if(argc>2){char out[1024];snprintf(out,sizeof out,"%s.travel.ppm",argv[2]);FILE*f=fopen(out,"wb");assert(f);fprintf(f,"P6\n%d %d\n255\n",width,height);for(int i=0;i<width*height;i++){unsigned c=pixels[i];fputc((c>>16)&255,f);fputc((c>>8)&255,f);fputc(c&255,f);}fclose(f);}
 LagoonSettings_Command(LAGOON_SETTINGS_LEFT);LagoonSettings_Command(LAGOON_SETTINGS_RIGHT);LagoonSettings_Command(LAGOON_SETTINGS_UP);LagoonSettings_Command(LAGOON_SETTINGS_DOWN);
 assert(!travel_confirming&&!LagoonSettings_TakeReturnRequest());
 LagoonSettings_Command(LAGOON_SETTINGS_ACTIVATE);assert(travel_confirming&&opened&&!LagoonSettings_TakeReturnRequest());
 LagoonSettings_SetTravelStatus("Ready to return to the Gold Cave entrance.",1);assert(travel_confirming); /* a refresh is not a new eligibility decision */
 assert(LagoonSettings_Image(&pixels,&width,&height));
 if(argc>2){char out[1024];snprintf(out,sizeof out,"%s.confirm.ppm",argv[2]);FILE*f=fopen(out,"wb");assert(f);fprintf(f,"P6\n%d %d\n255\n",width,height);for(int i=0;i<width*height;i++){unsigned c=pixels[i];fputc((c>>16)&255,f);fputc((c>>8)&255,f);fputc(c&255,f);}fclose(f);}
 LagoonSettings_Command(LAGOON_SETTINGS_CLOSE);assert(opened&&!travel_confirming&&!LagoonSettings_TakeReturnRequest());
 LagoonSettings_Command(LAGOON_SETTINGS_ACTIVATE);assert(travel_confirming);
 LagoonSettings_Command(LAGOON_SETTINGS_NEXT_TAB);assert(active_tab==TAB_SOUND&&!travel_confirming);
 LagoonSettings_Command(LAGOON_SETTINGS_PREVIOUS_TAB);assert(active_tab==TAB_TRAVEL);
 LagoonSettings_Command(LAGOON_SETTINGS_ACTIVATE);assert(travel_confirming);
 LagoonSettings_SetTravelStatus("Return is unavailable during a boss battle.",0);assert(!travel_available&&!travel_confirming);
 LagoonSettings_Command(LAGOON_SETTINGS_ACTIVATE);assert(opened&&!LagoonSettings_TakeReturnRequest());
 LagoonSettings_SetTravelStatus("Ready to return to the Gold Cave entrance.",1);LagoonSettings_Command(LAGOON_SETTINGS_ACTIVATE);assert(travel_confirming);
 LagoonSettings_SetTravelStatus("Ready to return to the castle entrance.",1);assert(!travel_confirming); /* changed destination must be confirmed afresh */
 LagoonSettings_PollOpen(SNES_PAD_A);LagoonSettings_Tick(SNES_PAD_A,2000);assert(!travel_confirming); /* held opener */
 LagoonSettings_Tick(0,2010);LagoonSettings_Tick(SNES_PAD_A,2020);assert(travel_confirming&&opened);
 LagoonSettings_Tick(SNES_PAD_A,3000);assert(travel_confirming&&opened&&!LagoonSettings_TakeReturnRequest());
 LagoonSettings_Tick(0,3010);LagoonSettings_Tick(SNES_PAD_B,3020);assert(opened&&!travel_confirming);
 LagoonSettings_Tick(SNES_PAD_B,4000);assert(opened); /* holding cancel must not also close */
 LagoonSettings_Tick(0,4010);LagoonSettings_Tick(SNES_PAD_A,4020);assert(travel_confirming&&opened);
 LagoonSettings_Tick(0,4030);LagoonSettings_Tick(SNES_PAD_A,4040);assert(!opened&&!travel_confirming);
 assert(LagoonSettings_TakeReturnRequest()&&!LagoonSettings_TakeReturnRequest()&&!LagoonSettings_TakeAchievementsRequest());
 assert(music==old_music&&master==old_master&&master_calls==old_calls&&!memcmp(source,source_before,sizeof source));
 result=file_read(path,&n);assert(!strcmp(result,unchanged));free(result);free(unchanged);
 char long_message[600];memset(long_message,'a',sizeof long_message);long_message[2]='\n';long_message[sizeof long_message-1]=0;
 LagoonSettings_SetTravelStatus(long_message,1);assert(strlen(travel_status)==sizeof travel_status-1&&travel_status[2]==' ');
 LagoonSettings_Open();LagoonSettings_Command(LAGOON_SETTINGS_PREVIOUS_TAB);assert(LagoonSettings_Image(&pixels,&width,&height));
 LagoonSettings_Command(LAGOON_SETTINGS_ACTIVATE);LagoonSettings_Command(LAGOON_SETTINGS_ACTIVATE);assert(!opened);
 LagoonSettings_Open();assert(!LagoonSettings_TakeReturnRequest()); /* a fresh overlay cannot replay an old request */
 LagoonSettings_SetTravelStatus(NULL,0);assert(!travel_available&&!strcmp(travel_status,"Return is unavailable here."));
 LagoonSettings_Configure(path);assert(!travel_available&&!travel_confirming&&!travel_status[0]&&!LagoonSettings_TakeReturnRequest());
 puts("PASS settings overlay: master/music5steps, heldrepeat, opener latch, close, limits; targeted atomic UTF8/BOM/newline INI persistence, unknownfields, duplicate/malformed preservation; frozen640x360 image; one-shot browser request, heldopener protection and unchanged volume/settings");
 puts("PASS Sound/Journal/Items/Display/Travel tabs: shoulder/page commands, held shoulder latch, 15/25-item wrapping/zero-item fallback, copied context/invalidation; journal/item fixtures are not progression evidence");
 puts("PASS Display sword cue: default OFF, no view/navigation writes, deliberate once-per-input toggles/held repeat, persistent reload, BOM/unknown-key preservation, independent Sound volume key, malformed/duplicate preservation with session-only warning");
 puts("PASS Travel UI: copied/bounded status, unavailable refusal, separate confirmation, cancel/tab/status invalidation, held-input guards, one-shot request; no guest/preference/audio writes; live travel validation is the host's responsibility");
 return 0;
}
