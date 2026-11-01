#include "narrative_script.h"
#define E (TRAINER_JOHTO_FIRST+8)
#define B (TRAINER_JOHTO_FIRST+1)
#define M (TRAINER_JOHTO_FIRST+3)
#define J (TRAINER_JOHTO_FIRST+5)
#define P(a,n,x,y,z) {a,0,N_ART_PERSON,n,{x,y,z}}
#define S(k,n,x,y,z) {E,0,k,n,{x,y,z}}
#define MON(id,n,x,y,z) {E,id,N_ART_MON,n,{x,y,z}}
static const narrative_script_t scripts[N_SCENE_COUNT]={
 [N_LETTER]={"A Distant Letter",{
  P(N_ACTOR_OAK,"Prof. Oak","Prof. Elm sent a letter.","Pokemon in Johto are","all migrating one way."),
  P(N_ACTOR_OAK,"Prof. Oak","He wants an expert trainer","to find out why.","Will you go and look?"),
  S(N_ART_JOURNEY,"Off to Johto","Sea wind flips the letter.","A strange cry drifts in.","Elm waits at his lab.")}},
 [N_ARRIVAL]={"Elm's Lab",{
  P(E,"Prof. Elm","You made it at last!","Bugs changed their course.","The ruins make odd sounds."),
  P(E,"Prof. Elm","Don't chase legends yet.","Look at the tracks that","ordinary Pokemon left."),
  P(E,"Prof. Elm","Pick a Johto partner.","First stop: Sprout Fields.","It joins once you confirm.")}},
 [N_WELCOME]={"Our Survey",{
  P(E,"Prof. Elm","Welcome to Johto.","We build on what we found.","A new survey starts here."),
  P(E,"Prof. Elm","Notes log the next stop.","Observe, judge, then act.","Come back any time.")}},
 [N_FIELD]={"Small Traces",{
  S(N_ART_FOREST,"Sprout Fields","Grass leans one way.","This is no chance meeting.","Note the route they took."),
  P(E,"Prof. Elm","Show this to the Leaders.","Falkner preps your trip.","Bugsy knows forest bugs.")}},
 [N_EGG]={"A Moving Puzzle",{
  P(E,"Prof. Elm","I'm keeping a Mystery Egg.","It reacts to travel noise.","Bring it on the survey."),
  P(E,"Prof. Elm","Claim it on the Egg page.","Each valid explore helps.","No need to wait to hatch.")}},
 [N_BUG_INVITE]={"Bugsy's Invitation",{
  P(B,"Bugsy","Bugs now avoid one path.","Join a bug survey?","Let's see where they go."),
  P(B,"Bugsy","Grass, trunks, flowers.","Six sites make a record.","No need to win or catch.")}},
 [N_BUG_REPORT]={"Why the Swarm Detours",{
  P(B,"Bugsy","The record is back.","Bugs ignored the flowers.","They avoid the wall path."),
  S(N_ART_RUINS,"End of the Path","Carvings line the wall.","Bark marks look the same.","They may point one place."),
  P(B,"Bugsy","To learn their origin,","where should we look next?","")},
  {"Sniff the flowers","Compare the ruin walls","Ask shop prices"},"Carvings are clues. Match.",3},
 [N_RUIN_INVITE]={"Echoes in the Wall",{
  S(N_ART_RUINS,"Alph Ruins","The bug tracks now match.","Similar marks on the wall.","Shapes shift with sound."),
  P(M,"Morty","Study the first wall.","Place symbols in order.","Read one set to go on.")}},
 [N_RUIN_REPORT]={"Direction in the Mural",{
  MON(201,"Unknown glyphs","Symbols rise off the wall.","They form three figures.","You read the first record."),
  S(N_ART_TOWER,"Ancient Mural","No leaves or ripples here.","Smoke curls up past posts.","Three figures circle it."),
  P(M,"Morty","Where does this mural","most likely depict?","")},
  {"A dense forest","A calm lake bed","A scorched old tower"},"Read smoke and posts.",3},
 [N_TOWER_INVITE]={"Steps in the Smoke",{
  P(M,"Morty","Your partner is ready.","Ruin notes name the tower.","Stay close. Listen inside."),
  S(N_ART_TOWER,"Burned Tower entrance","A low echo rumbles below.","Old posts bear 3 marks.","Go in and record them.")}},
 [N_TOWER_REPORT]={"Three Different Paths",{
  S(N_ART_TOWER,"Burned Tower","Smoke clears. They leave.","Thunder, scorch, water","each head different ways."),
  P(M,"Morty","Read the clues first.","Then bring a fit partner.","Re-walking finds nothing."),
  P(M,"Morty","Which trail first?","Notes log your choice.","")},
  {"Thunder and Log","Warm scorch marks","Clear stream"},"All three are fine.",3},
 [N_LAKE_INVITE]={"Red Shadow on the Lake",{
  P(E,"Prof. Elm","Someone saw a red shape.","Is the red shape to blame?","Look first. Don't attack."),
  MON(130,"Lake of Rage","Red scales near a plant.","Harsh signals on the lake.","Find the sound's source.")}},
 [N_LAKE_REPORT]={"After the Signal",{
  MON(130,"Prof. Elm","You calmed the Gyarados.","The red one will join you.","A lakeside site caused it."),
  P(J,"Jasmine","Signal off. Tide stays.","Another sound far at sea.","When ready, try Whirl Is.")}},
 [N_SEA_INVITE]={"Other Side of the Wind",{
  P(J,"Jasmine","Wind shifts, tide shifts.","Bring your three picks.","Relay camps: stay or go."),
  S(N_ART_SEA,"Whirl Islands","Shallows give safe hauls.","Wing clues lie deep down.","Check team and stamina.")}},
 [N_SEA_REPORT]={"The Silver Wing Answers",{
  S(N_ART_SEA,"Deep Sea camp","The Silver Wing trembles.","The tide finds a rhythm.","It answers a deeper call."),
  P(J,"Jasmine","It reacts to each partner.","Watch the tide, then ask.","Site hints can be viewed.")}},
 [N_ROOF]={"Rainbow in the Mural",{
  P(M,"Morty","The beast notes fit.","One blank spot is left.","Answer: top of Bell Tower."),
  S(N_ART_TOWER,"Bell Tower","Eight badges catch light.","Follow the feather's hue,","finish the last checks.")}},
 [N_ELM]={"The Last Study",{
  P(E,"Prof. Elm","Bugs, ruins, the deep.","You filled in the records.","One last study remains."),
  P(E,"Prof. Elm","Now it's my team's turn.","Bring your full team.","Show me how you've grown.")}},
 [N_ENDING]={"The Answer Was the Journey",{
  P(E,"Prof. Elm","You did more than chase.","You watched and helped.","That made the study whole."),
  S(N_ART_FOREST,"Journey Review","Forest bugs, ruin echoes,","the Wing, the red one,","all became your story."),
  P(E,"Prof. Elm","Keepsake is in Adv. Notes.","But the woods echo again.","They sound like the past.")}},
 [N_FOREST]={"A Familiar, Strange Forest",{
  P(N_ACTOR_OAK,"Prof. Oak","Kanto woods echo the same.","Re-survey a known route.","Then take it to Ilex Woods"),
  S(N_ART_FOREST,"Deep in the forest","Both forests seem to merge","Follow the sound onward.","Time left its mark too.")}},
 [N_LEGEND_RAI]={"End of the Thunder",{S(N_ART_FOREST,"Survey site","The log is lightning-hit.","Distant steps stop short.","A figure steps out."),MON(243,"Raikou","The thunder answers.","This encounter is logged.","Go now or later. Kept.")}},
 [N_LEGEND_ENTEI]={"End of the Scorch Trail",{S(N_ART_TOWER,"Survey site","Warm tracks stop here.","Dust slides off the steps.","A low roar fills the air."),MON(244,"Entei","Flames light up its shape.","This encounter is logged.","Go now or later. Kept.")}},
 [N_LEGEND_SUICUNE]={"The Clear Water Answers",{S(N_ART_SEA,"Survey site","Branch moved. Water flows.","A shadow rises from below.","It quietly watches you."),MON(245,"Suicune","It accepts your approach.","This encounter is logged.","Go now or later. Kept.")}},
 [N_LEGEND_LUGIA]={"Call of the Deep",{S(N_ART_SEA,"Deep Sea depths","The Wing hums on the wind.","The surging tide parts.","A long cry echoes below."),MON(249,"Lugia","You found who was calling.","This encounter is logged.","Go now or later. Kept.")}},
 [N_LEGEND_HOOH]={"Rainbow Atop the Tower",{S(N_ART_TOWER,"Bell Tower top","The notes match the mural.","The feather glows rainbow.","Huge wings sweep the roof."),MON(250,"Ho-Oh","Old records come to life.","This encounter is logged.","Go now or later. Kept.")}},
 [N_LEGEND_CELEBI]={"Master of the Echo",{S(N_ART_FOREST,"Ilex Woods","Familiar shadows overlap.","A voice from the past.","A small friend appears."),MON(251,"Celebi","It travels through time.","This encounter is logged.","Go now or later. Kept.")}},
};
const narrative_script_t *narrative_script(unsigned s){return s<N_SCENE_COUNT?&scripts[s]:NULL;}
