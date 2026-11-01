#include "johto.h"
#include <limits.h>
#include <string.h>

const johto_route_t JOHTO_ROUTES[8]={
 {"Sprout Fields","Starter Research",0,35,44,{161,163,165,167,179,187,191,194,172,173,174,183},12},
 {"Ilex Woods","Bug Watching",1,38,48,{165,166,167,168,177,187,188,190,193,204,206,214},12},
 {"Goldenrod Fields","Pokemon Rescue",2,42,52,{189,179,180,183,184,185,191,192,209,216,231,241},12},
 {"Alph Ruins","Ruin Survey",3,48,58,{177,178,198,200,201,202,203,207,213,234,235,236},12},
 {"Burned Tower","Beast Tracks",4,52,62,{156,162,164,181,195,197,205,210,215,218,228,229},12},
 {"Snowy Path","Ice Expedition",5,58,68,{215,217,220,221,225,238,239,240,246,247,224,232},12},
 {"Whirl Islands","Silver Wing Survey",6,64,76,{158,159,170,171,184,195,211,222,223,224,226,230},12},
 {"Bell Tower","Rainbow Wing Survey",7,70,82,{154,157,160,169,182,186,196,199,208,212,227,248},12},
};
const johto_trainer_t JOHTO_TRAINERS[11]={
 {"Falkner","Zephyr Badge","Behold the power of flight!","Your team has learned the wind.",{164,176,18},{40,41,42},3},
 {"Bugsy","Hive Badge","Small bodies hold big potential.","This badge honors your patience.",{166,168,214,212},{44,45,46,47},4},
 {"Whitney","Plain Badge","Ready to face our teamwork?","Next time, we'll be stronger!",{210,203,217,241},{49,50,51,52},4},
 {"Morty","Fog Badge","See what's ahead, and within.","You may now survey the legends.",{200,94,198,197},{54,55,56,57},4},
 {"Chuck","Storm Badge","True power comes from training.","Go on with your team!",{237,107,106,62,214},{58,59,60,61,62},5},
 {"Jasmine","Mineral Badge","Gentle can also be strong.","The Silver Wing lies across the sea.",{82,205,227,212,208},{64,65,66,67,68},5},
 {"Pryce","Glacier Badge","Storms teach perseverance.","Your passion melted the ice.",{87,91,215,225,221},{70,71,72,73,74},5},
 {"Clair","Rising Badge","Show us your bond.","You've proven yourself. Finish the survey!",{130,148,181,248,230,149},{75,76,77,78,79,80},6},
 {"Prof. Elm","Johto Research Master","Show me all you've learned.","The answer is in your journey.",{212,248,243,245,249,250},{88,88,89,90,91,92},6},
 {"Elm: Guard","Relay Defense","Read the defense, find a gap.","Each choice advances our research.",{212,248,243,245,249,250},{88,88,89,90,91,92},6},
 {"Elm: Gale","Fast Offense","Speed and power need teamwork.","Next time, try a new team.",{212,248,243,245,249,250},{88,88,89,90,91,92},6},
};

void johto_init(johto_state_t *s){memset(s,0,sizeof(*s));s->version=1;}
unsigned johto_badges(const johto_state_t *s){unsigned n=0;for(unsigned v=s->badges;v;v>>=1)n+=v&1;return n;}
bool johto_valid(const johto_state_t *s){
 if(!s||s->version!=1||s->phase>JOHTO_COMPLETE||s->reserved||s->final_reward_claimed>1)return false;
 if(s->badges!=((1u<<johto_badges(s))-1u))return false;
 if(s->phase<JOHTO_ACTIVE&&(s->starter||s->badges||s->explorations||s->egg_status))return false;
 if(s->phase>=JOHTO_ACTIVE&&s->starter!=152&&s->starter!=155&&s->starter!=158)return false;
 if(s->egg_status>JOHTO_EGG_READY||s->egg_steps>JOHTO_EGG_STEPS)return false;
 if(s->egg_status==JOHTO_EGG_NONE&&(s->egg_steps||s->egg_seed))return false;
 if(s->egg_status==JOHTO_EGG_READY&&s->egg_steps!=JOHTO_EGG_STEPS)return false;
 if(s->egg_status==JOHTO_EGG_INCUBATING&&s->egg_steps>=JOHTO_EGG_STEPS)return false;
 if(s->final_reward_claimed&&s->phase!=JOHTO_COMPLETE)return false;
 if(s->phase==JOHTO_COMPLETE&&s->badges!=255)return false;
 for(unsigned i=0;i<6;i++)if(s->legend_clues[i]>3)return false;
 return true;
}
bool johto_offer(johto_state_t *s,uint16_t defeated){
 unsigned n=0;for(unsigned i=0;i<8;i++)n+=(defeated>>i)&1;
 if(n<4||s->phase!=JOHTO_HIDDEN)return false;
 s->phase=JOHTO_INVITED;return true;
}
bool johto_depart(johto_state_t *s){if(s->phase!=JOHTO_INVITED)return false;s->phase=JOHTO_VOYAGE;return true;}
bool johto_choose_starter(johto_state_t *s,uint16_t species){
 if(s->phase!=JOHTO_VOYAGE||(species!=152&&species!=155&&species!=158))return false;
 s->starter=species;s->phase=JOHTO_ACTIVE;return true;
}
bool johto_route_open(const johto_state_t *s,unsigned route){return route<8&&s->phase>=JOHTO_ACTIVE&&johto_badges(s)>=JOHTO_ROUTES[route].badges;}
uint8_t johto_route_level(unsigned route,unsigned level,uint32_t roll){
 if(route>=8)return 0;
 unsigned lo=JOHTO_ROUTES[route].min_level,hi=JOHTO_ROUTES[route].max_level;
 if(level<lo)level=lo;
 if(level>hi)level=hi;
 // A route always enforces its floor/ceiling; +/-2 variation stays in band.
 int result=(int)level+(int)(roll%5)-2;
 return result<(int)lo?lo:result>(int)hi?hi:(unsigned)result;
}
bool johto_gym_open(const johto_state_t *s,unsigned gym){
 return gym<8&&s->phase>=JOHTO_ACTIVE&&johto_badges(s)>=gym;
}
bool johto_gym_win(johto_state_t *s,unsigned gym){
 if(!johto_gym_open(s,gym)||(s->badges&(1u<<gym)))return false;
 s->badges|=1u<<gym;return true;
}
bool johto_explore(johto_state_t *s,unsigned route,uint32_t action,bool paid){
 if(!paid||!action||action<=s->last_action||!johto_route_open(s,route)||s->explorations==UINT32_MAX)return false;
 s->last_action=action;s->explorations++;
 if(s->research[route]<UINT16_MAX)s->research[route]++;
 if(s->egg_status==JOHTO_EGG_INCUBATING&&++s->egg_steps==JOHTO_EGG_STEPS)s->egg_status=JOHTO_EGG_READY;
 // Story-specific route progression is committed by the world transaction.
 return true;
}
bool johto_receive_egg(johto_state_t *s,uint32_t seed){
 if(s->phase<JOHTO_ACTIVE||!s->badges||s->egg_status||s->explorations<s->next_egg_at)return false;
 s->egg_status=JOHTO_EGG_INCUBATING;s->egg_seed=seed;s->egg_steps=0;return true;
}
bool johto_egg_shiny(const johto_state_t *s){return s->egg_status&&s->egg_seed%JOHTO_EGG_SHINY_DENOMINATOR==0;}
bool johto_hatch(johto_state_t *s,bool received){
 if(s->egg_status!=JOHTO_EGG_READY||!received)return false;
 s->egg_status=JOHTO_EGG_NONE;s->egg_steps=0;s->egg_seed=0;
 if(s->eggs_hatched<UINT16_MAX)s->eggs_hatched++;
 s->next_egg_at=s->explorations>UINT32_MAX-12?UINT32_MAX:s->explorations+12;return true;
}
bool johto_legend_ready(const johto_state_t *s,unsigned legend){return legend<6&&s->legend_clues[legend]==3;}
bool johto_final_open(const johto_state_t *s,uint16_t defeated){
 if(s->phase<JOHTO_ACTIVE||s->badges!=255||(defeated&0x20ff)!=0x20ff)return false;
 for(unsigned i=0;i<5;i++)if(!johto_legend_ready(s,i))return false;
 return true;
}
bool johto_final_win(johto_state_t *s,uint16_t defeated){
 if(!johto_final_open(s,defeated)||s->phase==JOHTO_COMPLETE)return false;
 s->phase=JOHTO_COMPLETE;return true;
}
bool johto_claim_final(johto_state_t *s){
 if(s->phase!=JOHTO_COMPLETE||s->final_reward_claimed)return false;
 s->final_reward_claimed=1;return true;
}

const johto_research_reward_t JOHTO_REWARDS[JOHTO_REWARD_COUNT]={
 {"Ilex Woods Survey",1,1,6},{"Snowy Path Survey",5,6,6},{"Alph Ruins Survey",3,3,6},
 {"Whirl Islands Survey",6,6,6},{"Goldenrod Survey",2,2,6},
};
unsigned johto_reward_entitled(const johto_state_t *s,unsigned id){
 if(id>=JOHTO_REWARD_COUNT||s->phase<JOHTO_ACTIVE||johto_badges(s)<JOHTO_REWARDS[id].badges)return 0;
 return s->research[JOHTO_REWARDS[id].route]/JOHTO_REWARDS[id].step;
}
