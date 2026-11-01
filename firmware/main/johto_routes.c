#include "johto_routes.h"
#include "assets.h"
#include "battle.h"
#include <limits.h>
#include <string.h>
static const johto_path_t PATHS[8][3]={
 {{"Grass Path",{161,165,179,191}},{"Treetops",{163,167,187,173}},{"Pond Bank",{194,172,174,183}}},
 {{"Trunk Survey",{165,166,167,168}},{"Canopy Walk",{177,187,188,193}},{"Deep Woods",{190,204,206,214}}},
 {{"Flower Trail",{189,191,192,185}},{"Ranch Fence",{179,180,209,241}},{"Riverside Grass",{183,184,216,231}}},
 {{"Old Tree Shade",{177,178,198,200}},{"Stone Passage",{202,203,207,213}},{"Ruins Camp",{234,235,236,203}}},
 {{"Charred Hall",{156,162,164,181}},{"Tower Puddles",{195,197,205,210}},{"Deep Embers",{215,218,228,229}}},
 {{"Snowfield Tracks",{215,217,220,221}},{"Frozen Channel",{225,238,224,232}},{"Rock Cave",{239,240,246,247}}},
 {{"Shoal Weeds",{158,159,184,195}},{"Coral Bay",{170,211,222,223}},{"Tidal Depths",{171,224,226,230}}},
 {{"Tower Courtyard",{154,157,160,182}},{"Moonlit Hall",{169,186,196,199}},{"Tower Gallery",{208,212,227,248}}}
};
static uint32_t route_mix(uint32_t x){x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;return x^(x>>16);}
const johto_path_t *johto_path(unsigned route,unsigned path){return route<8&&path<3?&PATHS[route][path]:NULL;}
// Keep progress useful even after the historical 16-bit research counter saturates.
static uint32_t visits(const johto_state_t *s,unsigned route){return s->research[route]==UINT16_MAX?s->explorations:s->research[route];}
unsigned johto_track_clues(const johto_state_t *s,unsigned route){return route<8?visits(s,route)%4:0;}
unsigned johto_track_target(const johto_state_t *s,unsigned route){
 if(route>=8)return 0;
 const johto_route_t *r=&JOHTO_ROUTES[route];return r->species[8+(visits(s,route)/4+route)%4];
}
static unsigned rarity(unsigned id){species_t sp;if(!assets_species(id,&sp))return 1;unsigned total=sp.hp+sp.attack+sp.defense+sp.special+sp.speed;return total<250?1:total<330?2:total<400?3:total<470?4:5;}
bool johto_search_apply(save_t *s,unsigned route,unsigned path,uint32_t seed,johto_search_t *out){
 const johto_path_t *p=johto_path(route,path);
 if(!s||!out||!p||!johto_route_open(&s->johto,route)||s->pet.stamina<NURT_EXPLORE_COST||s->johto.last_action==UINT32_MAX)return false;
 unsigned clues=johto_track_clues(&s->johto,route),target=johto_track_target(&s->johto,route);
 if(!johto_explore(&s->johto,route,s->johto.last_action+1,true))return false;
 johto_story_explore(&s->story,&s->johto,route);s->pet.stamina-=NURT_EXPLORE_COST;
 uint32_t roll=route_mix(seed^s->johto.explorations*0x9e3779b9u);
 *out=(johto_search_t){.route=route,.path=path,.target=target,.clues=johto_track_clues(&s->johto,route),.item=ITEM_NONE};
 if(clues==3||roll%100<60){
  unsigned species=target;
  if(clues!=3){unsigned total=0,weights[4];for(unsigned i=0;i<4;i++){weights[i]=(unsigned[]){0,40,28,16,7,2}[rarity(p->species[i])];total+=weights[i];}unsigned pick=route_mix(roll)%total;for(unsigned i=0;i<4;i++){if(pick<weights[i]){species=p->species[i];break;}pick-=weights[i];}}
  encounter_t e={.species_id=species,.rarity=rarity(species),.hp_ratio=100,.level=johto_route_level(route,s->level,roll>>16),.activity=ENC_ACTIVITY_EXPLORATION,.ts=s->johto.explorations,.is_shiny=enc_shiny_from_roll(route_mix(roll^0x619ab3e5),SHINY_EXPLORATION)};
  enc_queue_push(&s->queue,&e);e=s->queue.items[s->queue.count-1];
  exploration_event_t bonus={.uid=e.uid,.chain_roll=route_mix(roll^0xab31e059)};exploration_chain_discovery(&bonus,&s->queue,&s->dex,s->exploration_wins);
  e.is_shiny=bonus.shiny;out->encounter=e;out->kind=clues==3?JOHTO_SEARCH_TARGET:JOHTO_SEARCH_ENCOUNTER;
 }else{
  out->kind=JOHTO_SEARCH_CLUE;
  // Clue stops supply food and better balls; full stacks use the existing overflow rule.
  unsigned item=(unsigned[]){ITEM_BERRY,ITEM_BERRY,ITEM_MILK,ITEM_GREAT,ITEM_JOY_COOKIE}[route_mix(roll)%5];
  item_loot_t loot={.item_id=item,.quantity=item==ITEM_BERRY?2:1};loot=items_fit_loot(loot,&s->inventory,route_mix(roll^7));s->inventory.quantity[loot.item_id]+=loot.quantity;
  out->item=loot.item_id;out->quantity=loot.quantity;out->item_full=loot.full;
 }
 return true;
}

// Each entry consumes one existing evidence step. Old completed steps remain
// completed, including saves made before these preparation requirements existed.
static const johto_probe_t PROBES[JOHTO_PROBE_COUNT]={
 {0,0,4,0,TY_ELECTRIC,TY_NONE,"Sparks in Charred Wood",{"Burnt posts still glimmer.","Hair prickles as you near."},"Electric partners reply.","Link the glimmers up.","Sparks run the grain!","Lightning marks point on.","Check the fence ground."},
 {0,1,2,1,TY_GROUND,TY_NONE,"Tremors Under the Fence",{"Deep prints by the fence.","The soil trembles faintly."},"Ask an earth-wise partner.","It's underfoot, not wind.","Partner crouches to feel.","Tracks lead to Icy Path.","The cave seems to echo."},
 {0,2,5,2,TY_GROUND,TY_ROCK,"Echo in the Rock Wall",{"Snow prints end at a wall.","A low echo from within."},"Find an echo-wise partner.","Tap the wall. It may reply","Partner taps. Echo rings.","Three marks show thunder","Back to Notes: thunder."},
 {1,0,4,2,TY_FIRE,TY_NONE,"Embers Still Warm",{"The charcoal is cold.","Only deep ash stays warm."},"A fireproof partner nears.","Warmth needs another fire.","Partner stirs the ash.","Warm tracks: Ilex Woods.","Seek warm wind in woods."},
 {1,1,1,2,TY_FIRE,TY_NONE,"Warm Wind in the Woods",{"Deeper, grass is drier.","Heat drifts without wind."},"A fire-wise partner knows.","Follow the warmth.","Partner answers the heat.","Tracks lead to Bell Tower.","Win the tower's badge."},
 {1,2,7,0,TY_FIRE,TY_NONE,"Warmth in the Courtyard",{"Ash rings the steps.","Stone near plants is warm."},"Let a flame partner try.","The steps await a spark.","Sparks glow on the steps.","Three warm spots align.","Back to Notes: the scorch."},
 {2,0,2,2,TY_WATER,TY_NONE,"Upstream Ripples",{"Riverbank grass is wet.","A clear ripple goes up."},"A water partner follows.","Against the flow, not wind","Partner wades in after it.","The stream leads to Whirl.","Win the isles' badge."},
 {2,1,6,0,TY_WATER,TY_GRASS,"Spring in the Shoals",{"Weeds sway after the tide.","Roots block the spring."},"Ask a water/grass partner.","The spring may be in roots","Partner finds the spring.","The blockage is now clear.","To Notes: Water Trial."},
 {3,0,6,0,TY_WATER,TY_NONE,"Tide Lines in the Wing",{"The Wing shows tidelines.","Shoal weeds sway one way."},"A tide-wise partner knows.","Follow weeds, watch it.","Partner meets the tide.","Line one: Coral Bay.","Lights flicker in a row."},
 {3,1,6,1,TY_ELECTRIC,TY_NONE,"Lights in the Coral",{"The lights blink in time.","As if waiting for a reply."},"Let a sparker reply.","They want the same rhythm.","Sparks link the lights.","Tideline 2 points deeper.","Bring a wave partner."},
 {3,2,6,2,TY_WATER,TY_FLYING,"Where Wind Meets Tide",{"The deepest waves part.","Wind lifts the Wing."},"Fliers or swimmers reach.","Waves or wind both work.","Partner crosses. It rings.","Three lines meet at sea.","Back to Notes: the sea."},
 {4,0,7,0,TY_FIRE,TY_NONE,"Dawn on the Mural",{"The mural's sun is dim.","Three figures ring a flame"},"Let a fire partner light.","It lacks one warm light.","Fire lights the first part","A moon glyph shows by sun.","Read it in Moonlit Hall."},
 {4,1,7,1,TY_PSYCHIC,TY_GHOST,"The Silent Bell",{"The bells hang still.","Yet wall shadows sway."},"Ask a ghost-sense partner.","The chime hides from view.","Partner stares. It rings.","Moon shadows point up.","Bring a flier up."},
 {4,2,7,2,TY_FLYING,TY_NONE,"Rainbow Light at the Top",{"Wind carries the bell far.","Rainbow light rests above."},"A wind-rider may reach it.","It's high. Follow wind in.","Partner flies. Light falls","Mural and notes agree.","Back to Notes: the rainbow"},
 {5,0,1,0,TY_GRASS,TY_NONE,"Two Rings of the Old Tree",{"Two growth rings overlap.","Buds echo Kanto's forest."},"A grass partner hears it.","Both rings await a reply.","Buds open to your partner.","The echo runs into woods.","Find where shadow lags."},
 {5,1,1,2,TY_PSYCHIC,TY_GHOST,"The Shadow a Step Late",{"Tree shadows lag a step.","Steps stop, shadows follow"},"Ask a ghost-sense partner.","Shadows lag in time.","Partner stares. Time blurs","Two moments meet up high.","Bring a grass partner up."},
 {5,2,1,1,TY_GRASS,TY_NONE,"Buds of Two Moments",{"Treetops hold new and old.","Echoes drift in leaves."},"Let a grass partner reply.","Leaves await resonance.","Buds wake. Woods brighten.","The two moments align.","Back to Notes: the echo."},
};
const johto_probe_t *johto_probe(unsigned id){return id<JOHTO_PROBE_COUNT?&PROBES[id]:NULL;}
bool johto_probe_available(const johto_state_t *j,const johto_story_t *s,bool feather,unsigned id){
 const johto_probe_t *p=johto_probe(id);
 if(!p||!johto_route_open(j,p->route)||!s->tower_seen||j->legend_clues[p->legend]!=p->step)return false;
 if(p->legend==3&&(!feather||johto_badges(j)<6))return false;
 if(p->legend==4&&(johto_badges(j)<8||j->legend_clues[0]<3||j->legend_clues[1]<3||j->legend_clues[2]<3))return false;
 if(p->legend==5&&(j->phase!=JOHTO_COMPLETE||!(s->forest_visits&1)))return false;
 return true;
}
unsigned johto_probe_next(const johto_state_t *j,const johto_story_t *s,bool feather,unsigned route,unsigned focus){
 for(unsigned n=0;n<6;n++){unsigned legend=n<3?(n+focus%3)%3:n;
  for(unsigned id=0;id<JOHTO_PROBE_COUNT;id++)if(PROBES[id].legend==legend&&PROBES[id].route==route&&johto_probe_available(j,s,feather,id))return id;
 }return JOHTO_PROBE_NONE;
}
bool johto_probe_member(unsigned id,const mon_t *m){
 const johto_probe_t *p=johto_probe(id);species_t sp;
 if(!p||!m||!m->species_id||!m->hp||!assets_species(m->species_id,&sp))return false;
 return sp.type1==p->type1||sp.type2==p->type1||(p->type2!=TY_NONE&&(sp.type1==p->type2||sp.type2==p->type2));
}
unsigned johto_probe_apply(save_t *s,unsigned id,unsigned path,unsigned slot,const mon_t *expected){
 const johto_probe_t *p=johto_probe(id);
 if(!s||!p||!johto_probe_available(&s->johto,&s->story,s->activities.silver_feather,id))return PROBE_LOCKED;
 if(path>=JOHTO_PATH_COUNT||path!=p->path)return PROBE_WRONG_PATH;
 if(!expected||slot>=s->members.party_count||memcmp(expected,&s->members.party[slot],sizeof(*expected)))return PROBE_LOCKED;
 if(!johto_probe_member(id,expected))return PROBE_WRONG_PARTNER;
 if(s->pet.stamina<NURT_EXPLORE_COST)return PROBE_NO_STAMINA;
 if(s->johto.last_action==UINT32_MAX||!johto_explore(&s->johto,p->route,s->johto.last_action+1,true))return PROBE_LOCKED;
 johto_story_explore(&s->story,&s->johto,p->route);
 s->johto.legend_clues[p->legend]++;s->pet.stamina-=NURT_EXPLORE_COST;return PROBE_OK;
}
