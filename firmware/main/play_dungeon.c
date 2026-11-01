#include "dungeon.h"
#include "assets.h"
#include "game_ui.h"
#include "screen.h"
#include "render.h"
#include "esp_timer.h"
#include "presentation_clock.h"
#include "screen_idle.h"
#include "lvgl.h"
#include "sfx.h"
#include "music_director.h"
#include <stdio.h>
static unsigned selected,chosen_count;
static uint8_t chosen[3];
static world_party_t owned;
static bool selecting,lobby,details,failed,themes,collecting,discarding;
static unsigned chosen_theme;static bool challenge_mode;
static uint8_t theme_ids[10];static unsigned theme_count;
static void theme_options(void){theme_count=0;for(unsigned i=0;i<10;i++)if(dungeon_theme_open(i))theme_ids[theme_count++]=i;}
void play_dungeon_open_region(unsigned map){chosen_theme=map>=4&&map<EXPLORATION_MAPS?map-3:0;challenge_mode=false;nav_go(PAGE_DUNGEON);}
static unsigned reward_frame;
static presentation_clock_t reward_clock;
static void reward_begin(void){reward_frame=0;presentation_clock_start(&reward_clock,(uint32_t)(esp_timer_get_time()/1000));}
static const char *failure_hint;
static lv_timer_t *reward_timer;
static unsigned receipt_items(void){unsigned count=0;for(unsigned i=0;i<ITEM_LEGACY_COUNT;i++)count+=dungeon_get()->receipt.items.quantity[i]>0;return count;}
static void label(unsigned id,char *buf,size_t size){species_t sp;if(assets_species(id,&sp))snprintf(buf,size,"%.*s",sp.name_zh_len,sp.name_zh);else snprintf(buf,size,"#%u",id);}
static unsigned options(void){const dungeon_t *d=dungeon_get();return collecting?(discarding?1:2):themes?theme_count:selecting?owned.count+1:details?DUNGEON_CARD_COUNT:lobby?8:d->phase==DUNGEON_SETTLEMENT?(receipt_items()?(receipt_items()+2)/3:1):d->phase==DUNGEON_FORK?2:d->phase==DUNGEON_CAMP||d->phase==DUNGEON_REWARD||d->phase==DUNGEON_TRAIL?3:1;}
static void draw(void){
 const dungeon_t *d=dungeon_get();char text[72],name[40];
 for(int y=0;y<SCREEN_H;y+=SCREEN_BAND_H){
  screen_band_clear(GAME_UI_BG);
  if(collecting){
   exploration_view_t v;world_exploration_snapshot(&v);unsigned kinds=0;
   for(unsigned i=0;i<ITEM_LEGACY_COUNT;i++)kinds+=v.regions.pending_items.quantity[i]!=0;
   game_ui_title(y,discarding?"Discard finds?":"Stored finds","");
   snprintf(text,sizeof(text),"%u item types waiting",kinds);game_ui_text_centered(y,8,70,224,16,text,GAME_UI_INK);
   if(v.regions.pending_partner.species_id){label(v.regions.pending_partner.species_id,name,sizeof(name));snprintf(text,sizeof(text),"%s%s",v.regions.pending_partner.is_shiny?"Shiny ":"Pokemon ",name);}
   else snprintf(text,sizeof(text),"No Pokemon waiting");
   game_ui_text_fitted(y,12,106,216,text,GAME_UI_ACCENT);
   game_ui_text_fitted(y,12,146,216,discarding?"Discarded finds are gone":"Free bag and queue space",GAME_UI_MUTED);
   const char *labels[]={discarding?"Discard":"Claim what fits","Discard these finds"};
   game_ui_box(y,8,190,224,64);for(unsigned i=0;i<options();i++){render_text(40,198+i*28-y,labels[i],GAME_UI_INK);if(i==selected)game_ui_cursor(y,18,202+i*28);}
  }else if(themes){
   game_ui_title(y,"Pick a dungeon","");
   unsigned first=selected/4*4;
   for(unsigned i=0;i<4&&first+i<theme_count;i++){
    unsigned theme=theme_ids[first+i];const exploration_region_t *r=exploration_region(theme+3);
    game_ui_box(y,8,48+i*50,224,46);game_ui_text_fitted(y,36,54+i*50,192,dungeon_theme_name(theme),GAME_UI_INK);
    if(r)snprintf(text,sizeof(text),"Lv%u-%u. Clear to catch",r->min_level,r->max_level);
    else snprintf(text,sizeof(text),theme==9?"Lv70-100 multi-theme run":"Scales with your Pokemon");
    game_ui_text_fitted(y,36,74+i*50,192,text,GAME_UI_MUTED);if(first+i==selected)game_ui_cursor(y,16,62+i*50);
   }
   game_ui_text_fitted(y,12,260,216,"Badges/research open more",GAME_UI_MUTED);
  }else if(selecting){
   snprintf(text,sizeof(text),"Picked %u/3",chosen_count);game_ui_title(y,"Companions",text);
   unsigned index=selected<owned.count?selected:chosen_count?chosen[0]:0;
   const mon_t *mon=&owned.members[index];unsigned id=mon->species_id;uint8_t size;species_t sp;const uint8_t *sprite=assets_front_form(id,mon->special.traits.form,&size);
   if(sprite&&assets_species(id,&sp)){uint16_t pal[4];assets_palette_variant(sp.palette,mon->flags&1,pal);game_ui_sprite_centered(y,88,36,64,64,sprite,size,size,1,pal);}
   for(unsigned i=0;i<owned.count;i++){
    bool picked=false;for(unsigned j=0;j<chosen_count;j++)picked|=chosen[j]==i;
    const mon_t *m=&owned.members[i];label(m->species_id,name,sizeof(name));
    snprintf(text,sizeof(text),"%s%s%s Lv%u",picked?"Picked ":"",name,m->flags&1?"★":"",m->level);
    game_ui_text_fitted(y,36,106+i*22,196,text,GAME_UI_INK);if(selected==i)game_ui_cursor(y,16,110+i*22);
   }
   snprintf(text,sizeof(text),"Go (%u) costs 20 stamina",chosen_count);
   game_ui_text_fitted(y,36,106+owned.count*22,196,text,chosen_count?GAME_UI_ACCENT:GAME_UI_MUTED);
   if(selected==owned.count)game_ui_cursor(y,16,110+owned.count*22);
   game_ui_text_fitted(y,8,260,224,"Pick 1-3. C: select/undo",GAME_UI_MUTED);
  }else if(lobby){
   game_ui_title(y,dungeon_theme_name(chosen_theme),challenge_mode?"Hard":"Normal");
   world_t w;world_snapshot(&w);snprintf(text,sizeof(text),"Stamina %u/100  Entry 20",nurture_stamina_points(&w.pet));game_ui_text_centered(y,8,44,224,16,text,GAME_UI_MUTED);
   const char *labels[]={"Continue run","Pick Pokemon, new run","Pick a themed dungeon",challenge_mode?"Switch to Normal":"Switch to Hard","Claim stored finds","View run boosts","End run, keep finds","Back to menu"};
   unsigned first=selected/4*4;
   for(unsigned i=0;i<4;i++){render_text(40,80+i*34-y,labels[first+i],GAME_UI_INK);if(selected==first+i)game_ui_cursor(y,20,84+i*34);}
   snprintf(text,sizeof(text),d->phase?"Current run %s": "Pick Pokemon to bring",dungeon_theme_name(d->theme));game_ui_text_fitted(y,12,228,216,text,GAME_UI_MUTED);
   game_ui_text_fitted(y,12,252,216,"Hard mode after first clear",GAME_UI_MUTED);
  }else if(details){
   game_ui_title(y,"Run boosts","");game_ui_text_centered(y,8,84,224,16,dungeon_cards[selected],GAME_UI_INK);
   game_ui_text_fitted(y,12,124,216,dungeon_desc[selected],GAME_UI_INK);
   game_ui_text_centered(y,8,172,224,16,d->cards&(1u<<selected)?"Gained":"None yet",GAME_UI_ACCENT);
   snprintf(text,sizeof(text),"%u/%u",selected+1,DUNGEON_CARD_COUNT);game_ui_text_centered(y,8,224,224,16,text,GAME_UI_MUTED);
  }else if(d->phase==DUNGEON_SETTLEMENT){
   game_ui_title(y,d->pending?"Reward pending save":!d->run_id?"Old trial record":d->receipt.first_clear?"First clear!":d->receipt.first_elite?"First Elite win!":"Finds banked","");
   snprintf(text,sizeof(text),"Companion EXP +%lu",(unsigned long)(d->receipt.xp*reward_frame/24));game_ui_text_centered(y,8,52,224,16,text,GAME_UI_ACCENT);
   game_ui_meter(y,24,80,192,reward_frame*100/24);
   unsigned row=0,index=0;
   for(unsigned i=0;i<ITEM_LEGACY_COUNT;i++)if(d->receipt.items.quantity[i]){
    if(index/3==selected){snprintf(text,sizeof(text),"%s x%u",items_info(i)->name,d->receipt.items.quantity[i]);game_ui_text_fitted(y,24,116+row++*28,192,text,GAME_UI_INK);}index++;
   }
   if(!index)game_ui_text_centered(y,8,128,224,16,"No bonus items",GAME_UI_MUTED);
   if(d->receipt.partner_species){
    label(d->receipt.partner_species,name,sizeof(name));
    snprintf(text,sizeof(text),"%s%s",d->receipt.partner_shiny?"Found shiny ":"Found ",name);
    game_ui_text_fitted(y,12,192,216,text,GAME_UI_ACCENT);
   }
   snprintf(text,sizeof(text),"Run EXP total %lu",(unsigned long)d->xp);game_ui_text_centered(y,8,212,224,16,text,GAME_UI_INK);
   game_ui_text_centered(y,8,240,224,16,!d->run_id?"Old trial. No rewards":d->pending?"Reward unsaved. C: retry":d->receipt.partner_species?(d->theme?"Saved. Claim at entrance":"Added to queue. Go catch it"):d->receipt.full?(d->theme?"Items stored. Claim later":"Some items full. Not added"):"Finds kept if you lose",GAME_UI_MUTED);
   game_ui_footer(y,"A/B page C Next HoldB Back");screen_push_band(y);continue;
  }else{
   snprintf(text,sizeof(text),"%u/8",d->node+1);game_ui_title(y,dungeon_node_name(),text);
   game_ui_text_centered(y,8,44,224,16,d->phase==DUNGEON_TRAIL?"Pick where to search":d->phase==DUNGEON_REWARD?"Pick a run boost":d->phase==DUNGEON_FORK?"Pick the path ahead":d->phase==DUNGEON_CAMP?"Camp: one choice only":d->phase==DUNGEON_WON?"Beat the boss to clear":"Run over. Try again",GAME_UI_INK);
   const char *labels[3],*hint="";unsigned count=options();
   if(d->phase==DUNGEON_TRAIL){
    const exploration_region_t *r=exploration_region(d->theme==9?4+(d->node+d->boss_variant)%8:d->theme+3);
    for(unsigned i=0;i<3;i++){labels[i]=r->directions[i];}
    hint="4-star+ shiny odds 1/16";
   }else if(d->phase==DUNGEON_REWARD){for(unsigned i=0;i<3;i++)labels[i]=dungeon_cards[d->choices[i]];hint=dungeon_desc[d->choices[selected]];}
   else if(d->phase==DUNGEON_FORK){labels[0]="Face the Path Guard";labels[1]=d->node==1?(d->theme?"Find a shortcut":"Cross the thorns"):"Search for supplies";hint=!selected?"Win for EXP and boosts":d->node==1?"Lose 12% HP for a boost":"Heal 15% HP, get Berries";}
   else if(d->phase==DUNGEON_CAMP){labels[0]="Rest and cleanse";labels[1]="Revive the fallen";labels[2]="Study dungeon boosts";hint=selected==0?(d->cards&(1u<<9)?"Survivors heal 45% HP":"Survivors heal 25% HP"):selected==1?"First fallen heals 35%":"Skip heal, pick a boost";}
   else{labels[0]="Back to entrance";snprintf(text,sizeof(text),"Kept EXP %lu, %u wins",(unsigned long)d->xp,d->wins);hint=text;}
   game_ui_box(y,8,72,224,104);
   for(unsigned i=0;i<count;i++){render_text(40,84+i*28-y,labels[i],GAME_UI_INK);if(selected==i)game_ui_cursor(y,20,88+i*28);}
   game_ui_text_fitted(y,12,184,216,hint,GAME_UI_MUTED);
   for(unsigned i=0;i<d->count;i++){const combat_mon_t *m=&d->battle.session.sides[0].mons[i];label(d->ids[i],name,sizeof(name));snprintf(text,sizeof(text),"%s %u/%u",name,m->hp,m->max_hp);game_ui_text_fitted(y,12,212+i*20,216,text,m->hp?GAME_UI_INK:GAME_UI_MUTED);}
   if(d->phase==DUNGEON_WON||d->phase==DUNGEON_LOST)game_ui_text_centered(y,8,264,224,16,"Items banked. Try new picks",GAME_UI_ACCENT);
  }
  game_ui_footer(y,failed?(failure_hint?failure_hint:"Check Pokemon, stamina, save"):GAME_UI_NAV_HINT);screen_push_band(y);
 }
}
static void launch(void){dungeon_set_playing(true);nav_go(PAGE_TRAINER);}
bool play_dungeon_screen_busy(void){return !lobby&&!selecting&&dungeon_get()->phase==DUNGEON_SETTLEMENT&&reward_frame<24;}
static void reward_tick(lv_timer_t *t){
 (void)t;bool busy=play_dungeon_screen_busy();
 unsigned ms=presentation_clock_sample(&reward_clock,(uint32_t)(esp_timer_get_time()/1000),!busy||screen_idle_is_off());
 if(busy&&!screen_idle_is_off()){reward_frame=presentation_sample(ms,50,24);draw();}
}
void play_dungeon_enter(void){
 dungeon_load();dungeon_set_playing(false);selected=chosen_count=0;selecting=details=failed=themes=collecting=discarding=false;if(!dungeon_theme_open(chosen_theme))chosen_theme=0;
 unsigned phase=dungeon_get()->phase;lobby=phase==DUNGEON_EMPTY||phase==DUNGEON_BATTLE||phase==DUNGEON_ENTRY;
 reward_begin();reward_timer=lv_timer_create(reward_tick,50,NULL);
 music_director_play(phase==DUNGEON_WON||phase==DUNGEON_REWARD||phase==DUNGEON_SETTLEMENT?MUSIC_TRAINER_WIN:phase==DUNGEON_CAMP?MUSIC_CENTER:MUSIC_ROUTE);
 screen_set_redraw(draw);draw();
}
void play_dungeon_exit(void){if(reward_timer){lv_timer_delete(reward_timer);reward_timer=NULL;}}
void play_dungeon_key(bsp_btn_t b,bsp_btn_ev_t e){
 failed=false;failure_hint=NULL;
 if(nav_return(b,e)){if(discarding){discarding=false;selected=1;draw();return;}if(lobby){nav_go(PAGE_MENU);return;}lobby=true;selecting=details=themes=collecting=false;selected=0;draw();return;}
 if(nav_direction(b,e)){selected=nav_list_selection(b,e,options(),selected);draw();return;}
 if(!nav_confirm(b,e))return;
 if(collecting){
  if(discarding){if(world_region_discard()){collecting=discarding=false;lobby=true;selected=4;}else failed=true;}
  else if(selected==1){discarding=true;selected=0;}
  else if(!world_region_collect())failed=true;
 }else if(themes){chosen_theme=theme_ids[selected];challenge_mode=false;themes=false;lobby=true;selected=1;}
 else if(selecting){
  if(selected==owned.count){
   if(chosen_count&&dungeon_new_theme(chosen,chosen_count,(uint32_t)esp_timer_get_time(),chosen_theme,challenge_mode)){launch();return;}
   failed=true;
  }else{
   unsigned i=0;while(i<chosen_count&&chosen[i]!=selected)i++;
   if(i<chosen_count){for(;i+1<chosen_count;i++)chosen[i]=chosen[i+1];chosen_count--;}
   else if(chosen_count<3)chosen[chosen_count++]=selected;
  }
 }else if(lobby){
  if(selected==7){nav_go(PAGE_MENU);return;}
  if(selected==6){if(dungeon_abandon()){selected=0;}else failed=true;draw();return;}
  if(selected==1){world_party_snapshot(&owned);selecting=true;lobby=false;chosen_count=selected=0;}
  else if(selected==2){themes=true;lobby=false;theme_options();selected=0;}
  else if(selected==3){exploration_view_t v;world_exploration_snapshot(&v);if(chosen_theme&& (chosen_theme==9||v.regions.region[chosen_theme-1].clears))challenge_mode=!challenge_mode;else {failed=true;failure_hint="Hard mode after clearing it";}}
  else if(selected==4){collecting=true;lobby=false;selected=0;}
  else if(selected==5){details=true;lobby=false;selected=0;}
  else if(dungeon_get()->phase==DUNGEON_BATTLE||dungeon_get()->phase==DUNGEON_ENTRY){if(dungeon_resume()){launch();return;}failed=true;}
  else if(dungeon_get()->phase){lobby=false;selected=0;}else failed=true;
 }else if(details){lobby=true;details=false;selected=0;}
 else if(dungeon_get()->phase==DUNGEON_WON||dungeon_get()->phase==DUNGEON_LOST){lobby=true;selected=0;}
 else if(dungeon_get()->phase==DUNGEON_SETTLEMENT&&reward_frame<24){reward_frame=24;}
 else if(dungeon_choose(dungeon_get()->phase==DUNGEON_SETTLEMENT?0:selected)){if(dungeon_get()->phase==DUNGEON_BATTLE){launch();return;}selected=0;reward_begin();}else failed=true;
 draw();
}
