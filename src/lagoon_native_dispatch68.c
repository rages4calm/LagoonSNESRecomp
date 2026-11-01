/* Compose read-only observations of the same real dispatcher entry. Each
 * observer authenticates its own target; the generated dispatcher still runs. */
#include "lagoon_native_dispatch68.h"
#include "lagoon_native_death_menu68.h"
#include "lagoon_native_map68.h"
#include "lagoon_native_player_intent68.h"
bool LagoonDispatch68_Entry(CpuState*c,uint16_t s,uint8_t h,uint32_t host,RecompReturn*out){
 (void)LagoonDeathMenu68_DispatchEntry(c,s,h,host,out);
 (void)LagoonMap68_DispatchEntry(c,s,h,host,out);
 (void)LagoonPlayerIntent68_DispatchEntry(c,s,h,host,out);
 return false;
}
