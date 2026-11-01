#pragma once
#include "johto_story.h"
#include "endgame.h"

// V31: append-only scene IDs. Reading a scene never grants an activity reward.
enum {
 N_LETTER,N_ARRIVAL,N_WELCOME,N_FIELD,N_EGG,N_BUG_INVITE,N_BUG_REPORT,
 N_RUIN_INVITE,N_RUIN_REPORT,N_TOWER_INVITE,N_TOWER_REPORT,N_LAKE_INVITE,
 N_LAKE_REPORT,N_SEA_INVITE,N_SEA_REPORT,N_ROOF,N_ELM,N_ENDING,N_FOREST,
 N_LEGEND_RAI,N_LEGEND_ENTEI,N_LEGEND_SUICUNE,N_LEGEND_LUGIA,
 N_LEGEND_HOOH,N_LEGEND_CELEBI,N_SCENE_COUNT
};
typedef struct {
 uint64_t seen;
 encounter_t pending; // Locked until an actual battle/throw starts, even if FIFO evicts it.
 uint16_t scene;       // 0 = no interrupted dialogue, otherwise scene ID + 1.
 uint8_t page,version,bug_evidence,legacy_final,focus,legend;
} narrative_t;
_Static_assert(sizeof(narrative_t)==32,"Freeze V31 narrative record");
typedef struct {
 narrative_t state;
 johto_state_t chapter;
 johto_story_t story;
 uint16_t defeated,bug_completed;
 uint8_t feather,lake,activity,battle,dungeon;
} narrative_context_t;
enum { N_OK,N_LOCKED,N_BUSY,N_SAVE_FAILED,N_WRONG,N_NO_STAMINA };
enum { N_GO_ROUTE,N_GO_GYM,N_GO_STARTER,N_GO_EGG,N_GO_BUG,N_GO_RUIN,
 N_GO_TOWER,N_GO_LAKE,N_GO_SEA,N_GO_RESCUE,N_GO_PURIFY,N_GO_LEGEND,
 N_GO_KANTO,N_GO_KANTO_ROUTE,N_GO_ACTIVITY,N_GO_BATTLE,N_GO_DUNGEON,N_GO_FREE };
typedef struct { const char *title,*place,*task; uint8_t action,index; } narrative_goal_t;
static inline unsigned narrative_pages(unsigned s){
 static const uint8_t n[N_SCENE_COUNT]={3,3,2,2,2,2,3,2,3,2,3,2,2,2,2,2,2,3,2,2,2,2,2,2,2};
 return s<N_SCENE_COUNT?n[s]:0;
}
static inline bool narrative_seen(const narrative_t *n,unsigned s){return s<N_SCENE_COUNT&&(n->seen&(UINT64_C(1)<<s));}
static inline bool narrative_valid(const narrative_t *n){
 if(!n->version){narrative_t zero={0};return !memcmp(n,&zero,sizeof(zero));}
 if(n->version!=1||n->seen>>N_SCENE_COUNT||n->scene>N_SCENE_COUNT||n->bug_evidence>1||n->legacy_final>1||n->focus>2||n->legend>6)return false;
 if(n->scene?(n->page>=narrative_pages(n->scene-1)):n->page!=0)return false;
 if(!n->legend){encounter_t zero={0};return !memcmp(&n->pending,&zero,sizeof(zero));}
 const encounter_t *e=&n->pending;
 return e->uid&&e->species_id==(uint16_t[]){243,244,245,249,250,251}[n->legend-1]&&e->level>=1&&e->level<=100&&e->rarity==5&&e->hp_ratio==100&&!e->activity&&!e->biome&&((const uint8_t*)&e->exp_granted)[0]==0&&((const uint8_t*)&e->is_transient)[0]==0&&((const uint8_t*)&e->is_shiny)[0]<=1;
}
static inline void narrative_init(narrative_t *n){memset(n,0,sizeof(*n));n->version=1;}
static inline void narrative_consume(narrative_t *n,const encounter_t *e){if(e&&n->legend&&n->pending.uid==e->uid&&n->pending.ts==e->ts&&n->pending.species_id==e->species_id&&n->pending.level==e->level&&n->pending.is_shiny==e->is_shiny){if(n->scene==N_LEGEND_RAI+n->legend){n->seen|=UINT64_C(1)<<(n->scene-1);n->scene=0;n->page=0;}memset(&n->pending,0,sizeof(n->pending));n->legend=0;}}
static inline bool narrative_final(const narrative_context_t *c){
 if(c->state.legacy_final||johto_story_final_ready(&c->story,&c->chapter,c->defeated,c->feather))return true;
 return johto_final_open(&c->chapter,c->defeated)&&c->story.tower_seen&&(c->story.ruins&1)&&c->story.rescues&&c->feather&&(c->state.bug_evidence||c->bug_completed);
}
static inline bool narrative_allowed(const narrative_context_t *c,unsigned s){
 unsigned b=johto_badges(&c->chapter);
 if(s>=N_SCENE_COUNT||c->chapter.phase==JOHTO_HIDDEN)return false;
 if(s==N_LETTER)return true;
 if(s==N_ARRIVAL)return c->chapter.phase>=JOHTO_VOYAGE;
 if(c->chapter.phase<JOHTO_ACTIVE)return false;
 switch(s){
 case N_WELCOME:return true;
 case N_FIELD:return c->chapter.research[0]||b;
 case N_EGG:return b>=1;
 case N_BUG_INVITE:return b>=2;
 case N_BUG_REPORT:return c->state.bug_evidence||c->bug_completed;
 case N_RUIN_INVITE:return b>=3;
 case N_RUIN_REPORT:return c->story.ruins&1;
 case N_TOWER_INVITE:return b>=4;
 case N_TOWER_REPORT:return c->story.tower_seen;
 case N_LAKE_INVITE:return b>=5;
 case N_LAKE_REPORT:return c->lake==LAKE_RECEIVED;
 case N_SEA_INVITE:return b>=6;
 case N_SEA_REPORT:return c->feather;
 case N_ROOF:return b>=8&&c->chapter.legend_clues[0]==3&&c->chapter.legend_clues[1]==3&&c->chapter.legend_clues[2]==3;
 case N_ELM:return narrative_final(c);
 case N_ENDING:case N_FOREST:return c->chapter.phase==JOHTO_COMPLETE;
 default:return c->state.legend==s-N_LEGEND_RAI+1||(c->story.legend_visits&(1u<<(s-N_LEGEND_RAI)));
 }
}
// The world invokes this only on a transaction candidate under its save lock.
static inline unsigned narrative_begin(narrative_context_t *c,unsigned s){
 if(!narrative_allowed(c,s))return N_LOCKED;
 if(c->activity||c->battle||c->dungeon)return N_BUSY;
 if(c->state.scene==s+1)return N_OK;
 c->state.version=1;c->state.scene=s+1;c->state.page=0;return N_OK;
}
static inline unsigned narrative_advance(narrative_context_t *c,unsigned s,unsigned page,unsigned answer){
 if(c->state.scene!=s+1||c->state.page!=page||!narrative_allowed(c,s))return N_LOCKED;
 if(c->activity||c->battle||c->dungeon)return N_BUSY;
 if(page+1==narrative_pages(s)){
  if((s==N_BUG_REPORT&&answer!=1)||(s==N_RUIN_REPORT&&answer!=2))return N_WRONG;
  if(s==N_TOWER_REPORT){if(answer>2)return N_LOCKED;c->state.focus=answer;}
  c->state.seen|=UINT64_C(1)<<s;c->state.scene=0;c->state.page=0;
 }else c->state.page++;
 return N_OK;
}
// An explicit next destination is always available, including locked-route prerequisites.
static inline narrative_goal_t narrative_goal(const narrative_context_t *c){
 #define GO(t,p,d,a,i) return (narrative_goal_t){t,p,d,a,i}
 const johto_state_t *j=&c->chapter;const johto_story_t *s=&c->story;unsigned b=johto_badges(j);
 if(c->battle)GO("Continue the battle","Battle site","Finish it, then go on",N_GO_BATTLE,0);
 if(c->activity)GO("Survey in progress","Johto events","Bring back this record",N_GO_ACTIVITY,c->activity);
 if(c->dungeon)GO("Continue the dungeon","Dungeon camp","Story resumes afterwards",N_GO_DUNGEON,0);
 if(c->state.legend)GO("A legend is near","Survey site","A locked encounter awaits",N_GO_LEGEND,c->state.legend-1);
 if(j->phase<JOHTO_ACTIVE)GO("Reach Johto","Elm's Lab","Pick a companion",N_GO_STARTER,0);
 if(j->phase==JOHTO_COMPLETE){
  if(!(s->forest_visits&1))GO("Echoes in the Forest","Any Kanto explore map","Survey a familiar place",N_GO_KANTO_ROUTE,0);
  if(j->legend_clues[5]<3)GO("The Layered Forest","Ilex Woods","Follow the trees' echoes",N_GO_ROUTE,1);
  if(!(s->legend_visits&(1u<<5)))GO("Across Time","Ilex Woods","Go where echoes meet",N_GO_LEGEND,5);
  GO("The journey goes on","All of Johto","Free roam and re-surveys",N_GO_FREE,0);
 }
 if(c->state.legacy_final)GO("The Last Study","Elm's Lab","Old unlocks still hold",N_GO_GYM,8);
 if(!b){if(!j->research[0])GO("New survey record","Sprout Fields","Watch a migrating Pokemon",N_GO_ROUTE,0);GO("Surveyor's Prep","Falkner's Gym","Win the first Johto badge",N_GO_GYM,0);}
 if(b<2)GO("Invitation from the Grove","Bugsy's Gym","Win the 2nd Johto badge",N_GO_GYM,1);
 if(!c->state.bug_evidence&&!c->bug_completed)GO("Why the Swarm Detours","Bug Survey","Visit all six sites",N_GO_BUG,0);
 if(b<3)GO("Head to the Ruins","Whitney's Gym","Win the 3rd Johto badge",N_GO_GYM,2);
 if(!(s->ruins&1))GO("Marks on the Wall","Alph Ruins","Read the first symbols",N_GO_RUIN,0);
 if(!s->rescues){if(johto_rescue_ready(s,j)||s->rescue_step)GO("A Cry from the Field","Goldenrod Rescue","Follow tracks, bring it",N_GO_RESCUE,0);GO("Find the Lost Pokemon","Goldenrod Fields","Keep tracing footprints",N_GO_ROUTE,2);}
 if(b<4)GO("Shadow in the Tower","Morty's Gym","Win the 4th Johto badge",N_GO_GYM,3);
 if(!s->tower_seen)GO("Three Figures","Burned Tower","Log the tower's visions",N_GO_TOWER,0);
 if(b>=5&&c->lake!=LAKE_RECEIVED)GO("Red Shadow on the Lake","Lake of Rage","Trace the signal's scale",N_GO_LAKE,0);
 if(b>=6&&!c->feather)GO("Call from the Sea","Whirl Deep Expedition","Get the Silver Wing below",N_GO_SEA,0);
 for(unsigned n=0;n<3;n++){
  unsigned id=(c->state.focus+n)%3,clue=j->legend_clues[id];
  if(clue==3){if(!(s->legend_visits&(1u<<id)))GO("End of the Trail","Legend survey site","Close. Return can wait",N_GO_LEGEND,id);continue;}
  if(id==2&&clue==2)GO("Make the Water Flow Again","Suicune Water Trial","Find what blocks the flow",N_GO_PURIFY,0);
  unsigned route=id==0?(unsigned[]){4,2,5}[clue]:id==1?(unsigned[]){4,1,7}[clue]:(unsigned[]){2,6}[clue];
  if(!johto_route_open(j,route))GO("Toward the next clue",JOHTO_TRAINERS[b].name,"Win the next badge first",N_GO_GYM,b);
  GO(id==0?"Chase the Thunder":id==1?"Chase the Scorch Marks":"Chase the Clear Water",JOHTO_ROUTES[route].name,id==0?"Find the struck tree":id==1?"Spot the warm footprints":"Find the clear spring",N_GO_ROUTE,route);
 }
 if(b<8)GO("Permission at the Top",JOHTO_TRAINERS[b].name,"Win the next Johto badge",N_GO_GYM,b);
 if(j->legend_clues[3]<3)GO("The Silver Wing Answers","Whirl Islands","Follow the sea wind",N_GO_ROUTE,6);
 if(!(s->legend_visits&(1u<<3)))GO("The Silver Wing Answers","Whirl Islands","Near the deep's voice",N_GO_LEGEND,3);
 if(j->legend_clues[4]<3)GO("Rainbow in the Mural","Bell Tower","Find the wing's light",N_GO_ROUTE,7);
 if(!(s->legend_visits&(1u<<4)))GO("Master of the Rainbow","Bell Tower","Follow light up the tower",N_GO_LEGEND,4);
 if((c->defeated&0x20ff)!=0x20ff)GO("A Promise from Kanto","Kanto Gyms and Red","8 Kanto badges, face Red",N_GO_KANTO,0);
 GO("The Last Study","Elm's Lab","Take the trial, full team",N_GO_GYM,8);
 #undef GO
}
// Return one new scene at a safe UI boundary. No popping mid-battle or mid-reward.
static inline unsigned narrative_next(const narrative_context_t *c){
 if(c->activity||c->battle||c->dungeon)return N_SCENE_COUNT;
 if(c->state.scene)return c->state.scene-1;
 if(c->state.legend){unsigned s=N_LEGEND_RAI+c->state.legend-1;return narrative_seen(&c->state,s)?N_SCENE_COUNT:s;}
 if(c->chapter.phase==JOHTO_INVITED)return narrative_seen(&c->state,N_LETTER)?N_SCENE_COUNT:N_LETTER;
 if(c->chapter.phase==JOHTO_VOYAGE)return narrative_seen(&c->state,N_ARRIVAL)?N_SCENE_COUNT:N_ARRIVAL;
 if(c->chapter.phase<JOHTO_ACTIVE)return N_SCENE_COUNT;
 if(c->chapter.phase==JOHTO_COMPLETE){unsigned s=!narrative_seen(&c->state,N_ENDING)?N_ENDING:N_FOREST;return narrative_seen(&c->state,s)?N_SCENE_COUNT:s;}
 for(unsigned s=N_WELCOME;s<=N_ELM;s++)if(!narrative_seen(&c->state,s)&&narrative_allowed(c,s)){
  // Context comes first: a report cannot precede its activity; late-game saves
  // get a short recap, then the next actionable scene, never forced replays.
  if(s==N_BUG_INVITE&&(c->state.bug_evidence||c->bug_completed))continue;
  if(s==N_RUIN_INVITE&&(c->story.ruins&1))continue;
  if(s==N_TOWER_INVITE&&c->story.tower_seen)continue;
  if(s==N_LAKE_INVITE&&c->lake==LAKE_RECEIVED)continue;
  if(s==N_SEA_INVITE&&c->feather)continue;
  return s;
 }
 return N_SCENE_COUNT;
}
