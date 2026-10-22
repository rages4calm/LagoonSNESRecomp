/* White-box audio regression: compile the real standalone implementation,
 * without a game runtime, SPC, DSP, guest RAM or guest sample-ring library. */
#define SDL_MAIN_HANDLED 1
#include "../src/lagoon_achievement_sound.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"FAIL line %d: %s; SDL: %s\n",__LINE__,#x,SDL_GetError());exit(1);}}while(0)
static uint64_t digest(const void*data,size_t bytes){const uint8_t*p=data;uint64_t h=UINT64_C(14695981039346656037);while(bytes--)h=(h^*p++)*UINT64_C(1099511628211);return h;}
int main(int argc,char**argv){
  CHECK(argc==3);bool real=!strcmp(argv[1],"actual");CHECK(real||!strcmp(argv[1],"dummy"));
  /* Opening without SDL audio is a normal fail-closed startup path. */
  LagoonAchievementSound_Configure(true);CHECK(!stream&&failed);LagoonAchievementSound_Play(100);LagoonAchievementSound_Shutdown();CHECK(!stream&&!enabled&&!failed);
  CHECK(SDL_SetHintWithPriority(SDL_HINT_AUDIO_DRIVER,real?"wasapi":"dummy",SDL_HINT_OVERRIDE));
  CHECK(SDL_Init(SDL_INIT_AUDIO));const char*driver=SDL_GetCurrentAudioDriver();CHECK(driver);
  LagoonAchievementSound_Configure(false);LagoonAchievementSound_Play(100);CHECK(!stream);
  LagoonAchievementSound_Configure(true);CHECK(stream&&!failed&&enabled);CHECK(SDL_PauseAudioStreamDevice(stream));
  const SDL_AudioSpec control_spec={.format=SDL_AUDIO_S16,.channels=2,.freq=32000};
  SDL_AudioStream*control=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&control_spec,NULL,NULL);CHECK(control);CHECK(control!=stream);CHECK(SDL_GetAudioStreamDevice(control)!=SDL_GetAudioStreamDevice(stream));
  int16_t control_samples[2048];for(unsigned i=0;i<2048;i++)control_samples[i]=(int16_t)(i*31);uint64_t control_digest=digest(control_samples,sizeof control_samples);
  CHECK(SDL_PutAudioStreamData(control,control_samples,sizeof control_samples));int control_queued=SDL_GetAudioStreamQueued(control);CHECK(control_queued==(int)sizeof control_samples);
  LagoonAchievementSound_Play(0);CHECK(SDL_GetAudioStreamQueued(stream)==0);
  LagoonAchievementSound_Play(25);CHECK(SDL_GetAudioStreamGain(stream)>0.249f&&SDL_GetAudioStreamGain(stream)<0.251f);CHECK(SDL_GetAudioStreamQueued(stream)==(int)sizeof samples);uint64_t sample_digest=digest(samples,sizeof samples);
  LagoonAchievementSound_Play(1000);CHECK(SDL_GetAudioStreamGain(stream)==1.0f);
  for(unsigned i=0;i<300;i++){LagoonAchievementSound_Play(50);CHECK(SDL_GetAudioStreamQueued(stream)<=3*(int)sizeof samples);}
  CHECK(SDL_GetAudioStreamQueued(control)==control_queued);CHECK(digest(control_samples,sizeof control_samples)==control_digest);CHECK(digest(samples,sizeof samples)==sample_digest);
  int queued=SDL_GetAudioStreamQueued(stream);CHECK(queued>0);LagoonAchievementSound_SetVolume(12);CHECK(SDL_GetAudioStreamQueued(stream)==queued);CHECK(SDL_GetAudioStreamGain(stream)>0.119f&&SDL_GetAudioStreamGain(stream)<0.121f);
  LagoonAchievementSound_SetVolume(0);CHECK(SDL_GetAudioStreamGain(stream)==0.0f);CHECK(SDL_GetAudioStreamQueued(stream)==0);
  LagoonAchievementSound_SetVolume(70);CHECK(SDL_GetAudioStreamQueued(stream)==0);CHECK(SDL_GetAudioStreamGain(stream)>0.699f&&SDL_GetAudioStreamGain(stream)<0.701f);
  LagoonAchievementSound_Play(25);CHECK(SDL_GetAudioStreamQueued(stream)>0);LagoonAchievementSound_Play(0);CHECK(SDL_GetAudioStreamQueued(stream)==0);
  CHECK(SDL_ClearAudioStream(stream));LagoonAchievementSound_Play(0);CHECK(SDL_GetAudioStreamQueued(stream)==0);
  LagoonAchievementSound_Play(25);CHECK(SDL_GetAudioStreamQueued(stream)>0);LagoonAchievementSound_Configure(false);CHECK(SDL_GetAudioStreamQueued(stream)==0);LagoonAchievementSound_Play(100);CHECK(SDL_GetAudioStreamQueued(stream)==0);
  LagoonAchievementSound_Configure(true);LagoonAchievementSound_Play(real?15:25);CHECK(SDL_ResumeAudioStreamDevice(stream));
  SDL_Delay(40);LagoonAchievementSound_SetVolume(8);CHECK(SDL_GetAudioStreamGain(stream)>0.079f&&SDL_GetAudioStreamGain(stream)<0.081f);
  uint64_t deadline=SDL_GetTicks()+3000;while(SDL_GetAudioStreamQueued(stream)>0&&SDL_GetTicks()<deadline)SDL_Delay(10);CHECK(SDL_GetAudioStreamQueued(stream)==0);
  CHECK(SDL_GetAudioStreamQueued(control)==control_queued);CHECK(SDL_GetAudioStreamGain(control)==1.0f);
  LagoonAchievementSound_Shutdown();CHECK(!stream&&!enabled&&!failed);SDL_DestroyAudioStream(control);SDL_Quit();
  FILE*f=fopen(argv[2],"wb");CHECK(f);fprintf(f,"{\"passed\":true,\"driver\":\"%s\",\"device\":\"%s\",\"standaloneWithoutGuestLibraries\":true,\"independentControlStreamPreserved\":true,\"queueBoundBytes\":%zu,\"sampleFrames\":%u,\"rate\":%u}\n",real?"wasapi":"dummy",real?"actual":"dummy",3*sizeof samples,FRAMES,RATE);CHECK(!fclose(f));
  printf("PASS: %s independent notification stream; disabled/mute/clamp, bounded queue, drain, failure and control-stream preservation.\n",real?"actual WASAPI":"SDL dummy");return 0;
}
