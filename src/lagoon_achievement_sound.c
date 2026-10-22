/* Original synthesized notification plus the optional owner-supplied WAV. */
#include "lagoon_achievement_sound.h"
#include "desktop/sdl_compat.h"
#include <math.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
enum { RATE=48000, FRAMES=31200, MAX_WAV_BYTES=2*1024*1024, MAX_LAUGH_FRAMES=RATE*8 };
static int16_t samples[FRAMES];
static uint8_t *laugh_samples;
static int laugh_bytes;
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
static unsigned le16(const uint8_t *p){return p[0]|(unsigned)p[1]<<8;}
static uint32_t le32(const uint8_t *p){return le16(p)|(uint32_t)le16(p+2)<<16;}
/* Bound every chunk before invoking a decoder. Only ordinary PCM16 mono or
 * stereo is needed for this optional effect; never trust declared WAV sizes. */
static bool valid_wav(const uint8_t *p,size_t bytes) {
  if(bytes<44||memcmp(p,"RIFF",4)||memcmp(p+8,"WAVE",4)||le32(p+4)!=bytes-8)return false;
  unsigned rate=0,align=0;size_t data=0;bool have_format=false,have_data=false;
  for(size_t at=12;at<bytes;) {
    if(bytes-at<8)return false;
    size_t chunk=le32(p+at+4),body=at+8;
    if(chunk>bytes-body||chunk+(chunk&1)>bytes-body)return false;
    if(!memcmp(p+at,"fmt ",4)) {
      if(have_format||chunk<16||le16(p+body)!=1||le16(p+body+14)!=16)return false;
      unsigned channels=le16(p+body+2);rate=le32(p+body+4);align=le16(p+body+12);
      if(channels<1||channels>2||rate<8000||rate>96000||align!=channels*2||le32(p+body+8)!=rate*align)return false;
      have_format=true;
    } else if(!memcmp(p+at,"data",4)) {
      if(have_data||!chunk)return false;have_data=true;data=chunk;
    }
    at=body+chunk+(chunk&1);
  }
  return have_format&&have_data&&data%align==0&&data/align<=rate*8;
}
bool LagoonAchievementSound_LoadLaugh(const char *path) {
  SDL_free(laugh_samples);laugh_samples=NULL;laugh_bytes=0;
  if(!path||!*path)return false;
  SDL_IOStream *file=SDL_IOFromFile(path,"rb");if(!file)return false;
  Sint64 size=SDL_GetIOSize(file);
  if(size<44||size>MAX_WAV_BYTES){SDL_CloseIO(file);return false;}
  uint8_t *encoded=SDL_malloc((size_t)size);
  if(!encoded){SDL_CloseIO(file);return false;}
  bool read=SDL_ReadIO(file,encoded,(size_t)size)==(size_t)size;SDL_CloseIO(file);
  if(!read||!valid_wav(encoded,(size_t)size)){SDL_free(encoded);return false;}
  SDL_IOStream *memory=SDL_IOFromConstMem(encoded,(size_t)size);
  SDL_AudioSpec source={0};uint8_t *decoded=NULL;Uint32 decoded_bytes=0;
  bool loaded=memory&&SDL_LoadWAV_IO(memory,true,&source,&decoded,&decoded_bytes);
  SDL_free(encoded);
  if(!loaded)return false;
  const SDL_AudioSpec destination={.format=SDL_AUDIO_S16,.channels=1,.freq=RATE};
  bool converted=SDL_ConvertAudioSamples(&source,decoded,(int)decoded_bytes,&destination,&laugh_samples,&laugh_bytes);
  SDL_free(decoded);
  if(!converted||!laugh_samples||laugh_bytes<=0||laugh_bytes%2||laugh_bytes>MAX_LAUGH_FRAMES*2) {
    SDL_free(laugh_samples);laugh_samples=NULL;laugh_bytes=0;return false;
  }
  return true;
}
unsigned LagoonAchievementSound_LaughDurationMs(void) {
  return laugh_bytes>0?((unsigned)laugh_bytes/2*1000+RATE-1)/RATE:0;
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
  /* Muting discards queued effects: unmuting cannot revive them. */
  if(!volume)SDL_ClearAudioStream(stream);
}
static void play_samples(unsigned volume,const int16_t *data,size_t bytes) {
  if(!enabled||!stream||failed||!data||!bytes)return;
  LagoonAchievementSound_SetVolume(volume);
  if(!volume)return;
  /* A disconnected notification device cannot accumulate an unbounded queue. */
  if(SDL_GetAudioStreamQueued(stream)>(int)bytes*2)SDL_ClearAudioStream(stream);
  if(SDL_PutAudioStreamData(stream,data,(int)bytes))SDL_FlushAudioStream(stream);
}
void LagoonAchievementSound_Play(unsigned volume) {
  play_samples(volume,samples,sizeof samples);
}
void LagoonAchievementSound_PlayLaugh(unsigned volume) {
  play_samples(volume,(const int16_t*)laugh_samples,(size_t)laugh_bytes);
}
void LagoonAchievementSound_Shutdown(void) {
  if(stream)SDL_DestroyAudioStream(stream);
  stream=NULL;enabled=false;failed=false;
  SDL_free(laugh_samples);laugh_samples=NULL;laugh_bytes=0;
}
