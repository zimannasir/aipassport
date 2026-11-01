// P18: all displayed decisions mutate the same persisted activity as hardware.
#include <stdio.h>
#include "play.h"
#include "activity_play.h"
#include "nav.h"
#include "world.h"
#include "assets.h"
#include "game_ui.h"
#include "screen.h"
#include "render.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "presentation_clock.h"
#include "capture_presentation.h"
#include "screen_idle.h"
static presentation_clock_t animation_clock;
#include "lvgl.h"
#include "ball_assets.h"
#include "sfx.h"
#include "music_director.h"
static unsigned requested_kind;
static bool guided;
enum { MENU, TEAM, START, RUN, KEEP_CONFIRM, END_CONFIRM, BOX, BOX_CONFIRM, REWARDS, PRIZE, FINISHED };
static unsigned view,selected,kind,team_mask,hard,ball,animation,box_slot;
static bool playing,caught;
static activity_store_t state;
static world_party_t party;
static johto_state_t chapter;
static inventory_t bag;
static world_t world;
static mon_t replacing;
static activity_request_t request;
static activity_result_data_t result_data;
static activity_result_t error;
static lv_timer_t *timer;
static uint32_t capture_start, visible_capture_elapsed;
static cap_result_t visible_capture;
static mon_t capture_target;
static bool capture_ready;
#define CAPTURE_BAR_TOP 208
#define ACTIVITY_TICK_MS 40
static bool throw_pending, press_handled;
static unsigned final_score,final_nodes,final_won,final_kind,final_partner;
static uint32_t final_xp;
static inventory_t final_rewards;
bool activity_playing(void){return playing;}
void activity_set_playing(bool on){playing=on;}
void activity_battle_snapshot(trainer_store_t *out,world_party_t *out_party){world_activity_snapshot(&state);*out=state.run.battle;memset(out_party,0,sizeof(*out_party));out_party->count=state.run.count;world_party_snapshot(&party);for(unsigned i=0;i<state.run.count;i++)out_party->members[i]=party.members[state.run.slots[i]];}
static bool battle_action(unsigned op,unsigned choice,bool forced,trainer_event_t *event){activity_request_t q={.op=op,.choice=choice,.hard=forced};activity_result_data_t data;bool ok=world_activity_action(&q,&data)==ACT_OK;if(ok&&event)*event=data.event;return ok;}
bool activity_battle_step(trainer_event_t *out){return battle_action(ACTION_STEP,0,false,out);}
bool activity_battle_switch(unsigned slot,bool forced){return battle_action(ACTION_SWITCH,slot,forced,NULL);}
bool activity_battle_finish(void){world_activity_snapshot(&state);return activity_expedition(state.run.kind)&&(state.run.phase==ACT_SETTLE||battle_action(ACTION_BATTLE_END,0,false,NULL));}
bool activity_battle_retire(void){return battle_action(ACTION_RETIRE,0,false,NULL);}
static void refresh(void){world_activity_snapshot(&state);world_party_snapshot(&party);world_johto_snapshot(&chapter);world_inventory_snapshot(&bag);world_snapshot(&world);}
static bool action(unsigned op,unsigned choice){memset(&request,0,sizeof(request));request.op=op;request.choice=choice;request.box_slot=BOX_CAPACITY;request.seed=(uint32_t)esp_timer_get_time();error=world_activity_action(&request,&result_data);refresh();return error==ACT_OK;}
static const char *failure(void){return error==ACT_FULL?"No room. Reward kept":error==ACT_STAMINA?"Low stamina. Wait to recover":error==ACT_BUSY?"End the battle/dungeon first":error==ACT_STALE?"Pokemon changed. Pick again":error==ACT_SAVE_FAILED?"Save failed. C: retry":"Requirements not met";}
static void text(int y,int top,const char *s){if(render_text_width(s)>224)game_ui_text_fitted(y,8,top,224,s,GAME_UI_INK);else game_ui_text_centered(y,8,top,224,16,s,GAME_UI_INK);}
static void mon_name(const mon_t *m,char *s,size_t n){species_t sp;if(assets_species(m->species_id,&sp))snprintf(s,n,"%.*s%s",sp.name_zh_len,sp.name_zh,m->flags&1?"★":"");else snprintf(s,n,"No Pokemon");}
static void sprite(int y,int x,int top,int width,int height,const mon_t *m,int scale){species_t sp;uint8_t size;if(!assets_species(m->species_id,&sp))return;const uint8_t *p=assets_front_form(m->species_id,m->special.traits.form,&size);uint16_t pal[4];assets_palette_variant(sp.palette,m->flags&1,pal);if(p)game_ui_sprite_centered(y,x,top,width,height,p,size,size,scale,pal);}
static void options(int y,int top,const char *const *labels,unsigned count,int step){for(unsigned i=0;i<count;i++){render_text(32,top+i*step-y,labels[i],GAME_UI_INK);if(selected==i)game_ui_cursor(y,12,top+4+i*step);}}
static unsigned menu_ids(unsigned out[7]){unsigned n=0;if(state.run.kind)out[n++]=0;else {if(activity_open(ACT_BUG,&chapter))out[n++]=1;if(activity_open(ACT_SEA,&chapter))out[n++]=2;if(activity_open(ACT_ICE,&chapter))out[n++]=5;if(activity_open(ACT_TOWER,&chapter))out[n++]=6;}out[n++]=3;out[n++]=4;return n;}
static unsigned selected_level(void){unsigned n=0;for(unsigned i=0;i<party.count;i++)if((team_mask&(1u<<i))&&party.members[i].level>n)n=party.members[i].level;return n;}
static unsigned choices(void){
 const activity_run_t *r=&state.run;
 if(view==MENU){unsigned ids[7];return menu_ids(ids);}
 if(view==TEAM)return party.count+1;
 if(view==START)return activity_expedition(kind)?3:2;
 if(view==KEEP_CONFIRM||view==END_CONFIRM||view==BOX_CONFIRM||view==PRIZE)return 2;
 if(view==BOX)return BOX_CAPACITY;
 if(view==REWARDS)return ITEM_COUNT+1;
 if(view!=RUN)return 1;
 if(r->phase==ACT_PATH||r->phase==ACT_FIND)return 4;
 if(r->phase==ACT_COMPARE)return r->candidate.species_id?2:1;
 if(r->phase==ACT_CAMP||r->phase==ACT_RELAY||r->phase==ACT_DEEP_PATH)return 2;
 return 1;
}
/* Redraws (including USB screenshots) replay the last presented pointer.  The
 * input judgment uses this same sample, never a later clock after CLICK/NVS. */
static void draw_range(int first,int end){
 const activity_run_t *r=&state.run;
 for(int y=first/SCREEN_BAND_H*SCREEN_BAND_H;y<end;y+=SCREEN_BAND_H){screen_band_clear(GAME_UI_BG);char b[96];
 game_ui_title(y,(view==RUN||view==KEEP_CONFIRM||view==END_CONFIRM||view==BOX||view==BOX_CONFIRM)?activity_name(r->kind):"Johto events","");
 if(view==MENU){
  const char *labels[]={"Resume event","Bug Survey","Whirl Deep Expedition","Event rewards","Back to Johto","Ice Floor Rescue","Bell Tower Trial"};unsigned ids[7],n=menu_ids(ids);const char *rows[7];for(unsigned i=0;i<n;i++)rows[i]=labels[ids[i]];options(y,42,rows,n,29);
  text(y,234,johto_badges(&chapter)<2?"2 badges: Bug Survey":johto_badges(&chapter)<5?"5 badges: Ice Rescue":"Separate from explore combos");
  snprintf(b,sizeof(b),"Stamina %u/100",nurture_stamina_points(&world.pet));text(y,258,b);
 }else if(view==TEAM){
  for(unsigned i=0;i<party.count;i++){mon_name(&party.members[i],b,sizeof(b));char row[96];snprintf(row,sizeof(row),"%.60s Lv%u %s",b,party.members[i].level,team_mask&(1u<<i)?"Picked":"");game_ui_text_fitted(y,32,40+i*29,196,row,GAME_UI_INK);if(selected==i)game_ui_cursor(y,12,44+i*29);}
  text(y,224,"Pick, then go");if(selected==party.count)game_ui_cursor(y,12,228);
  snprintf(b,sizeof(b),"Pick %u of your Pokemon",kind==ACT_BUG?1:3);text(y,254,b);
 }else if(view==START){
  text(y,48,kind==ACT_BUG?"6 looks, up to 3 throws":"6 nodes, relay banks gains");text(y,76,kind==ACT_BUG?"Keep only one Pokemon":"Uses your move settings");
  snprintf(b,sizeof(b),"Entry costs %u stamina",kind==ACT_BUG?10:20);text(y,104,b);
  text(y,132,kind==ACT_BUG?"Lv35-50. Keep only one":(kind==ACT_ICE?"Normal 58-78 Hard 72-92":kind==ACT_TOWER?"Normal 70-92 Hard 82-100":"Normal 55-75 Hard 70-90"));
  text(y,160,selected_level()<activity_min_level(kind,hard)?"Level is low. Be careful":"Resumable after pause/restart");const char *labels[]={"Cancel", "Confirm and go",hard?"Mode: Hard":"Mode: Normal"};options(y,204,labels,choices(),24);
 }else if(view==REWARDS){
  unsigned top=selected/6*6;
  for(unsigned i=0;i<6&&top+i<ITEM_COUNT+1;i++){unsigned id=top+i;if(id==ITEM_COUNT)snprintf(b,sizeof(b),"3-run reward x%u",state.bug_choices);else snprintf(b,sizeof(b),"%s ×%u",items_info(id)->name,state.pending.quantity[id]);render_text(32,48+i*30-y,b,GAME_UI_INK);if(selected==id)game_ui_cursor(y,12,52+i*30);}
  snprintf(b,sizeof(b),"Valid surveys %u. Next %u/3",state.bug_completed,state.bug_completed%3);text(y,240,b);text(y,262,"Overflow items stay here");
 }else if(view==PRIZE){text(y,70,"Finish 3 valid surveys");text(y,102,"Pick an evolution item");const char *labels[]={"Sun Stone","King's Rock"};options(y,166,labels,2,40);
 }else if(view==KEEP_CONFIRM||view==END_CONFIRM||view==BOX_CONFIRM){
  text(y,80,view==KEEP_CONFIRM?"Replace the shiny candidate?":view==BOX_CONFIRM?"Replace this box Pokemon?":"End and settle the survey?");
  if(view==BOX_CONFIRM){mon_name(&replacing,b,sizeof(b));text(y,112,b);snprintf(b,sizeof(b),"Lv%u %s",replacing.level,replacing.flags&1?"Shiny":"Normal");text(y,144,b);text(y,176,"Old Pokemon and settings go");}
  else {text(y,132,view==KEEP_CONFIRM?"The old candidate isn't boxed":"Keeps candidate and rewards");}
  const char *labels[]={"Cancel","Confirm"};options(y,210,labels,2,30);
 }else if(view==BOX){
  text(y,38,"Box full. Pick a swap target");unsigned top=selected/6*6;for(unsigned i=0;i<6&&top+i<BOX_CAPACITY;i++){mon_t m;if(!world_box_at(top+i,&m))continue;mon_name(&m,b,sizeof(b));char row[96];snprintf(row,sizeof(row),"%u %.60s Lv%u",top+i+1,b,m.level);game_ui_text_fitted(y,28,66+i*30,204,row,GAME_UI_INK);if(selected==top+i)game_ui_cursor(y,8,70+i*30);}text(y,254,"Hold B: keep it, sort later");
 }else if(view==FINISHED){text(y,62,"Event settled");snprintf(b,sizeof(b),"%u looks, score %u",final_nodes,final_score);if(activity_expedition(final_kind))snprintf(b,sizeof(b),"%s",final_won?(final_kind==ACT_SEA?"Deep clear. Silver Wing logged":"Deep clear. Research reward"):"Expedition over");text(y,102,b);
  if(activity_expedition(final_kind)){snprintf(b,sizeof(b),"EXP gained %lu",(unsigned long)final_xp);text(y,136,b);if(final_won)text(y,168,"Theme Pokemon queued");}else {text(y,138,final_partner?"Pokemon added to party/box":"Survey log saved");snprintf(b,sizeof(b),"Valid surveys %u/3",state.bug_completed%3);text(y,168,b);}unsigned item=0;while(item<ITEM_COUNT&&!final_rewards.quantity[item])item++;if(item<ITEM_COUNT){snprintf(b,sizeof(b),"%s x%u and supplies",items_info(item)->name,final_rewards.quantity[item]);text(y,204,b);}text(y,230,"Overflow kept in event rewards");text(y,260,"[C] Back to events");
 }else if(view==RUN){
  snprintf(b,sizeof(b),"Node %u/6",r->node);text(y,38,b);
  if(r->phase==ACT_PATH){text(y,72,"Follow which trace?");const char *labels[]={activity_habitat(0),activity_habitat(1),activity_habitat(2),"Settle early"};options(y,108,labels,4,30);text(y,244,selected<3?activity_habitat_hint(selected):"Keep candidate and rewards");}
  else if(r->phase==ACT_FIND||animation){
   unsigned source=capture_source_frame(animation_clock.elapsed_ms);
   if(!animation||source<80||(!caught&&source>=CAPTURE_RESULT_SOURCE_FRAME))sprite(y,16,58,208,112,&r->found,2);
   if(animation){ui_art_t icon;capture_ball_pose_t pose=capture_ball_pose(animation_clock.elapsed_ms);
    if(assets_ui((source>=37&&source<97)||(!caught&&source>=CAPTURE_RESULT_SOURCE_FRAME)?"ball_open":"ball_24",&icon))
     render_sprite_2bpp_wh(pose.x-12,pose.y-12-y,icon.data,icon.w,icon.h,1,ball_assets_palette(ball));
    text(y,218,source<CAPTURE_RESULT_SOURCE_FRAME?"The ball wobbles...":caught?"Pokemon caught!":"It broke free!");}

   else {mon_name(&r->found,b,sizeof(b));text(y,169,b);snprintf(b,sizeof(b),"%s x%u, %u balls left",cap_ball_name(ball),bag.quantity[ball],ACTIVITY_BUG_TRIES-r->attempts);text(y,191,b);
    if(capture_ready)game_ui_capture_bar(y,CAPTURE_BAR_TOP,&visible_capture);
    else text(y,218,r->attempts>=ACTIVITY_BUG_TRIES?"No throws left this run":"Out of this ball. Switch");
    const char *labels[]={"Throw","Switch ball","Keep observing","Settle early"};for(unsigned i=0;i<4;i++){int xx=30+(i%2)*112,yy=244+(i/2)*18;render_text(xx,yy-y,labels[i],!i&&!capture_ready?GAME_UI_MUTED:GAME_UI_INK);if(selected==i)game_ui_cursor(y,xx-18,yy+4);}
   }
  }else if(r->phase==ACT_COMPARE){
   sprite(y,4,64,112,84,&r->candidate,1);sprite(y,124,64,112,84,&r->found,1);mon_name(&r->candidate,b,sizeof(b));game_ui_text_fitted(y,4,158,112,b,GAME_UI_INK);mon_name(&r->found,b,sizeof(b));game_ui_text_fitted(y,124,158,112,b,GAME_UI_INK);
   if(r->candidate.species_id){snprintf(b,sizeof(b),"Lv%u %u★",r->candidate.level,activity_rarity(r->candidate.species_id));render_text(8,183-y,b,GAME_UI_MUTED);}snprintf(b,sizeof(b),"Lv%u %u★",r->found.level,activity_rarity(r->found.species_id));render_text(128,183-y,b,GAME_UI_MUTED);
   snprintf(b,sizeof(b),"Candidate score %u / %u",r->candidate.species_id?20+activity_rarity(r->candidate.species_id)*10:0,20+activity_rarity(r->found.species_id)*10);text(y,203,b);
   const char *labels[]={r->candidate.species_id?"Keep candidate":"Take this Pokemon","Pick another"};options(y,230,labels,choices(),26);
  }else if(r->phase==ACT_SETTLE){
   if(r->kind==ACT_BUG){sprite(y,16,64,208,100,&r->candidate,2);unsigned score=activity_score(r);snprintf(b,sizeof(b),"Score %u %s",score,score>=90?"Gold":score>=50?"Silver":score>=20?"Bronze":"Survey log");text(y,180,b);snprintf(b,sizeof(b),"Silver 50 Gold 90. Seen %u",activity_observations(r));text(y,210,b);}
   else {text(y,82,r->won?"Whirl Deep cleared!":"Expedition ended");snprintf(b,sizeof(b),"EXP banked %lu",(unsigned long)r->banked_xp);text(y,122,b);snprintf(b,sizeof(b),"EXP unbanked %lu",(unsigned long)r->xp);text(y,156,b);text(y,194,r->won?"Dragon Scale, Water clues":"Banked gains stay");}
   text(y,253,"[C] Settle and bank");
  }else if(r->phase==ACT_CAMP){text(y,84,"Reached a fixed rest camp");text(y,116,"HP changes affect this run only");const char *labels[]={"Survivors heal 60%","Revive and heal 25%"};options(y,180,labels,2,38);}
  else if(r->phase==ACT_RELAY){snprintf(b,sizeof(b),"EXP banked %lu",(unsigned long)r->banked_xp);text(y,80,b);text(y,114,r->banked?"First-half loot is safe":"Unsaved. C: retry relay");text(y,148,"Deep failure loses extras");const char *labels[]={"Settle and return","Dive deeper"};options(y,208,labels,2,34);}
  else if(r->phase==ACT_DEEP_PATH){text(y,68,"Pick a deep route");text(y,100,"Water: Water foes, Dragon Scale");text(y,132,"Elite: Steel, bonus Metal Coat");text(y,164,"Elite foes get +2 levels");const char *labels[]={"Follow safe waters","Face the Steel Elite"};options(y,212,labels,2,30);}
  else{text(y,112,"Party ready");text(y,160,"[C] Keep battling");}
 }
 if(error){game_ui_box(y,4,242,232,38);game_ui_text_fitted(y,10,252,220,failure(),GAME_UI_INK);}
 game_ui_footer(y,throw_pending?"Throwing...":animation?"Throw result saved":GAME_UI_NAV_HINT);screen_push_band(y);
 }
}
static void repaint(void){draw_range(0,SCREEN_H);}
static void sample_pointer(void){
 const activity_run_t *r=&state.run;species_t sp;
 capture_ready=false;memset(&visible_capture,0,sizeof(visible_capture));
 if(r->kind!=ACT_BUG||r->phase!=ACT_FIND||r->attempts>=ACTIVITY_BUG_TRIES||
    ball>=CAP_BALL_COUNT||!bag.quantity[ball]||!assets_species(r->found.species_id,&sp))return;
 visible_capture_elapsed=(uint32_t)(esp_timer_get_time()/1000)-capture_start;
 cap_context_t ctx={r->members[0].level,r->found.level,sp.speed,sp.weight_hg};
 unsigned w=cap_window_width_context(sp.catch_rate,1024+r->members[0].intimacy*3,ball,100,&ctx);
 cap_sample_window(w,ball,visible_capture_elapsed,&visible_capture);
 capture_target=r->found;capture_ready=true;
}
static void draw(void){if(!animation&&view==RUN&&state.run.phase==ACT_FIND)sample_pointer();repaint();}
static void run_screen(void){view=RUN;selected=0;capture_ready=false;refresh();capture_start=(uint32_t)(esp_timer_get_time()/1000);
 if(state.run.phase==ACT_RELAY&&!state.run.banked)action(ACTION_BANK,0);
}
static void launch_battle(void){playing=true;nav_open(PAGE_TRAINER);}
static void tick_frame(lv_timer_t *t){
 (void)t;unsigned old=animation_clock.elapsed_ms;
 unsigned ms=presentation_clock_sample(&animation_clock,(uint32_t)(esp_timer_get_time()/1000),screen_idle_is_off());
 if(screen_idle_is_off())return;
 if(animation){
  bool committed=false;
  if(throw_pending&&ms>=capture_frame_ms(37)){
   /* Show the open ball's intentional still pose before Flash work.  The
    * flight responds immediately; persistence must not skip later movement
    * or expose a successful capture before the transaction has committed. */
   animation_clock.elapsed_ms=capture_frame_ms(37);repaint();screen_flush();
   error=world_activity_action(&request,&result_data);throw_pending=false;
   animation_clock.last_ms=(uint32_t)(esp_timer_get_time()/1000);
   if(error){animation=0;refresh();music_director_play(MUSIC_ROUTE);draw();return;}
   caught=result_data.capture.caught;refresh();
   committed=true;
   ms=animation_clock.elapsed_ms;
  }
  for(unsigned i=0;i<3;i++){unsigned cue=capture_frame_ms(287+i*49);if(old<cue&&ms>=cue)sfx_play(SFX_MENU);}
  if(old<capture_frame_ms(CAPTURE_RESULT_SOURCE_FRAME)&&ms>=capture_frame_ms(CAPTURE_RESULT_SOURCE_FRAME)){
   if(caught)music_director_play(MUSIC_CAUGHT);else{sfx_play(SFX_ESCAPED);music_director_play(MUSIC_ROUTE);}
  }
  if(ms>=capture_frame_ms(CAPTURE_END_SOURCE_FRAME)+ (caught?3200u:1000u)){
   animation=0;refresh();selected=0;capture_start=(uint32_t)(esp_timer_get_time()/1000);
  }
  if(animation){
   // Sprite, ball and result message stay inside these ten bands. The title,
   // cleared action rows and footer are static throughout the performance.
   draw_range(40,240);
   if(committed)draw_range(280,SCREEN_H);
  }else draw();
 }else if(view==RUN&&state.run.phase==ACT_FIND&&capture_ready){sample_pointer();draw_range(CAPTURE_BAR_TOP,CAPTURE_BAR_TOP+32);}
}
static void tick(lv_timer_t *t){screen_cadence_begin(ACTIVITY_TICK_MS);tick_frame(t);screen_cadence_end();}
void play_activity_open_goal(unsigned k){requested_kind=k;guided=true;nav_open(PAGE_ACTIVITY);}
void play_activity_enter(void){refresh();error=0;selected=animation=0;throw_pending=press_handled=capture_ready=false;view=state.run.kind?RUN:MENU;capture_start=(uint32_t)(esp_timer_get_time()/1000);if(view==RUN)run_screen();else if(requested_kind&&activity_open(requested_kind,&chapter)){kind=requested_kind;team_mask=0;hard=0;view=TEAM;}requested_kind=0;screen_set_redraw(repaint);timer=lv_timer_create(tick,ACTIVITY_TICK_MS,NULL);music_director_play(MUSIC_ROUTE);draw();}
void play_activity_exit(void){if(timer){lv_timer_delete(timer);timer=NULL;}}
bool play_activity_screen_busy(void){return animation!=0;}
void play_activity_debug_state(void){
 ESP_LOGI("activity","@@ACTIVITY_VIEW view=%u phase=%u node=%u species=%u attempts=%u ball=%u ready=%u visible_species=%u window=%u..%u pointer=%u elapsed=%lu animation_ms=%lu pending=%u error=%u result_window=%u..%u result_pointer=%u caught=%u",
  view,state.run.phase,state.run.node,state.run.found.species_id,state.run.attempts,ball,capture_ready,capture_target.species_id,
  visible_capture.window_start,visible_capture.window_end,visible_capture.pointer,(unsigned long)visible_capture_elapsed,
  (unsigned long)animation_clock.elapsed_ms,throw_pending,error,result_data.capture.window_start,result_data.capture.window_end,result_data.capture.pointer,caught);
}
void play_activity_key(bsp_btn_t b,bsp_btn_ev_t e){
 /* Only timing-sensitive throws use PRESS; ordinary menus still use CLICK.
  * Consume the rest of the gesture even if saving failed (no silent retry). */
 if(b==BSP_BTN_OK){
  if(e==BSP_BTN_CLICK&&press_handled){press_handled=false;return;}
  if(e==BSP_BTN_PRESS){
   press_handled=animation!=0;
   if(animation)return;
   if(!animation&&view==RUN&&state.run.phase==ACT_FIND&&selected==0){press_handled=true;e=BSP_BTN_CLICK;}
  }
 }
 if(animation)return;
 int direction=nav_direction(b,e);bool confirm=nav_confirm(b,e),back=nav_return(b,e);if(!direction&&!confirm&&!back)return;
 if(back){error=0;selected=0;if(view==MENU){if(guided){guided=false;nav_go(PAGE_NARRATIVE);}else nav_back(PAGE_JOHTO);return;}if(view==KEEP_CONFIRM||view==END_CONFIRM||view==BOX||view==BOX_CONFIRM)run_screen();else view=MENU;draw();return;}
 if(direction){selected=nav_list_selection(b,e,choices(),selected);error=0;draw();return;}if(!confirm)return;error=0;
 if(view==MENU){unsigned ids[7];menu_ids(ids);unsigned id=ids[selected];selected=0;if(id==0)run_screen();else if(id==1||id==2||id==5||id==6){kind=id==5?ACT_ICE:id==6?ACT_TOWER:id;team_mask=0;hard=0;view=TEAM;}else if(id==3)view=REWARDS;else{nav_back(PAGE_JOHTO);return;}}
 else if(view==TEAM){if(selected<party.count){if(kind==ACT_BUG)team_mask=1u<<selected;else team_mask^=1u<<selected;}else{unsigned n=0;for(unsigned i=0;i<party.count;i++)n+=!!(team_mask&(1u<<i));if(n!=(kind==ACT_BUG?1:3))error=ACT_LOCKED;else{view=START;selected=0;}}}
 else if(view==START){if(!selected){view=TEAM;selected=0;}else if(selected==2)hard=!hard;else{memset(&request,0,sizeof(request));request.op=ACTION_BEGIN;request.choice=kind;request.hard=hard;request.seed=(uint32_t)esp_timer_get_time();for(unsigned i=0;i<party.count;i++)if(team_mask&(1u<<i))request.slots[request.count++]=i;error=world_activity_action(&request,&result_data);if(!error){run_screen();if(activity_expedition(kind)){launch_battle();return;}}}}
 else if(view==REWARDS){if(selected==ITEM_COUNT){if(state.bug_choices){view=PRIZE;selected=0;}else error=ACT_LOCKED;}else action(ACTION_CLAIM,0);}
 else if(view==PRIZE){if(action(ACTION_PRIZE,selected)){view=REWARDS;selected=ITEM_COUNT;}}
 else if(view==KEEP_CONFIRM){if(selected&&action(ACTION_KEEP,1))run_screen();else if(!selected)run_screen();}
 else if(view==END_CONFIRM){if(selected&&action(ACTION_END_SURVEY,0))run_screen();else if(!selected)run_screen();}
 else if(view==BOX){if(world_box_at(selected,&replacing)){box_slot=selected;view=BOX_CONFIRM;selected=0;}}
 else if(view==BOX_CONFIRM){if(!selected){view=BOX;selected=box_slot;}else{memset(&request,0,sizeof(request));request.op=ACTION_SETTLE;request.box_slot=box_slot;request.expected=replacing;error=world_activity_action(&request,&result_data);if(!error){final_rewards=result_data.rewards;refresh();view=FINISHED;selected=0;}}}
 else if(view==FINISHED){if(guided){guided=false;nav_go(PAGE_NARRATIVE);return;}view=MENU;selected=0;}
 else if(view==RUN){activity_run_t *r=&state.run;
  if(r->phase==ACT_PATH){if(selected==3){view=END_CONFIRM;selected=0;}else if(action(ACTION_SURVEY,selected))run_screen();}
  else if(r->phase==ACT_FIND){if(selected==0){if(!capture_ready){draw();return;}memset(&request,0,sizeof(request));request.op=ACTION_THROW;request.choice=visible_capture.ball;request.elapsed=visible_capture_elapsed;request.expected=capture_target;
   throw_pending=true;caught=false;animation=1;
   presentation_clock_start(&animation_clock,(uint32_t)(esp_timer_get_time()/1000));
   music_director_play(MUSIC_NONE);sfx_play(SFX_BALL_THROW);
  }else if(selected==1)ball=(ball+1)%CAP_BALL_COUNT;else if(selected==2){if(action(ACTION_SKIP,0))run_screen();}else{view=END_CONFIRM;selected=0;}}
  else if(r->phase==ACT_COMPARE){bool replace=!r->candidate.species_id||selected!=0;if(replace&&(r->candidate.flags&1)){view=KEEP_CONFIRM;selected=0;}else if(action(ACTION_KEEP,replace))run_screen();}
  else if(r->phase==ACT_SETTLE){final_score=activity_score(r);final_nodes=r->node;final_won=r->won;final_kind=r->kind;final_partner=r->candidate.species_id;final_xp=r->banked_xp;if(action(ACTION_SETTLE,0)){final_xp+=result_data.exp_gained;final_rewards=result_data.rewards;view=FINISHED;selected=0;}else if(error==ACT_FULL&&state.run.kind==ACT_BUG&&state.run.candidate.species_id){view=BOX;selected=0;error=0;}}
  else if(r->phase==ACT_BATTLE){launch_battle();return;}
  else{unsigned op=r->phase==ACT_CAMP?ACTION_CAMP:r->phase==ACT_RELAY?ACTION_CONTINUE:ACTION_PATH;if(r->phase==ACT_RELAY&&!r->banked){if(!action(ACTION_BANK,0)){draw();return;}}if(action(op,selected)){run_screen();if(state.run.phase==ACT_BATTLE){launch_battle();return;}}}
 }
 refresh();draw();
}
