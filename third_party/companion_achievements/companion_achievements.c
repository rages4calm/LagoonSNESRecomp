/* SPDX-License-Identifier: MIT */
#include "companion_achievements.h"
#include <string.h>
static bool valid_id(const char *id) {
    if (!id || !*id || strlen(id)>80) return false;
    for (; *id; ++id)
        if (!((*id>='a'&&*id<='z')||(*id>='0'&&*id<='9')||*id=='.'||*id=='_'||*id=='-')) return false;
    return true;
}
bool ca_init(ca_engine *e, const ca_definition *defs, size_t count,
             ca_commit commit, ca_notify notify, void *context) {
    if (!e || !defs || !count || count>CA_MAX_ACHIEVEMENTS) return false;
    for (size_t i=0;i<count;++i) {
        if (!valid_id(defs[i].id)||!valid_id(defs[i].event)||!defs[i].title||!defs[i].description||
            !defs[i].target||(defs[i].rule!=CA_COUNTER&&defs[i].rule!=CA_MILESTONE)) return false;
        for(size_t j=0;j<i;++j) if(!strcmp(defs[i].id,defs[j].id)) return false;
    }
    memset(e,0,sizeof(*e));e->definitions=defs;e->count=count;e->commit=commit;e->notify=notify;e->context=context;
    return true;
}
bool ca_restore(ca_engine *e,const char *id,uint32_t progress,uint64_t when) {
    if(!e||!id) return false;
    for(size_t i=0;i<e->count;++i) if(!strcmp(e->definitions[i].id,id)) {
        if(progress>e->definitions[i].target||(when&&progress!=e->definitions[i].target)||
           (!when&&progress==e->definitions[i].target)) return false;
        e->states[i]=(ca_state){progress,when};return true;
    }
    return false;
}
bool ca_event(ca_engine *e,const char *event,uint32_t amount,uint64_t when) {
    if(!e||!event||!when) return false;
    ca_state next[CA_MAX_ACHIEVEMENTS];memcpy(next,e->states,sizeof(next));bool changed=false;
    for(size_t i=0;i<e->count;++i) {
        const ca_definition *d=&e->definitions[i];
        if(next[i].unlocked_at||strcmp(d->event,event)) continue;
        uint32_t progress=next[i].progress;
        if(d->rule==CA_COUNTER) {
            uint32_t remaining=d->target-progress;progress+=amount>remaining?remaining:amount;
        } else if(amount>progress) progress=amount>d->target?d->target:amount;
        if(progress==next[i].progress) continue;
        next[i].progress=progress;changed=true;
        if(progress==d->target) next[i].unlocked_at=when;
    }
    if(!changed) return true;
    if(e->commit&&!e->commit(e->context,next,e->count)) return false;
    bool unlocked[CA_MAX_ACHIEVEMENTS]={false};
    for(size_t i=0;i<e->count;++i) unlocked[i]=!e->states[i].unlocked_at&&next[i].unlocked_at;
    memcpy(e->states,next,sizeof(next));
    for(size_t i=0;i<e->count;++i) if(unlocked[i]&&e->notify) e->notify(e->context,&e->definitions[i],&e->states[i]);
    return true;
}
size_t ca_unlocked(const ca_engine *e) {
    size_t count=0;if(e)for(size_t i=0;i<e->count;++i)count+=e->states[i].unlocked_at!=0;return count;
}
