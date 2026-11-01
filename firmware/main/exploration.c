#include "exploration.h"
#include "assets.h"
#include <stddef.h>

static const exploration_route_t ROUTES[4]={
 {"Evergreen Forest","Grass and treetop Pokemon",{"Shock marks on a tree","Tiny cries in the grass","A yellow shape flashes by"},25,0x3ce7},
 {"Moon Cave","Find Pokemon in the rock",{"Huge drag marks on the ground","Low echoes from the wall","Gravel is trembling"},95,0x9c53},
 {"Seaside Shallows","Find tide-borne Pokemon",{"Wide tracks on the sand","A long song from the sea","A blue back rises in the water"},131,0x3d7f},
 {"Abandoned Power Plant","Find Pokemon in the current",{"Scorch marks by the wires","Distant crackling current","A black-yellow shape crosses"},125,0xfde0},
};
// Curated route habitats. Rarity is encounter frequency, not a stat-total filter.
static const uint8_t POOLS[4][5][16]={
 {{10,13,16,43,46,21,23},{11,14,25,69,1,29,32,48,102},{12,15,44,70,2,17,22,24,30,33,49,47},{3,45,71,114,123,18,31,34,83,122},{127,103,149,151}},
 {{74,41,27,50},{35,66,111,104,39,56,37,4},{75,42,51,67,28,5,57},{95,105,112,76,68,36,40,38,6,106,107,108},{142,143,150,115,113}},
 {{129,72,60,98,54},{7,116,118,90,79,86,120},{8,61,117,99,55,73,119,80},{9,62,87,134,139,141,91,121,138,140,147,148},{130,131,144,124}},
 {{19,100,81,88,52},{25,109,137,132,20,58,77,84,96,92,63,133},{82,101,26,110,53,78,85,97,93,64},{125,126,89,135,136,65,94},{145,59,146,128}}
};
const exploration_route_t *exploration_route(unsigned id) {const exploration_region_t *r=exploration_region(id);return r?&r->route:id<4?&ROUTES[id]:&ROUTES[0];}
static const exploration_chapter_t CHAPTERS[EXPLORATION_CHAPTERS]={
 {"Fresh Start","Begin the adventure","Oak: Find Pokemon nearby","4 starter routes",0,0,{25,95,131,125},ITEM_BERRY},
 {"Forest Watcher","Earn 1 badge","Brock: A scythe shape in woods","Deep forest and Leaf Stone",1,0,{123,95,131,125},ITEM_LEAF_STONE},
 {"Tide Secret","Earn 2 badges","Misty: Watch for old shells","Fossil beach, Water Stone",2,0,{123,95,138,125},ITEM_WATER_STONE},
 {"Ancient Echo","Earn 4 badges","Researcher: Wings in the rock","Ancient cave, Moon Stone",4,0,{123,142,138,125},ITEM_MOON_STONE},
 {"Sleeping Current","Earn 6 badges","Worker: Power is back","Plant core, Thunder Stone",6,0,{123,142,138,135},ITEM_THUNDER_STONE},
 {"Footprints of Fire","Earn 8 badges","Blaine: Undying fire feathers","Fire bird, Fire Stone",8,0,{123,146,138,135},ITEM_FIRE_STONE},
 {"Legend of Sky and Sea","Defeat the Elite Four","Lance: Chase the sky legend","Ice/Thunder birds, Link Cable",8,0x0f00,{149,146,144,145},ITEM_LINK_MACHINE},
 {"Visitor of the Forbidden","Defeat the Champion","Oak: Unknown power in the cave","Mewtwo clues, Growth Machine",8,0x1f00,{149,150,144,145},ITEM_GROWTH_MACHINE},
 {"The First Phantom","Defeat Red","Red: One last forest mystery","Mew clues, Master Ball",8,0x3f00,{151,150,144,145},ITEM_MASTER},
};
const exploration_chapter_t *exploration_chapter(unsigned chapter){return &CHAPTERS[chapter<EXPLORATION_CHAPTERS?chapter:0];}
bool exploration_chapter_open(unsigned chapter,uint16_t defeated){
 if(chapter>=EXPLORATION_CHAPTERS)return false;
 unsigned badges=0;for(unsigned i=0;i<8;i++)badges+=!!(defeated&(1u<<i));
 const exploration_chapter_t *c=&CHAPTERS[chapter];return badges>=c->badges&&(defeated&c->wins)==c->wins;
}
unsigned exploration_chapter_current(uint16_t defeated){unsigned n=0;for(unsigned i=1;i<EXPLORATION_CHAPTERS;i++)if(exploration_chapter_open(i,defeated))n=i;return n;}
uint16_t exploration_target(unsigned route,uint16_t defeated){return exploration_chapter(exploration_chapter_current(defeated))->targets[route<4?route:0];}
bool exploration_species_open(unsigned species,uint16_t defeated){
 unsigned c=exploration_chapter_current(defeated);
 if(species==151)return c>=8;
 if(species==150)return c>=7;
 if(species==144||species==145)return c>=6;
 if(species==146||species==149)return c>=5;
 if(species==142)return c>=3;
 if(species>=138&&species<=141)return c>=2;
 return true;
}
int exploration_habitat(unsigned species,unsigned *rarity) {
 for(unsigned route=0;route<4;route++)for(unsigned tier=0;tier<5;tier++)for(unsigned i=0;i<16&&POOLS[route][tier][i];i++)
  if(POOLS[route][tier][i]==species){if(rarity)*rarity=tier+1;return route;}
 return -1;
}
unsigned exploration_unlock_chapter(unsigned species) {
 if(species==151)return 8;
 if(species==150)return 7;
 if(species==144||species==145)return 6;
 if(species==146||species==149)return 5;
 if(species==142)return 3;
 if(species>=138&&species<=141)return 2;
 return 0;
}
uint16_t exploration_focus(const exploration_state_t *s,uint16_t defeated) {
 return s->tracked_species&&exploration_species_open(s->tracked_species,defeated)&&exploration_habitat(s->tracked_species,NULL)==s->route?s->tracked_species:exploration_target(s->route,defeated);
}
const char *exploration_story(unsigned species,unsigned clue) {
 static const char *const fossil[]={"Ancient patterns in the rock","A fossil still warm","Life echoes behind the wall"};
 static const char *const bird[]={"The far sky changes color","A feather holds great power","Legendary wings ahead"};
 static const char *const mewtwo[]={"Found lost research notes","Unknown power echoes in the cave","A call from deep in the lab"};
 static const char *const mew[]={"A pink shadow among leaves","Tiny tracks suddenly vanish","It seems curious about you"};
 static const char *const normal[]={"Tracks lead deeper on the route","The target cries in the distance","The target is nearby"};
 if(clue>2)clue=2;
 return (species==151?mew:species==150?mewtwo:species>=144&&species<=146?bird:species>=138&&species<=142?fossil:normal)[clue];
}
static uint32_t mix(uint32_t x) {
 x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;return x^(x>>16);
}
unsigned exploration_chain_shiny_bp(uint32_t wins,shiny_source_t source) {
 uint64_t denominator=(uint64_t)wins+48u*enc_shiny_denominator(source);
 return (unsigned)(((uint64_t)wins+48u)*10000u/denominator);
}
void exploration_chain_discovery(exploration_event_t *event,enc_queue_t *q,dex_t *dex,uint32_t wins) {
 if(!event||!event->uid)return;
 encounter_t *e=enc_queue_find(q,event->uid);
 if(!exploration_chain_encounter(e))return;
 shiny_source_t source=e->activity==ENC_ACTIVITY_EXPLORATION?SHINY_EXPLORATION:SHINY_BADGE;
 // The base draw already happened in the encounter generator. An independent
 // bonus gives P=(wins+48)/(wins+48*base_denominator), with no gameplay cap.
 // Only run during creation, before the candidate save commits.
 uint32_t roll=event->chain_roll;
 uint64_t threshold=((uint64_t)wins<<32)/((uint64_t)wins+48u*enc_shiny_denominator(source));
 if(wins&&roll<threshold)e->is_shiny=true;
 event->shiny=e->is_shiny;dex_mark_seen(dex,e->species_id,e->is_shiny);
}
unsigned exploration_legacy_level_min(unsigned route) {
 static const uint8_t minimum[4]={2,8,12,18};return route<4?minimum[route]:1;
}
static void clue_supply(exploration_event_t *e,inventory_t *bag,unsigned local_item,uint32_t seed) {
 if(!bag)return;
 unsigned roll=seed%100;
 unsigned item=roll<45?ITEM_BERRY:roll<65?ITEM_MILK:roll<70?ITEM_ENERGY_ROOT:roll<75?ITEM_JOY_COOKIE:local_item;
 item_loot_t loot={.item_id=item,.quantity=item==ITEM_BERRY?2:1};
 loot=items_fit_loot(loot,bag,mix(seed));
 bag->quantity[loot.item_id]+=loot.quantity;
 e->item=loot.item_id;e->quantity=loot.quantity;e->item_full=loot.full;
}
static bool pending(const enc_queue_t *q,unsigned species) {
 for(unsigned i=0;i<q->count;i++)if(q->items[i].species_id==species)return true;
 return false;
}
const char *exploration_special_name(unsigned special) {
 static const char *names[]={"", "Pokemon found supplies", "Pokemon training", "Shiny clue"};
 return special<4?names[special]:names[0];
}
void exploration_special_apply(exploration_event_t *e,enc_queue_t *q,dex_t *dex,
 inventory_t *bag,const nurture_t *pet,uint32_t steps) {
 if(!e||!q||!dex||!bag|| (e->kind!=EXPLORE_CLUE&&e->kind!=EXPLORE_ENCOUNTER&&e->kind!=EXPLORE_TARGET))return;
 // Only the committed action counter selects this event, never wall time,
 // redraw, the selected leader, or a failed save/retry.
 uint32_t seed=mix(steps^e->route*0x9e3779b9u^0x714e35bdu);
 if(seed%100>=nurture_event_percent(pet))return;
 if(e->kind==EXPLORE_CLUE){
  unsigned amount=2,room=items_capacity(ITEM_BERRY)-bag->quantity[ITEM_BERRY];
  if(amount>room)amount=room;
  bag->quantity[ITEM_BERRY]+=amount;e->extra_quantity=amount;e->special=EXPLORE_SPECIAL_SUPPLY;
 }else{
  encounter_t *enc=enc_queue_find(q,e->uid);if(!enc)return;
  e->special=(mix(seed^0x87ab31u)&1)?EXPLORE_SPECIAL_TRAINING:EXPLORE_SPECIAL_SPARKLE;
  // Base 1/48 plus an independent 1/47 gives 1/24 before the chain bonus.
  if(e->special==EXPLORE_SPECIAL_SPARKLE&&mix(seed^0x6618745bu)%47==0)enc->is_shiny=true;
  e->shiny=enc->is_shiny;dex_mark_seen(dex,e->species,e->shiny);
 }
}
static exploration_event_t step(exploration_state_t *s,enc_refresh_state_t *r,
 enc_queue_t *q,dex_t *dex,uint16_t active_uid,uint16_t defeated,bool campaign,unsigned nurture_bonus,unsigned target_override)
{
 exploration_event_t e={.kind=EXPLORE_NONE,.item=ITEM_NONE};
 if(!s||!r||!q||!dex||!exploration_valid(s)||!enc_refresh_valid(r))return e;
 e.route=s->route;e.clues=s->clues[s->route];
 // Retain the former boosted odds as the baseline; stamina is the only entry resource.
 nurture_bonus+=100;
 unsigned route=s->route;
 // Clue events are discoveries, not battles: do not advance rarity pity.
 if(s->clues[route]<3 && s->pulse[route]) {
 s->steps++;s->pulse[route]=0;s->clues[route]++;
  e.kind=EXPLORE_CLUE;e.clues=s->clues[route];return e;
 }
 bool target=s->clues[route]>=3;
 unsigned bonus=r->discoveries/10;if(bonus>10)bonus=10;
 uint32_t seed=mix(s->steps^r->serial*0x9e3779b9u^route*0x85ebca6bu^0x51a78u);
 unsigned roll=seed%1000;
 uint8_t rarity=roll<10+2*bonus+nurture_bonus/10?5:roll<60+4*bonus+nurture_bonus*2/5?4:roll<250+10*bonus+nurture_bonus?3:roll<650?2:1;
 if(target && rarity<3)rarity=3;
 if(r->since_elite>=29)rarity=5;
 else if(r->since_rare>=7 && rarity<4)rarity=4;
 unsigned species=0;
 if(target) {
  species=target_override?target_override:campaign?exploration_focus(s,defeated):ROUTES[route].target;
  unsigned native_rarity;if(exploration_habitat(species,&native_rarity)>=0)rarity=native_rarity;
  // Paid discoveries are separate individuals even when this species is queued.
  // enc_queue_push keeps distinct UIDs and applies the normal five-entry FIFO.
 } else {
  const uint8_t *pool=POOLS[route][rarity-1];
  unsigned n=0;while(n<16&&pool[n])n++;
  // Three of four rolls prefer uncaught species within the same unlocked tier.
  if(campaign&&(seed&3)!=0)for(unsigned i=0;i<n;i++){
   unsigned candidate=pool[(seed/1000+i)%n];
   if(exploration_species_open(candidate,defeated)&&!dex_is_caught(dex,candidate)){species=candidate;break;}
  }
  for(unsigned i=0;!species&&i<n;i++) {
   unsigned candidate=pool[(seed/1000+i)%n];
   if(!campaign||exploration_species_open(candidate,defeated)){species=candidate;break;}
  }
  if(!species){e.kind=EXPLORE_BLOCKED;return e;}
 }
 species_t sp;
 if(!assets_species(species,&sp)){e.kind=EXPLORE_BLOCKED;return e;}
 uint32_t shiny=mix(seed^0x735a91cdu);
 encounter_t encounter={.activity=ENC_ACTIVITY_EXPLORATION,.ts=r->online_s,.species_id=species,.rarity=rarity,
  .biome=route,.hp_ratio=100,.is_transient=true,.is_shiny=enc_shiny_from_roll(shiny,SHINY_EXPLORATION)};
 while(!q->next_uid||q->next_uid==active_uid||enc_queue_find(q,q->next_uid))q->next_uid++;
 enc_queue_push(q,&encounter);
 dex_mark_seen(dex,species,encounter.is_shiny);
 s->steps++;
 if(target){s->clues[route]=0;s->pulse[route]=0;}else s->pulse[route]=1;
 r->since_rare=rarity>=4?0:r->since_rare<7?r->since_rare+1:7;
 r->since_elite=rarity>=5?0:r->since_elite<29?r->since_elite+1:29;
 e.kind=target?EXPLORE_TARGET:EXPLORE_ENCOUNTER;
 e.uid=q->items[q->count-1].uid;e.species=species;e.rarity=rarity;e.shiny=encounter.is_shiny;e.chain_roll=mix(seed^0x8da6b343u);
 e.clues=s->clues[route];return e;
}

// Baseline rule harness retained for V11 regression. Runtime uses progress API.
exploration_event_t exploration_step(exploration_state_t *s,enc_refresh_state_t *r,enc_queue_t *q,dex_t *d,uint16_t active){return step(s,r,q,d,active,0,false,0,0);}
exploration_event_t exploration_step_with_target(exploration_state_t *s,enc_refresh_state_t *r,enc_queue_t *q,dex_t *d,uint16_t active,uint16_t defeated,inventory_t *bag,const nurture_t *pet,unsigned team_bonus,unsigned target){
 exploration_event_t e=step(s,r,q,d,active,defeated,true,nurture_rare_bonus(pet)+(team_bonus>30?30:team_bonus),target);
 if(e.kind!=EXPLORE_CLUE||!bag)return e;
 unsigned chapter=exploration_chapter_current(defeated);
 uint32_t roll=mix(s->steps^r->serial*0x9e3779b9u^s->route*0x85ebca6bu^0x18b479u);
 // Previously unlocked supplies remain available. Master Balls remain uncommon among
 // clue discoveries after Red, and never replace the champion reward.
 unsigned reward=(roll/100)%(chapter+1);
 if(reward==8 && (roll/1000)%4!=0)reward=7;
 clue_supply(&e,bag,exploration_chapter(reward)->item,roll);return e;
}

exploration_event_t exploration_step_progress(exploration_state_t *s,enc_refresh_state_t *r,enc_queue_t *q,dex_t *d,uint16_t active,uint16_t defeated,inventory_t *bag){return exploration_step_nurtured(s,r,q,d,active,defeated,bag,NULL);}

exploration_event_t exploration_step_nurtured(exploration_state_t *s,enc_refresh_state_t *r,enc_queue_t *q,dex_t *d,uint16_t active,uint16_t defeated,inventory_t *bag,const nurture_t *pet){return exploration_step_team(s,r,q,d,active,defeated,bag,pet,0);}

unsigned exploration_team_bonus(const party_t *party,unsigned route) {
 if(!party||route>=4)return 0;
 // GEN1 type IDs: grass/bug, rock/ground, water/ice, electric/poison.
 static const uint8_t types[4][2]={{4,11},{12,8},{2,5},{3,7}};
 unsigned matches=0;for(unsigned i=0;i<party->party_count;i++){
  bool duplicate=false;for(unsigned j=0;j<i;j++)if(party->party[j].species_id==party->party[i].species_id)duplicate=true;
  species_t sp;if(duplicate||!assets_species(party->party[i].species_id,&sp))continue;
  if(sp.type1==types[route][0]||sp.type2==types[route][0]||sp.type1==types[route][1]||sp.type2==types[route][1])matches++;
 }
 return matches>3?30:matches*10;
}

void exploration_research_progress(unsigned route,const dex_t *dex,uint8_t *seen,uint8_t *caught){
 *seen=*caught=0;if(route>=4||!dex)return;
 bool counted[152]={0};
 for(unsigned tier=0;tier<5;tier++)for(unsigned i=0;i<16&&POOLS[route][tier][i];i++){
  unsigned id=POOLS[route][tier][i];if(counted[id])continue;counted[id]=true;
  *seen+=dex_is_seen(dex,id);*caught+=dex_is_caught(dex,id);
 }
}
exploration_kind_t exploration_research_claim(exploration_state_t *s,const dex_t *dex,unsigned route){
 if(!s||route>=4)return EXPLORE_BLOCKED;
 if(s->research_flags&(1u<<route))return EXPLORE_RESEARCH_CLAIMED;
 uint8_t seen,caught;exploration_research_progress(route,dex,&seen,&caught);
 if(seen<5||caught<3||!(s->research_flags&(16u<<route)))return EXPLORE_RESEARCH_LOCKED;
 s->research_flags|=1u<<route;return EXPLORE_NONE;
}

// Trails rotate at encounter delivery, never on redraw, reboot or route selection.
// Weights are per species: uncommon 12, rare 6, very rare 2, legendary 1.
static unsigned target_weight(unsigned species,unsigned tier,const dex_t *dex) {
 unsigned weight=tier<2?12:tier==2?6:2;
 if(species>=144&&species<=151&&species!=147&&species!=148)weight=1;
 return dex_is_caught(dex,species)?weight:weight*3;
}
static uint16_t pick_target(unsigned route,unsigned previous,uint32_t round,uint16_t defeated,const dex_t *dex,const enc_queue_t *queue) {
 unsigned total=0;
 for(unsigned tier=1;tier<5;tier++)for(unsigned i=0;i<16&&POOLS[route][tier][i];i++){
  unsigned id=POOLS[route][tier][i];
  if(id!=previous&&exploration_species_open(id,defeated)&&!pending(queue,id))total+=target_weight(id,tier,dex);
 }
 if(!total)return exploration_target(route,defeated);
 unsigned roll=mix(round*0x9e3779b9u ^ route*0x85ebca6bu ^ defeated ^ 0x41c64e6du)%total;
 for(unsigned tier=1;tier<5;tier++)for(unsigned i=0;i<16&&POOLS[route][tier][i];i++){
  unsigned id=POOLS[route][tier][i];
  if(id==previous||!exploration_species_open(id,defeated)||pending(queue,id))continue;
  unsigned weight=target_weight(id,tier,dex);if(roll<weight)return id;roll-=weight;
 }
 return exploration_target(route,defeated);
}
void exploration_targets_sync(exploration_state_t *s,exploration_updates_t *u,const dex_t *d,const enc_queue_t *q,uint16_t defeated) {
 unsigned chapter=exploration_chapter_current(defeated);
 for(unsigned route=0;route<4;route++){
  if(u->targets[route]&&exploration_species_open(u->targets[route],defeated)&&(u->chapters[route]==chapter||s->clues[route]||s->pulse[route]))continue;
  unsigned previous=u->targets[route];
  // Migration preserves a trail that was already started under the old rules.
  u->targets[route]=!previous&&(s->clues[route]||s->pulse[route])?exploration_target(route,defeated):pick_target(route,previous,u->rounds[route],defeated,d,q);
  u->chapters[route]=chapter;
 }
}
uint16_t exploration_current_target(const exploration_state_t *s,const exploration_updates_t *u,uint16_t defeated) {
 if(s->tracked_species&&exploration_species_open(s->tracked_species,defeated)&&exploration_habitat(s->tracked_species,NULL)==s->route)return s->tracked_species;
 return u->targets[s->route]?u->targets[s->route]:exploration_target(s->route,defeated);
}
void exploration_target_completed(exploration_state_t *s,exploration_updates_t *u,const dex_t *d,const enc_queue_t *q,uint16_t defeated,unsigned route) {
 // Explicit dex tracking remains pinned until the player cancels it.
 if(s->tracked_species&&exploration_habitat(s->tracked_species,NULL)==(int)route)return;
 u->rounds[route]++;
 u->targets[route]=pick_target(route,u->targets[route],u->rounds[route],defeated,d,q);
 u->chapters[route]=exploration_chapter_current(defeated);
}
bool exploration_updates_valid(const exploration_updates_t *u) {
 for(unsigned i=0;i<4;i++)if(u->targets[i]>151||u->chapters[i]>=EXPLORATION_CHAPTERS)return false;
 for(unsigned i=0;i<8;i++)if(u->activity_progress[i]>3)return false;
 return true;
}
static const exploration_activity_t ACTIVITIES[8]={
 {"Cave Treasure","Brock: Search along the wall",1,12,ITEM_MOON_STONE,{74,27,50,35,66,95,104,111}},
 {"Tide Fishing","Misty: Low tide finds",2,19,ITEM_WATER_STONE,{60,54,72,98,116,90,138,140}},
 {"Plant Repair","Surge: Track the stray current",3,26,ITEM_THUNDER_STONE,{25,81,100,137,82,101,125,26}},
 {"Forest Survey","Erika: Forest Pokemon migrate",0,32,ITEM_LEAF_STONE,{43,69,46,102,114,123,127,44}},
 {"Poison Fog Survey","Janine: Find the fog source",3,39,ITEM_ENERGY_ROOT,{23,88,109,48,24,89,110,49}},
 {"Phantom Chase","Sabrina: Sense the phantom",3,43,ITEM_GROWTH_MACHINE,{63,96,92,64,97,93,122,65}},
 {"Volcano Expedition","Blaine: Fire tracks in hills",1,48,ITEM_FIRE_STONE,{4,37,58,77,5,38,78,126}},
 {"Deep Expedition","Giovanni: Strong ones below",1,53,ITEM_LINK_MACHINE,{95,112,31,34,76,142,115,143}}
};
const exploration_activity_t *exploration_activity(unsigned id){return id<8?&ACTIVITIES[id]:NULL;}
bool exploration_activity_open(unsigned id,uint16_t defeated){return id<8&&(defeated&(1u<<id));}
exploration_event_t exploration_activity_spawn(unsigned id,exploration_updates_t *u,enc_queue_t *q,dex_t *d,uint16_t defeated,uint32_t seed,uint16_t active) {
 exploration_event_t e={.kind=EXPLORE_RESEARCH_LOCKED,.item=ITEM_NONE};
 if(!exploration_activity_open(id,defeated)||u->activity_progress[id]>=3)return e;
 if(u->activity_uid[id]&&enc_queue_find(q,u->activity_uid[id])){e.kind=EXPLORE_BLOCKED;return e;}
 const exploration_activity_t *a=&ACTIVITIES[id];unsigned start=mix(seed^u->activity_runs[id]^id*7919u)%8;
 unsigned species=0,rarity=2;
 for(unsigned i=0;i<8;i++){unsigned candidate=a->species[(start+i)%8];if(exploration_species_open(candidate,defeated)&&!pending(q,candidate)){species=candidate;break;}}
 if(!species){e.kind=EXPLORE_BLOCKED;return e;}
 exploration_habitat(species,&rarity);
 encounter_t enc={.species_id=species,.rarity=rarity,.level=a->level+u->activity_progress[id],.activity=id+1,.biome=a->route,.hp_ratio=100,.is_transient=true,.ts=seed,.is_shiny=enc_shiny_from_roll(mix(seed^0x735a91cdu),SHINY_BADGE)};
 while(!q->next_uid||q->next_uid==active||enc_queue_find(q,q->next_uid))q->next_uid++;
 enc_queue_push(q,&enc);u->activity_uid[id]=q->items[q->count-1].uid;
 dex_mark_seen(d,species,enc.is_shiny);
 e.kind=EXPLORE_ENCOUNTER;e.uid=u->activity_uid[id];e.species=species;e.rarity=rarity;e.level=enc.level;e.route=a->route;e.shiny=enc.is_shiny;e.chain_roll=mix(seed^0x8da6b343u);return e;
}
void exploration_activity_credit(exploration_updates_t *u,const encounter_t *enc) {
 if(!enc->activity||enc->activity>8)return;
 unsigned id=enc->activity-1;
 if(u->activity_uid[id]!=enc->uid)return;
 u->activity_uid[id]=0;if(u->activity_progress[id]<3)u->activity_progress[id]++;
}
exploration_kind_t exploration_activity_claim(unsigned id,exploration_updates_t *u,inventory_t *bag,exploration_event_t *e) {
 if(id>=8||u->activity_progress[id]!=3)return EXPLORE_RESEARCH_LOCKED;
 bool first=!(u->activity_claimed&(1u<<id));
 unsigned roll=mix(u->activity_runs[id]^id*7919u)%100;
 unsigned item=first?ACTIVITIES[id].item:roll<50?ITEM_GREAT:roll<80?ITEM_BERRY:roll<95?ITEM_ULTRA:ACTIVITIES[id].item;
 unsigned qty=first||roll>=95?1:3;
 if(bag->quantity[item]+qty>items_capacity(item)){e->item_full=true;return EXPLORE_BLOCKED;}
 bag->quantity[item]+=qty;u->activity_claimed|=1u<<id;u->activity_progress[id]=0;u->activity_uid[id]=0;u->activity_runs[id]++;
 e->kind=EXPLORE_NONE;e->item=item;e->quantity=qty;return EXPLORE_NONE;
}

exploration_event_t exploration_step_team(exploration_state_t *s,enc_refresh_state_t *r,enc_queue_t *q,dex_t *d,uint16_t active,uint16_t defeated,inventory_t *bag,const nurture_t *pet,unsigned bonus) {
 return exploration_step_with_target(s,r,q,d,active,defeated,bag,pet,bonus,0);
}


bool exploration_dungeon_partner(uint32_t seed,uint32_t run_id,uint16_t defeated,
                                 const enc_queue_t *queue,encounter_t *out)
{
 if(!queue||!out||!run_id)return false;
 unsigned count=0;
 for(unsigned sid=1;sid<=DEX_SPECIES;sid++) {
  if(exploration_habitat(sid,NULL)==0&&exploration_species_open(sid,defeated)&&!pending(queue,sid))count++;
 }
 if(!count)return false;
 uint32_t identity=mix(seed^mix(run_id)^0xd091ca71u);
 unsigned pick=mix(identity^0x3a79b21du)%count;
 for(unsigned sid=1;sid<=DEX_SPECIES;sid++) {
  unsigned rarity=0;
  if(exploration_habitat(sid,&rarity)!=0||!exploration_species_open(sid,defeated)||pending(queue,sid))continue;
  if(pick-- != 0)continue;
  *out=(encounter_t){.species_id=sid,.rarity=rarity,.biome=0,.hp_ratio=100,
      .is_transient=true,.is_shiny=enc_shiny_from_roll(mix(identity^0x735a91cdu),SHINY_DUNGEON)};
  return true;
 }
 return false;
}

// First-generation expansion. Species keep their original rarity and story gates.
static const exploration_region_t REGIONS[EXPLORATION_REGIONS]={
 {{"Twin Islands","Songs of tide and ice cave",{"Low tide shows shells","Tracks on the ice","A song from the cave"},131,0x45df},"Tide Cave","Earn the Rainbow Badge",{"Shell Beach","Ice Cave Mouth","Long Song"},1u<<3,35,45,2,ITEM_WATER_STONE,
  {86,90,116,120,117,80,73,87,91,121,134,131,124,130,144},{{91,117,131},{87,121,124},{80,134,130}},{{91,121,131},{87,124,131},{134,130,131}}},
 {{"Ghost Tower","Lost Pokemon in moonlight",{"Candles suddenly flicker","Footsteps from upstairs","A shadow by the window"},94,0x9b3f},"Phantom Trial","Earn the Pink Badge",{"Ghost Tracks","Bone-mask Pokemon","Psychic Echo"},1u<<4,42,55,3,ITEM_LINK_MACHINE,
  {92,96,104,93,97,64,94,105,122,65,108,124,115},{{94,97,105},{93,122,94},{105,108,115}},{{94,122,124},{105,108,115},{65,122,124}}},
 {{"Wild Fields","Tree marks and giant prints",{"Tracks in the grass","Scythe marks on a trunk","A low rumble far off"},113,0x7dc9},"Patrol Expedition","Earn the Rainbow Badge",{"Giant Prints","Trunk Scythe Marks","Soft Tracks"},1u<<3,38,50,0,ITEM_MOON_STONE,
  {29,32,102,44,70,49,47,114,123,83,31,34,113,115,128,127},{{123,127,115},{31,34,128},{114,113,115}},{{31,34,128,115},{123,114,127},{83,113,115}}},
 {{"Magnet Rail Mine","Current in old tunnels",{"The mine rails tremble","Lights switch on in the plant","A magnetic field deep below"},125,0xbdf0},"Steel Corridor","Earn the Gold Badge",{"Old Mine Tunnel","Power Room","Abandoned Warehouse"},1u<<5,45,58,3,ITEM_THUNDER_STONE,
  {81,100,137,111,82,101,75,26,95,112,76,125,135,128,142,145},{{82,95,125},{101,112,135},{76,137,128}},{{95,112,142},{125,135,128},{76,125,128}}},
 {{"Volcano Ruins","Old steps by the lava",{"Steam from rock cracks","Claw marks on the steps","A fire feather rises far off"},126,0xfaa5},"Lava Trial","Earn the Crimson Badge",{"Lava Steps","Warm Cave","High Fire Feather"},1u<<6,48,60,1,ITEM_FIRE_STONE,
  {37,58,77,4,5,78,75,67,38,6,126,136,112,59,142,146},{{126,78,59},{112,6,136},{38,67,126}},{{112,126,142},{38,136,59},{6,126,146,59}}},
 {{"Dragon Breath Valley","Follow young dragon tracks",{"Scales left by the stream","Wing marks on the rock","A dragon roars in the valley"},148,0x6e39},"Dragon Den Trial","Earn the Green Badge",{"Stream Scales","Cliff Wing Marks","Valley Roar"},1u<<7,50,65,2,ITEM_GROWTH_MACHINE,
  {116,111,117,75,73,147,148,95,112,6,9,130,142,149,131},{{148,130,149},{95,142,149},{9,131,130}},{{147,148,130},{6,112,142},{148,131,149}}},
 {{"Empty Lab","A lost mirror experiment",{"Found research notes","A shape in the glass","Unknown power awakens"},137,0x7e7f},"Mirror Experiment","Defeat the Champion",{"Culture Pod","Sim Room","Secret Files"},1u<<12,60,75,3,ITEM_LINK_MACHINE,
  {137,132,63,64,82,93,65,94,122,135,136,134,113,143,150},{{65,94,143},{135,134,136},{122,113,150}},{{65,134,113},{94,135,143},{122,136,150,113}}},
 {{"Silver Snow Ridge","Expedition above the snowline",{"Tracks in the snow","Scales found at camp","A song from the peak"},131,0xc71f},"Snow Ridge Trek","Champion and new-area study",{"Snow Tracks","Mountain Camp","Peak Legend"},1u<<12,65,85,2,ITEM_GROWTH_MACHINE,
  {86,90,117,80,75,87,91,121,148,95,131,143,149,144,124},{{87,131,143},{124,91,149},{121,148,144}},{{87,91,124},{95,148,143},{121,131,144,149}}},
};
const exploration_region_t *exploration_region(unsigned map){return map>=4&&map<EXPLORATION_MAPS?&REGIONS[map-4]:NULL;}
// Explicit habitats: pool order must never change the meaning of a trail.
// Their union stays within (and covers) the existing region research pool.
static const exploration_trail_t TRAILS[EXPLORATION_REGIONS][3]={
 {{"Shell Beach",{90,91,120,121,134}},
  {"Ice Cave Mouth",{86,87,124,131,144}},
  {"Deep Tideway",{116,117,73,80,130}}},
 {{"Ghost Tracks",{92,93,94}},
  {"Bone-mask Pokemon",{104,105,108,115}},
  {"Psychic Echo",{96,97,64,65,122,124}}},
 {{"Giant Prints",{29,32,31,34,128,115}},
  {"Trunk Scythe Marks",{102,44,70,49,47,114,123,127}},
  {"Grass Tracks",{29,32,49,83,113,115}}},
 {{"Old Mine Tunnel",{111,75,95,112,76,142}},
  {"Power Room",{81,100,82,101,26,125,135,145}},
  {"Abandoned Warehouse",{137,111,75,112,76,128}}},
 {{"Lava Steps",{75,67,112,126,142}},
  {"Warm Wind Slope",{37,38,58,59,77,78,136}},
  {"Fire Feather Traces",{4,5,6,146}}},
 {{"Stream Scales",{116,117,147,148,130}},
  {"Cliff Wing Marks",{111,75,95,112,6,142}},
  {"Lakeside Roar",{147,148,149,9,73,131}}},
 {{"Culture Pod",{132,63,64,65,113,143,150}},
  {"Sim Room",{137,82,135,136,134}},
  {"Secret Files",{63,64,93,65,94,122,150}}},
 {{"Snow Tracks",{86,87,90,91,124}},
  {"Mountain Camp",{75,95,80,143,117,148}},
  {"Peak Legend",{117,80,148,121,131,149,144}}},
};
// Temporary visitors broaden each area's ecology without moving its regular
// habitats or adding new persistent counters. No legendary visitor shortcuts.
static const uint8_t GUESTS[EXPLORATION_REGIONS][12]={
 {7,8,9,54,55,72,79,98,99,118,119,130},
 {41,42,52,53,63,109,110,132},
 {1,2,3,16,17,18,25,30,33,48,69,103},
 {27,28,50,51,66,67,74,104,105,109,110,137},
 {66,68,74,76,95,104,105,111,115,128},
 {21,22,54,55,79,80,83,84,85,98,99,123},
 {25,26,81,96,97,100,101,109,110,125,133,136},
 {28,42,55,73,76,79,82,105,112,119,130,134},
};
unsigned exploration_guest_candidates(unsigned map,uint8_t out[12]){
 if(map<4||map>=EXPLORATION_MAPS||!out)return 0;
 unsigned n=0;const exploration_region_t *r=exploration_region(map);
 for(unsigned i=0;i<12&&GUESTS[map-4][i];i++){
  unsigned id=GUESTS[map-4][i],tier=0;exploration_habitat(id,&tier);
  bool regular=false;for(unsigned j=0;j<24&&r->pool[j];j++)regular|=id==r->pool[j];
  if(!regular&&tier>=2&&tier<=4)out[n++]=id;
 }
 return n;
}
unsigned exploration_visitors(unsigned map,unsigned direction,uint32_t steps,uint16_t defeated,uint8_t out[2]){
 if(!out||direction>=3)return 0;
 uint8_t candidates[12];unsigned total=exploration_guest_candidates(map,candidates),n=0;
 for(unsigned i=0;i<total;i++)if(exploration_species_open(candidates[i],defeated))candidates[n++]=candidates[i];
 if(!n)return 0;
 unsigned start=((steps/EXPLORATION_ROTATION_STEPS)%n*2+direction*3)%n;
 out[0]=candidates[start];if(n>1)out[1]=candidates[(start+1)%n];
 return n>1?2:1;
}
const exploration_trail_t *exploration_region_trail(unsigned map,unsigned direction){
 return map>=4&&map<EXPLORATION_MAPS&&direction<3?&TRAILS[map-4][direction]:NULL;
}
unsigned exploration_trail_examples(unsigned map,unsigned direction,bool deep,uint16_t defeated,uint8_t out[2]){
 const exploration_trail_t *trail=exploration_region_trail(map,direction);unsigned count=0;
 if(!trail||!out)return 0;
 for(unsigned i=0;i<12&&trail->species[i]&&count<2;i++){
  unsigned id=trail->species[i],tier=0;exploration_habitat(id,&tier);
  if(tier>=(deep?3u:2u)&&exploration_species_open(id,defeated))out[count++]=id;
 }
 return count;
}
bool exploration_map_open(unsigned map,uint16_t defeated,const exploration_regions_t *s){
 if(map<4)return true;
 const exploration_region_t *r=exploration_region(map);if(!r||(defeated&r->gate)!=r->gate)return false;
 if(map==11){if(!s)return false;for(unsigned i=0;i<8;i++)if(s->region[i].claimed)return true;return false;}
 return true;
}
static bool region_has(unsigned map,unsigned species){
 const exploration_region_t *r=exploration_region(map);if(!r)return false;
 for(unsigned i=0;i<24&&r->pool[i];i++)if(r->pool[i]==species)return true;
 return false;
}
bool exploration_regions_valid(const exploration_regions_t *s){
 if(!s||s->selected>=EXPLORATION_MAPS||s->dungeon_pity>4||!items_legacy_inventory_valid(&s->pending_items))return false;
 for(unsigned i=0;i<8;i++){
  const exploration_region_progress_t *p=&s->region[i];
  if(p->clues>3||p->pulse>1||p->pity>EXPLORATION_REGION_PITY||p->deep>1||p->traced>1||p->claimed>1||p->challenge_clear>1||(p->target&&!region_has(i+4,p->target)))return false;
 }
 const encounter_t *p=&s->pending_partner;
 // Inspect raw bool bytes before evaluating untrusted NVS fields.
 const unsigned char *bytes=(const unsigned char *)p;
 if(bytes[offsetof(encounter_t,is_shiny)]>1||bytes[offsetof(encounter_t,is_transient)]>1||bytes[offsetof(encounter_t,exp_granted)]>1)return false;
 return p->species_id<=151&&(!p->species_id|| (p->rarity>=4&&p->rarity<=5&&p->level>=1&&p->level<=100&&p->biome<4&&p->hp_ratio==100&&p->activity==0));
}
void exploration_region_research(unsigned map,const dex_t *d,uint8_t *seen,uint8_t *caught){
 *seen=*caught=0;const exploration_region_t *r=exploration_region(map);if(!r||!d)return;
 for(unsigned i=0;i<24&&r->pool[i];i++){*seen+=dex_is_seen(d,r->pool[i]);*caught+=dex_is_caught(d,r->pool[i]);}
 uint8_t guests[12];unsigned n=exploration_guest_candidates(map,guests);
 for(unsigned i=0;i<n;i++){*seen+=dex_is_seen(d,guests[i]);*caught+=dex_is_caught(d,guests[i]);}
}
unsigned exploration_region_target(unsigned map,const exploration_regions_t *s,uint16_t defeated,const dex_t *d){
 const exploration_region_t *r=exploration_region(map);if(!r)return 0;
 const exploration_region_progress_t *p=&s->region[map-4];
 if(p->target&&exploration_species_open(p->target,defeated))return p->target;
 unsigned ids[24],n=0;
 for(unsigned i=0;i<24&&r->pool[i];i++){unsigned tier=0,id=r->pool[i];exploration_habitat(id,&tier);if(tier>=4&&exploration_species_open(id,defeated))ids[n++]=id;}
 if(!n)return 0;
 unsigned start=mix(p->steps^map*7919u)%n;
 for(unsigned i=0;i<n;i++)if(!dex_is_caught(d,ids[(start+i)%n]))return ids[(start+i)%n];
 return ids[start];
}
exploration_event_t exploration_region_step(exploration_regions_t *s,enc_refresh_state_t *refresh,enc_queue_t *q,dex_t *dex,uint16_t active,uint16_t defeated,inventory_t *bag,unsigned direction){
 unsigned map=s->selected;const exploration_region_t *r=exploration_region(map);
 exploration_event_t e={.kind=EXPLORE_BLOCKED,.route=map,.item=ITEM_NONE};
 if(!r||direction>3||!exploration_map_open(map,defeated,s))return e;
 exploration_region_progress_t *p=&s->region[map-4];
 unsigned target=exploration_region_target(map,s,defeated,dex);if(!target)return e;
 uint32_t seed=mix(p->steps^map*7919u^refresh->serial*0x9e3779b9u);
 if(p->clues<3&&p->pulse){
  p->steps++;p->target=target;p->pulse=0;p->clues++;e.kind=EXPLORE_CLUE;e.clues=p->clues;
  clue_supply(&e,bag,(seed/100)%4?r->item:ITEM_ULTRA,mix(seed^0x18b479u));
  return e;
 }
 unsigned id=0,tier=0;bool traced=p->clues==3;
 if(traced){id=target;exploration_habitat(id,&tier);}
 else{
  unsigned candidates[5][24],n[5]={0},weights[5]={0,30,56,12,2};
  if(p->deep){weights[1]=18;weights[3]=22;weights[4]=4;}
  bool guaranteed=p->pity>=EXPLORATION_REGION_PITY;
  const uint8_t *pool=direction?exploration_region_trail(map,direction-1)->species:r->pool;
  unsigned capacity=direction?12:24;
  for(unsigned i=0;i<capacity&&pool[i];i++){unsigned sp=pool[i],t=0;exploration_habitat(sp,&t);if(t&&exploration_species_open(sp,defeated))candidates[t-1][n[t-1]++]=sp;}
  unsigned first=direction?direction-1:0,last=direction?direction:3;
  for(unsigned dir=first;dir<last;dir++){
   uint8_t guests[2];unsigned guests_n=exploration_visitors(map,dir,p->steps,defeated,guests);
   for(unsigned i=0;i<guests_n;i++){
    unsigned sp=guests[i],t=0;exploration_habitat(sp,&t);bool duplicate=false;
    for(unsigned j=0;j<n[t-1];j++)duplicate|=candidates[t-1][j]==sp;
    if(!duplicate&&n[t-1]<24)candidates[t-1][n[t-1]++]=sp;
   }
  }
  if(guaranteed&&!n[3]&&!n[4]){ // Promised rare encounter can use a nearby habitat.
   for(unsigned i=0;i<24&&r->pool[i];i++){unsigned sp=r->pool[i],t=0;exploration_habitat(sp,&t);if(t>=4&&exploration_species_open(sp,defeated))candidates[t-1][n[t-1]++]=sp;}
  }
  unsigned roll=seed%(guaranteed?(weights[3]+weights[4]):100);
  for(tier=guaranteed?4:1;tier<5;tier++){if(roll<weights[tier-1])break;roll-=weights[tier-1];}
  // Missing tiers never magnify the rare share. Redirect downward first. A
  // common roll can only use another common tier. Pending individuals do not
  // change the available species or force the player to clear the queue first.
  if(!n[tier-1]){
   unsigned replacement=0;
   for(unsigned t=guaranteed?4:1;t<tier;t++)if(n[t-1])replacement=t;
   if(!replacement)for(unsigned t=guaranteed?4:1;t<=(tier<=3?3u:5u);t++)if(n[t-1]){replacement=t;break;}
   if(!replacement)return e;
   tier=replacement;
  }
  unsigned count=n[tier-1],start=mix(seed)%count;id=candidates[tier-1][start];
  if(seed&3)for(unsigned j=0;j<count;j++){unsigned sp=candidates[tier-1][(start+j)%count];if(!dex_is_caught(dex,sp)){id=sp;break;}}
 }
 unsigned low=exploration_region_level_min(r,p->deep);
 // A separate roll avoids tying level to the rarity roll. It is persisted with
 // the encounter: opening a page, switching leaders or rebooting cannot reroll it.
 unsigned level=low+mix(seed^0x4c657665u)%(r->max_level-low+1);
 encounter_t enc={.activity=ENC_ACTIVITY_EXPLORATION,.ts=refresh->online_s,.species_id=id,.rarity=tier,.level=level,.biome=r->biome,.hp_ratio=100,.is_transient=true,.is_shiny=enc_shiny_from_roll(mix(seed^0x735a91cdu),SHINY_EXPLORATION)};
 while(!q->next_uid||q->next_uid==active||enc_queue_find(q,q->next_uid))q->next_uid++;
 enc_queue_push(q,&enc);dex_mark_seen(dex,id,enc.is_shiny);p->steps++;
 if(traced){p->clues=p->pulse=0;p->traced=1;p->target=0;if(tier>=4)p->pity=0;}else{p->pulse=1;p->target=target;p->pity=tier>=4?0:p->pity<EXPLORATION_REGION_PITY?p->pity+1:EXPLORATION_REGION_PITY;}
 e.kind=traced?EXPLORE_TARGET:EXPLORE_ENCOUNTER;e.uid=q->items[q->count-1].uid;e.species=id;e.rarity=tier;e.level=level;e.shiny=enc.is_shiny;e.chain_roll=mix(seed^0x8da6b343u);e.clues=p->clues;e.visitor=!region_has(map,id);return e;
}
bool exploration_region_partner(unsigned map,unsigned direction,bool challenge,uint32_t seed,uint16_t defeated,exploration_regions_t *s,encounter_t *out){
 const exploration_region_t *r=exploration_region(map);if(!r||direction>=3)return false;
 unsigned ids[2][4],n[2]={0};
 for(unsigned i=0;i<4&&r->rewards[direction][i];i++){unsigned id=r->rewards[direction][i],tier=0;exploration_habitat(id,&tier);if(tier>=4&&exploration_species_open(id,defeated))ids[tier-4][n[tier-4]++]=id;}
 if(!n[0]&&!n[1])return false;
 unsigned tier=n[1]&&(!n[0]||s->dungeon_pity==4||mix(seed)%100<(challenge?40u:20u))?1:0;
 unsigned id=ids[tier][mix(seed^0xa391u)%n[tier]];
 if(n[1])s->dungeon_pity=tier?0:s->dungeon_pity<4?s->dungeon_pity+1:4;
 *out=(encounter_t){.species_id=id,.rarity=tier+4,.biome=r->biome,.hp_ratio=100,.is_transient=true,.is_shiny=enc_shiny_from_roll(mix(seed^0x58fc8du),SHINY_DUNGEON)};return true;
}
