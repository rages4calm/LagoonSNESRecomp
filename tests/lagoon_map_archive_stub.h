/* Legacy explored-map model fixtures deliberately have no terrain source.
 * Real archive serialization and terrain are covered by their own source tests. */
int LagoonSceneRenderer_ArchiveOnRomInit(const uint8_t *rom,size_t size){(void)rom;(void)size;return 0;}
int LagoonSceneRenderer_ArchiveLoad(const char *path){(void)path;return 0;}
int LagoonSceneRenderer_ArchiveSave(const char *path){(void)path;return 0;}
unsigned LagoonSceneRenderer_ArchivedRoomCount(void){return 0;}
int LagoonSceneRenderer_VisitedMapImage(unsigned room,const uint32_t **p,unsigned*w,unsigned*h,unsigned*ww,unsigned*wh){return LagoonSceneRenderer_MapImage(room,0,p,w,h,ww,wh);}
