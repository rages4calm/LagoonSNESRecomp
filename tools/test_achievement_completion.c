/* Focused completion-to-storage integration using the original external ROM,
 * an observed private WRAM dump and SYNTHETIC credits CPU boundaries. This is
 * not a final-boss victory or credits-playback test. Never pass an owner profile. */
#include "lagoon_achievement_ending.h"
#include "lagoon_achievements.h"
#include "cpu_state.h"
#include "cJSON.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

static uint8_t rom[0x100000],ram[0x20000],before[sizeof ram];
static CpuState cpu;
static unsigned checks;
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL line%d: %s\n",__LINE__,#x);exit(1);}checks++;}while(0)
static void read_bytes(const char *name,void *bytes,size_t size){
  FILE*f=fopen(name,"rb");CHECK(f);CHECK(fread(bytes,1,size,f)==size);CHECK(fgetc(f)==EOF);CHECK(!fclose(f));
}
static char *read_text(const char *name){
  FILE*f=fopen(name,"rb");CHECK(f);CHECK(!fseek(f,0,SEEK_END));long n=ftell(f);CHECK(n>=0&&n<32768);rewind(f);
  char *s=malloc((size_t)n+1);CHECK(s);CHECK(fread(s,1,(size_t)n,f)==(size_t)n);s[n]=0;CHECK(!fclose(f));return s;
}
static int ending(unsigned pc){
  CpuState original=cpu;memcpy(before,ram,sizeof ram);
  int complete=LagoonAchievementEnding_Observe(&cpu,pc,1);
  CHECK(!memcmp(&original,&cpu,sizeof cpu));CHECK(!memcmp(before,ram,sizeof ram));return complete;
}
static size_t ending_index(void){
  for(size_t i=0;i<LagoonAchievements_Count();i++){LagoonAchievementRow row;CHECK(LagoonAchievements_GetRow(i,&row));if(!strcmp(row.id,"story.finished"))return i;}
  CHECK(0);return 0;
}
int main(int argc,char **argv){
  CHECK(argc==4); /* ROM, private observed WRAM, NEW absolute scratch profile. */
  CHECK(strstr(argv[3],"/work-native-batch54/achievements/"));
  CHECK(GetFileAttributesA(argv[3])==INVALID_FILE_ATTRIBUTES);
  read_bytes(argv[1],rom,sizeof rom);read_bytes(argv[2],ram,sizeof ram);
  CHECK(ram[0x109a]==208); /* Actual palette phase in the reported owner's states. */
  CHECK(LagoonAchievements_Init(argv[3],false));CHECK(LagoonAchievements_Count()==26);
  LagoonAchievements_OnLoaded(1);memcpy(before,ram,sizeof ram);
  LagoonAchievements_Observe(ram,sizeof ram,1,true);CHECK(!memcmp(before,ram,sizeof ram));
  CHECK(LagoonAchievements_WaitIdle(5000));CHECK(!LagoonAchievements_Unlocked());
  size_t index=ending_index(),notice=0;CHECK(!LagoonAchievements_PopNotice(&notice));
  /* Explicit synthetic CPU/counter/stack fixture. No guest execution or quest
   * outcome is fabricated by the modules under test; this test supplies them. */
  memset(&cpu,0,sizeof cpu);cpu.ram=ram;cpu.P=CPU_P_M;cpu.m_flag=1;
  cpu.PB=2;cpu.DB=2;cpu.S=0x1ef0;ram[cpu.S+1]=0x3c;ram[cpu.S+2]=0xde;ram[cpu.S+3]=2;
  LagoonAchievementEnding_Init(rom,sizeof rom);LagoonAchievementEnding_Reset(1);
  ram[0x1099]=7;CHECK(!ending(0x02e9bd)); /* A terminal snapshot alone is silent. */
  ram[0x1099]=0;CHECK(!ending(0x02e99d));ram[0x1099]=7;
  CHECK(ending(0x02e9bd));CHECK(ram[0x109a]==208);
  memcpy(before,ram,sizeof ram);LagoonAchievements_CompletedEnding(1);
  CHECK(LagoonAchievements_WaitIdle(5000));CHECK(!memcmp(before,ram,sizeof ram));
  LagoonAchievementRow row;CHECK(LagoonAchievements_GetRow(index,&row));
  CHECK(row.progress==1&&row.unlocked_at);uint64_t when=row.unlocked_at;
  CHECK(LagoonAchievements_Unlocked()==1);CHECK(LagoonAchievements_PopNotice(&notice)&&notice==index);
  CHECK(!LagoonAchievements_PopNotice(&notice));
  char profile[4096];CHECK(strlen(LagoonAchievements_ProfilePath())<sizeof profile);strcpy(profile,LagoonAchievements_ProfilePath());
  char *committed=read_text(profile);cJSON *document=cJSON_Parse(committed);CHECK(document);
  cJSON *rows=cJSON_GetObjectItemCaseSensitive(document,"achievements");CHECK(cJSON_IsArray(rows));unsigned earned=0,finished=0;
  for(cJSON *item=rows->child;item;item=item->next){
    cJSON *id=cJSON_GetObjectItemCaseSensitive(item,"id"),*progress=cJSON_GetObjectItemCaseSensitive(item,"progress"),*time=cJSON_GetObjectItemCaseSensitive(item,"unlockedAt");
    CHECK(cJSON_IsString(id)&&cJSON_IsNumber(progress)&&cJSON_IsNumber(time));
    earned+=time->valuedouble!=0;
    if(!strcmp(id->valuestring,"story.finished")){CHECK(progress->valuedouble==1&&time->valuedouble==(double)when);finished++;}
  }
  CHECK(earned==1&&finished==1);cJSON_Delete(document);
  CHECK(!ending(0x02e9bd));LagoonAchievements_CompletedEnding(1);
  CHECK(LagoonAchievements_WaitIdle(5000));CHECK(!LagoonAchievements_PopNotice(&notice));
  char *unchanged=read_text(profile);CHECK(!strcmp(committed,unchanged));free(unchanged);
  LagoonAchievements_Shutdown();CHECK(LagoonAchievements_Init(argv[3],false));
  CHECK(LagoonAchievements_GetRow(index,&row)&&row.progress==1&&row.unlocked_at==when);
  CHECK(LagoonAchievements_Unlocked()==1&&!LagoonAchievements_PopNotice(&notice));
  LagoonAchievements_Shutdown();free(committed);
  printf("PASS %u checks: original-source ending witness with actual observed109A=208 -> real asynchronous CompanionAchievements atomic JSON commit -> exactly one completion notice; repeat/restore silent, RAM/CPU unchanged; synthetic credits boundaries, not game playback\n",checks);
  return 0;
}
