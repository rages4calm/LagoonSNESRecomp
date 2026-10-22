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
  CHECK(argc==4);bool real=!strcmp(argv[1],"actual");CHECK(real||!strcmp(argv[1],"dummy"));
  /* Opening without SDL audio is a normal fail-closed startup path. */
  LagoonAchievementSound_Configure(true);CHECK(!stream&&failed);LagoonAchievementSound_Play(100);LagoonAchievementSound_PlayLaugh(100);LagoonAchievementSound_Shutdown();CHECK(!stream&&!enabled&&!failed);
  CHECK(SDL_SetHintWithPriority(SDL_HINT_AUDIO_DRIVER,real?"wasapi":"dummy",SDL_HINT_OVERRIDE));
  CHECK(SDL_Init(SDL_INIT_AUDIO));const char*driver=SDL_GetCurrentAudioDriver();CHECK(driver);
  LagoonAchievementSound_Configure(false);LagoonAchievementSound_Play(100);LagoonAchievementSound_PlayLaugh(100);CHECK(!stream);
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
  LagoonAchievementSound_PlayLaugh(100);CHECK(SDL_GetAudioStreamQueued(stream)==0);
  LagoonAchievementSound_Configure(true);CHECK(SDL_PauseAudioStreamDevice(stream));
  /* Optional files fail closed, including a malformed RIFF whose declared
   * chunk lengths could otherwise ask a WAV decoder for a giant buffer. */
  CHECK(LagoonAchievementSound_LaughDurationMs()==0);
  CHECK(!LagoonAchievementSound_LoadLaugh(NULL));CHECK(!LagoonAchievementSound_LoadLaugh("file-that-does-not-exist.wav"));
  char invalid_path[4096];CHECK(snprintf(invalid_path,sizeof invalid_path,"%s.invalid.wav",argv[2])>0);
  uint8_t invalid[44]={0};memcpy(invalid,"RIFF",4);memcpy(invalid+8,"WAVE",4);memcpy(invalid+12,"fmt ",4);
  invalid[4]=36;invalid[16]=255;invalid[17]=255;invalid[18]=255;invalid[19]=127;
  FILE*bad=fopen(invalid_path,"wb");CHECK(bad&&fwrite(invalid,1,sizeof invalid,bad)==sizeof invalid);CHECK(!fclose(bad));
  CHECK(!LagoonAchievementSound_LoadLaugh(invalid_path));CHECK(!laugh_samples&&!laugh_bytes);
  LagoonAchievementSound_PlayLaugh(100);CHECK(SDL_GetAudioStreamQueued(stream)==0);
  CHECK(LagoonAchievementSound_LoadLaugh(argv[3]));
  CHECK(laugh_samples&&laugh_bytes>0&&laugh_bytes<=MAX_LAUGH_FRAMES*2);
  CHECK(LagoonAchievementSound_LaughDurationMs()>=2811&&LagoonAchievementSound_LaughDurationMs()<=2813);
  unsigned peak=0,nonzero=0;
  for(int i=0;i<laugh_bytes/2;i++) {
    int value=((const int16_t*)laugh_samples)[i];unsigned amplitude=(unsigned)(value<0?-value:value);
    if(amplitude>peak)peak=amplitude;if(value)nonzero++;
  }
  CHECK(peak>1000&&peak<=32768&&nonzero>10000);
  uint64_t laugh_digest=digest(laugh_samples,(size_t)laugh_bytes);
  LagoonAchievementSound_PlayLaugh(0);CHECK(SDL_GetAudioStreamQueued(stream)==0);
  LagoonAchievementSound_PlayLaugh(35);CHECK(SDL_GetAudioStreamQueued(stream)==laugh_bytes);
  CHECK(SDL_GetAudioStreamGain(stream)>0.349f&&SDL_GetAudioStreamGain(stream)<0.351f);
  for(unsigned i=0;i<300;i++){LagoonAchievementSound_PlayLaugh(50);CHECK(SDL_GetAudioStreamQueued(stream)<=3*laugh_bytes);}
  CHECK(digest(laugh_samples,(size_t)laugh_bytes)==laugh_digest);CHECK(digest(samples,sizeof samples)==sample_digest);
  CHECK(SDL_GetAudioStreamQueued(control)==control_queued);CHECK(digest(control_samples,sizeof control_samples)==control_digest);
  LagoonAchievementSound_SetVolume(0);CHECK(SDL_GetAudioStreamQueued(stream)==0);
  LagoonAchievementSound_SetVolume(70);CHECK(SDL_GetAudioStreamQueued(stream)==0);
  LagoonAchievementSound_PlayLaugh(20);CHECK(SDL_GetAudioStreamQueued(stream)>0);LagoonAchievementSound_Configure(false);CHECK(SDL_GetAudioStreamQueued(stream)==0);
  LagoonAchievementSound_Configure(true);LagoonAchievementSound_Play(real?15:25);CHECK(SDL_ResumeAudioStreamDevice(stream));
  SDL_Delay(40);LagoonAchievementSound_SetVolume(8);CHECK(SDL_GetAudioStreamGain(stream)>0.079f&&SDL_GetAudioStreamGain(stream)<0.081f);
  uint64_t deadline=SDL_GetTicks()+3000;while(SDL_GetAudioStreamQueued(stream)>0&&SDL_GetTicks()<deadline)SDL_Delay(10);CHECK(SDL_GetAudioStreamQueued(stream)==0);
  LagoonAchievementSound_PlayLaugh(real?12:25);CHECK(SDL_GetAudioStreamQueued(stream)>0);
  SDL_Delay(30);LagoonAchievementSound_SetVolume(6);CHECK(SDL_GetAudioStreamGain(stream)>0.059f&&SDL_GetAudioStreamGain(stream)<0.061f);
  deadline=SDL_GetTicks()+5000;while(SDL_GetAudioStreamQueued(stream)>0&&SDL_GetTicks()<deadline)SDL_Delay(10);CHECK(SDL_GetAudioStreamQueued(stream)==0);
  CHECK(SDL_GetAudioStreamQueued(control)==control_queued);CHECK(SDL_GetAudioStreamGain(control)==1.0f);
  unsigned laugh_duration=LagoonAchievementSound_LaughDurationMs(),laugh_frames=(unsigned)laugh_bytes/2;size_t queue_bound=3*(size_t)laugh_bytes;
  LagoonAchievementSound_Shutdown();CHECK(!stream&&!enabled&&!failed&&!laugh_samples&&!laugh_bytes);CHECK(!LagoonAchievementSound_LaughDurationMs());SDL_DestroyAudioStream(control);SDL_Quit();
  FILE*f=fopen(argv[2],"wb");CHECK(f);fprintf(f,"{\"passed\":true,\"driver\":\"%s\",\"device\":\"%s\",\"standaloneWithoutGuestLibraries\":true,\"independentControlStreamPreserved\":true,\"queueBoundBytes\":%zu,\"sampleFrames\":%u,\"laughFrames\":%u,\"laughPeak\":%u,\"laughDurationMs\":%u,\"laughOwnerSuppliedWav\":true,\"rate\":%u}\n",real?"wasapi":"dummy",real?"actual":"dummy",queue_bound,FRAMES,laugh_frames,peak,laugh_duration,RATE);CHECK(!fclose(f));
  printf("PASS: %s independent notification/laugh stream; disabled/mute/clamp, bounded queue, drain, bounded supplied WAV load/refusals, failure and control-stream preservation.\n",real?"actual WASAPI":"SDL dummy");return 0;
}
