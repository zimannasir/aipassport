#include "achievements.h"
enum { DEX, SHINY, EVOLUTION, BADGES, CHAMPION, RED, ARRIVAL, EGG, CROSS_GEN, JOHTO_BADGES, JOHTO_DEX, NATIONAL_DEX, JOHTO_SHINY, RUINS, DEEP, ELM, CHAMP_GYM, CHAMP_ALL, CHAMP_ELM, RED_LAKE };
static const achievement_info_t INFO[ACHIEVEMENT_COUNT] = {
 {"New Partner","Catch 2 species",DEX,2,ITEM_GREAT,5},
 {"Little Collector","Catch 5 species",DEX,5,ITEM_THUNDER_STONE,1},
 {"Travel Collector","Catch 10 species",DEX,10,ITEM_WATER_STONE,1},
 {"Dex Researcher","Catch 25 species",DEX,25,ITEM_LEAF_STONE,1},
 {"Dex Expert","Catch 50 species",DEX,50,ITEM_LINK_MACHINE,1},
 {"Hundred Meetings","Catch 100 species",DEX,100,ITEM_GROWTH_MACHINE,2},
 {"Kanto Complete","Catch all 151 species",DEX,151,ITEM_MASTER,1},
 {"Shining Meeting","Catch 1 shiny species",SHINY,1,ITEM_MOON_STONE,1},
 {"Chasing Starlight","Catch 3 shiny species",SHINY,3,ITEM_MASTER,1},
 {"Moment of Growth","Evolve a Pokemon once",EVOLUTION,1,ITEM_FIRE_STONE,1},
 {"Evolution Scholar","Evolve Pokemon 5 times",EVOLUTION,5,ITEM_LINK_MACHINE,1},
 {"First Badge","Earn 1 gym badge",BADGES,1,ITEM_MILK,5},
 {"Gym Challenger","Earn 4 gym badges",BADGES,4,ITEM_MOON_STONE,1},
 {"Eight Badges","Earn 8 gym badges",BADGES,8,ITEM_GROWTH_MACHINE,1},
 {"League Champion","Defeat Champion Blue",CHAMPION,1,ITEM_MASTER,1},
 {"Top of Mt. Silver","Defeat Red on Mt. Silver",RED,1,ITEM_LINK_MACHINE,2},
 {"Johto Invitation","Reach Johto, pick a partner",ARRIVAL,1,ITEM_BERRY,5},
 {"Egg Surprise","Hatch 1 Togepi",EGG,1,ITEM_JOY_COOKIE,5},
 {"New Evolution","Do 1 cross-gen evolution",CROSS_GEN,1,ITEM_UPGRADE,1},
 {"Johto Rising Star","Earn 2 Johto badges",JOHTO_BADGES,2,ITEM_SUN_STONE,1},
 {"Johto Traveler","Earn 4 Johto badges",JOHTO_BADGES,4,ITEM_KINGS_ROCK,1},
 {"Heart of Steel","Earn 6 Johto badges",JOHTO_BADGES,6,ITEM_METAL_COAT,1},
 {"Johto Eight Badges","Earn 8 Johto badges",JOHTO_BADGES,8,ITEM_DRAGON_SCALE,1},
 {"Johto Collector","Catch 25 Johto species",JOHTO_DEX,25,ITEM_UPGRADE,1},
 {"Johto Researcher","Catch 50 Johto species",JOHTO_DEX,50,ITEM_METAL_COAT,2},
 {"Johto Complete","Catch 100 Johto species",JOHTO_DEX,100,ITEM_MASTER,1},
 {"Dex Across the Sea","National Dex: 200 species",NATIONAL_DEX,200,ITEM_UPGRADE,2},
 {"National Complete","National Dex: 251 species",NATIONAL_DEX,251,ITEM_MASTER,1},
 {"Johto Starlight","Catch 1 Johto shiny",JOHTO_SHINY,1,ITEM_FRIEND,5},
 {"Ruins Answer","Solve 4 ruin glyph sets",RUINS,4,ITEM_KINGS_ROCK,1},
 {"Return from the Deep","Clear a Whirl Deep run",DEEP,1,ITEM_DRAGON_SCALE,1},
 {"Research Master","Pass Elm's final trial",ELM,1,ITEM_MASTER,1},
 {"Wings Through Wind","Defeat Champion Falkner",CHAMP_GYM,1,ITEM_FAST,5},
 {"Nest Breaker","Defeat Champion Bugsy",CHAMP_GYM,1,ITEM_SUN_STONE,1},
 {"Cheer Breakthrough","Defeat Champion Whitney",CHAMP_GYM,1,ITEM_JOY_COOKIE,5},
 {"Dual-Phase Solution","Defeat Champion Morty",CHAMP_GYM,1,ITEM_MOON_STONE,1},
 {"Heavy Punch Counter","Defeat Champion Chuck",CHAMP_GYM,1,ITEM_LINK_MACHINE,1},
 {"Steel Armor Breaker","Defeat Champion Jasmine",CHAMP_GYM,1,ITEM_METAL_COAT,1},
 {"Cold Wave Ender","Defeat Champion Pryce",CHAMP_GYM,1,ITEM_WATER_STONE,1},
 {"Dragon Relay","Defeat Champion Clair",CHAMP_GYM,1,ITEM_DRAGON_SCALE,1},
 {"Johto Champion Tour","Beat 8 Champion Leaders",CHAMP_ALL,8,ITEM_MASTER,1},
 {"Legend Answer","Pass Elm's legend trial",CHAMP_ELM,1,ITEM_UPGRADE,2},
 {"Red Bond","Bring back the red Pokemon",RED_LAKE,1,ITEM_FRIEND,5},
};
const achievement_info_t *achievement_info(unsigned id) { return id<ACHIEVEMENT_COUNT?&INFO[id]:0; }
void achievement_view(achievement_view_t *out,const achievement_store_t *store,const dex_t *dex,uint16_t defeated) {
 *out=(achievement_view_t){.store=*store,.defeated=defeated};
 for(unsigned i=1;i<=151;i++)out->caught+=dex_is_caught(dex,i);
 for(unsigned i=0;i<DEX_SPECIES;i++){
  unsigned shiny=!!(dex->shiny_caught[i/8]&(1u<<(i%8)));out->shiny+=shiny;
  if(i>=151){out->johto_caught+=dex_is_caught(dex,i+1);out->johto_shiny+=shiny;}
 }
}
unsigned achievement_progress(const achievement_view_t *v,unsigned id) {
 const achievement_info_t *a=achievement_info(id);if(!a||!v)return 0;
 unsigned n=0;
 switch(a->kind){case DEX:n=v->caught;break;case SHINY:n=v->shiny;break;case EVOLUTION:n=v->store.evolutions;break;
 case BADGES:for(unsigned i=0;i<8;i++)n+=!!(v->defeated&(1u<<i));break;
 case ARRIVAL:n=v->johto.phase>=JOHTO_ACTIVE;break;
 case EGG:n=v->johto.eggs_hatched;break;case CROSS_GEN:n=v->store.cross_gen_evolutions;break;
 case JOHTO_BADGES:n=johto_badges(&v->johto);break;case JOHTO_DEX:n=v->johto_caught;break;
 case NATIONAL_DEX:n=v->caught+v->johto_caught;break;case JOHTO_SHINY:n=v->johto_shiny;break;
 case RUINS:for(unsigned i=0;i<4;i++)n+=!!(v->ruins&(1u<<i));break;
 case DEEP:n=v->sea_clears;break;
 case CHAMP_GYM:n=!!(v->champion_gyms&(1u<<(id-32)));break;
 case CHAMP_ALL:for(unsigned i=0;i<8;i++)n+=!!(v->champion_gyms&(1u<<i));break;
 case CHAMP_ELM:n=v->champion_elm;break;case RED_LAKE:n=v->lake_stage==LAKE_RECEIVED;break;
 case ELM:n=v->johto.phase==JOHTO_COMPLETE;break;
 case CHAMPION:n=!!(v->defeated&(1u<<12));break;case RED:n=!!(v->defeated&(1u<<13));break;}
 return n>a->target?a->target:n;
}
achievement_claim_t achievement_claim(achievement_store_t *s,inventory_t *bag,const achievement_view_t *v,unsigned id) {
 const achievement_info_t *a=achievement_info(id);if(!a||id>=32)return ACH_CLAIM_LOCKED;
 if(s->claimed&(1u<<id))return ACH_CLAIM_ALREADY;
 if(achievement_progress(v,id)<a->target)return ACH_CLAIM_LOCKED;
 unsigned total=(unsigned)bag->quantity[a->item]+a->quantity;
 unsigned cap=items_capacity(a->item);
 // New rare rewards remain claimable when their stack is full.
 if(id>=16&&total>cap)return ACH_CLAIM_FULL;
 bag->quantity[a->item]=total>cap?cap:total;
 s->claimed|=1u<<id;
 return total>cap?ACH_CLAIM_CAPPED:ACH_CLAIM_OK;
}

bool achievement_claimed(const achievement_view_t *v,unsigned id){return id<32?!!(v->store.claimed&(1u<<id)):id<ACHIEVEMENT_COUNT?!!(v->endgame_claimed&(1u<<(id-32))):false;}
achievement_claim_t achievement_claim_extended(achievement_store_t *s,endgame_t *end,inventory_t *bag,const achievement_view_t *v,unsigned id){
 if(id<32)return achievement_claim(s,bag,v,id);
 const achievement_info_t *a=achievement_info(id);if(!a||!end)return ACH_CLAIM_LOCKED;
 if(end->claimed&(1u<<(id-32)))return ACH_CLAIM_ALREADY;
 if(achievement_progress(v,id)<a->target)return ACH_CLAIM_LOCKED;
 unsigned total=bag->quantity[a->item]+a->quantity;if(total>items_capacity(a->item))return ACH_CLAIM_FULL;
 bag->quantity[a->item]=total;end->version=1;end->claimed|=1u<<(id-32);return ACH_CLAIM_OK;
}
