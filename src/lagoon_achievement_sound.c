/* Original, synthesized three-note notification. No game audio is extracted. */
#include "lagoon_achievement_sound.h"
#include "desktop/sdl_compat.h"
#include <math.h>
#include <stdint.h>
#include <stddef.h>
enum { RATE=48000, FRAMES=31200 };
static int16_t samples[FRAMES];
static SDL_AudioStream *stream;
static bool enabled, prepared, failed;
static void open_stream(void);
void LagoonAchievementSound_Configure(bool value) {
  enabled=value;
  if(!enabled&&stream)SDL_ClearAudioStream(stream);
  /* Called once after SDL startup, before play: opening an audio device is
   * deliberately kept out of the first unlock's gameplay frame. */
  if(enabled&&!stream&&!failed)open_stream();
}
static void prepare(void) {
  const double notes[]={523.2511306,659.2551138,783.9908720};
  for(unsigned i=0;i<FRAMES;i++) {
    double time=(double)i/RATE, value=0;
    for(unsigned note=0;note<3;note++) {
      double t=time-note*0.105;
      if(t<0||t>0.40)continue;
      double attack=t<0.008?t/0.008:1,release=(0.40-t)/0.40;
      double phase=t*notes[note];phase-=floor(phase);
      double triangle=1-4*fabs(phase-0.5);
      value+=triangle*attack*release*release*0.095;
    }
    samples[i]=(int16_t)(value*32767.0);
  }
  prepared=true;
}
static void open_stream(void) {
  if(!prepared)prepare();
  const SDL_AudioSpec spec={.format=SDL_AUDIO_S16,.channels=1,.freq=RATE};
  stream=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,NULL,NULL);
  if(!stream){failed=true;return;}
  if(!SDL_ResumeAudioStreamDevice(stream)){SDL_DestroyAudioStream(stream);stream=NULL;failed=true;}
}
void LagoonAchievementSound_SetVolume(unsigned volume) {
  if(!stream||failed)return;
  if(volume>100)volume=100;
  SDL_SetAudioStreamGain(stream,(float)volume/100.0f);
  /* Muting discards an already queued chime: unmuting cannot revive it. */
  if(!volume)SDL_ClearAudioStream(stream);
}
void LagoonAchievementSound_Play(unsigned volume) {
  if(!enabled||!stream||failed)return;
  LagoonAchievementSound_SetVolume(volume);
  if(!volume)return;
  /* A disconnected notification device cannot accumulate an unbounded queue. */
  if(SDL_GetAudioStreamQueued(stream)>(int)sizeof samples*2)SDL_ClearAudioStream(stream);
  if(SDL_PutAudioStreamData(stream,samples,sizeof samples))SDL_FlushAudioStream(stream);
}
void LagoonAchievementSound_Shutdown(void) {
  if(stream)SDL_DestroyAudioStream(stream);
  stream=NULL;enabled=false;failed=false;
}
