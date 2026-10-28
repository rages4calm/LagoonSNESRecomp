/* Target-host check for the actual POSIX persistence/locking branch. The caller
 * supplies a disposable absolute directory; never point this at owner saves. */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#include "lagoon_achievements.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define CHECK(v) do { if (!(v)) { fprintf(stderr,"FAIL line%d: %s\n",__LINE__,#v); exit(1); } } while (0)

static uint8_t ram[0x20000];
static LagoonAchievementRow level_two(void) {
  LagoonAchievementRow row={0};
  for(size_t i=0;i<LagoonAchievements_Count();i++) {
    CHECK(LagoonAchievements_GetRow(i,&row));
    if(!strcmp(row.id,"level.2"))return row;
  }
  CHECK(0);return row;
}

int main(int argc,char **argv) {
  CHECK(argc==3&&argv[2][0]=='/');
  if(!strcmp(argv[1],"locked")) {
    CHECK(!LagoonAchievements_Init(argv[2],false));
    CHECK(strstr(LagoonAchievements_Status(),"Another game copy"));
    LagoonAchievements_Shutdown();return 0;
  }
  CHECK(!strcmp(argv[1],"run"));
  /* A backslash is one valid Unix filename character, not a separator. This
   * also proves the corrected parent walker reaches the actual leaf. */
  char root[4096];
  CHECK(snprintf(root,sizeof root,"%s/posix\\profile",argv[2])<(int)sizeof root);
  struct stat st;CHECK(lstat(root,&st)!=0);CHECK(mkdir(root,0700)==0);
  CHECK(LagoonAchievements_Init(root,false));
  CHECK(strstr(LagoonAchievements_ProfilePath(),"posix\\profile/Achievements/"));
  CHECK(!level_two().unlocked_at);
  pid_t child=fork();CHECK(child>=0);
  if(!child){execl(argv[0],argv[0],"locked",root,(char*)NULL);_exit(127);}
  int status;CHECK(waitpid(child,&status,0)==child&&WIFEXITED(status)&&WEXITSTATUS(status)==0);
  ram[0x500]=0xc0;ram[0x520]=18;ram[0x52c]=1;
  LagoonAchievements_Observe(ram,sizeof ram,1,true);
  ram[0x52c]=2;LagoonAchievements_Observe(ram,sizeof ram,1,true);
  CHECK(LagoonAchievements_WaitIdle(5000));
  uint64_t unlocked=level_two().unlocked_at;CHECK(unlocked);
  char profile[4096];CHECK(strlen(LagoonAchievements_ProfilePath())<sizeof profile);
  strcpy(profile,LagoonAchievements_ProfilePath());
  CHECK(lstat(profile,&st)==0&&S_ISREG(st.st_mode));
  LagoonAchievements_Shutdown();
  CHECK(LagoonAchievements_Init(root,false));CHECK(level_two().unlocked_at==unlocked);
  size_t notice;CHECK(!LagoonAchievements_PopNotice(&notice));
  LagoonAchievements_Shutdown();
  puts("PASS: POSIX achievement lock, worker commit/reload, silent restore and backslash filename.");
  return 0;
}
#else
#include <stdio.h>
int main(void) {fputs("SKIP: POSIX runtime test requires Linux/macOS.\n",stderr);return 77;}
#endif
