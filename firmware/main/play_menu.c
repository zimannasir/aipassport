#include "display_settings.h"
#include "battery_ui.h"
// P11: GSC's right-side Start menu, adapted to the 240x320 display.
// Frame/cursor are the original shared tiles.
#include <stdio.h>
#include <string.h>

#include "assets.h"
#include "game_ui.h"
#include "nav.h"
#include "play.h"
#include "pokemon_names.h"
#include "render.h"
#include "screen.h"
#include "screen_idle.h"
#include "world.h"
#include "audio_settings.h"
#include "usb_backup.h"
#include "wifi_time.h"
#include "lvgl.h"

#define MENU_COUNT 13
#define MENU_VISIBLE 10
#define MENU_X 120
#define MENU_Y 64
#define MENU_W 112
#define MENU_H 184
#define ROW_Y 72
#define ROW_STEP 16
#define OPTION_Y 48
#define OPTION_STEP 18
enum { OPTION_NAMES, OPTION_SCREEN_OFF, OPTION_MUTE, OPTION_VOLUME, OPTION_BACKUP, OPTION_IMPORT, OPTION_WIFI, OPTION_BRIGHTNESS, OPTION_BATTERY, OPTION_RETURN, OPTION_COUNT };

SCREEN_ASSERT_ALLOW_CROSS_BAND(menu_title, 8, 16);
SCREEN_ASSERT_ALLOW_CROSS_BAND(menu_portrait, 88, 96);
SCREEN_ASSERT_ALLOW_CROSS_BAND(menu_description, 252, 16);
SCREEN_ASSERT_ALLOW_CROSS_BAND(menu_option_names, OPTION_Y, 16);
SCREEN_ASSERT_ALLOW_CROSS_BAND(menu_option_screen, OPTION_Y + OPTION_STEP, 16);
SCREEN_ASSERT_ALLOW_CROSS_BAND(menu_option_return, OPTION_Y + OPTION_STEP * 3, 16);

static const char *LABELS[MENU_COUNT] = {"Pokedex", "Party", "Items", "Care", "Battle", "Options", "Awards", "Explore", "Dungeon", "Johto", "Radar", "Roam", "Close"};
static const char *DESCRIPTIONS[MENU_COUNT] = {
    "Seen and caught Pokemon", "View Pokemon, pick leader", "View and use items",
    "Feed, play, rest", "Gyms, badges, League, Red", "Names, sound, screen", "Progress and rewards", "Pick a route, track Pokemon", "Take Pokemon in for rewards", "New Pokemon, Johto surveys", "Nearby signals, shinies", "Walk the map, find traces", "Back to the adventure",
};
static uint8_t s_selected;
static bool s_johto_visible;
static unsigned menu_count(void){return s_johto_visible?13:12;}
static unsigned menu_id(unsigned row){return !s_johto_visible&&row>=9?row+1:row;}
static bool s_options;
static bool s_backup_view;
static bool s_wifi_view;
static wifi_time_view_t s_wifi_shown;
static bool s_import_accept;
static lv_timer_t *s_backup_timer;
static usb_backup_state_t s_backup_shown;
static bool s_volume_edit;
static bool s_brightness_edit;
static bool s_display_save_failed;
static bool s_audio_save_failed;
static uint8_t s_option_selected;
static world_t s_world;
static world_party_t s_party;
static unsigned s_claimable;

static void draw_options(int band_y)
{
    char value[64];
    game_ui_title(band_y, "Options", "");
    game_ui_box(band_y, 8, 40, 224, 200);
    render_text(40, OPTION_Y - band_y, "Names", GAME_UI_INK);
    snprintf(value, sizeof(value), "English");
    render_text(212 - render_text_width(value), OPTION_Y - band_y, value, GAME_UI_ACCENT);
    render_text(40, OPTION_Y + OPTION_STEP - band_y, "Screen off now", GAME_UI_INK);
    render_text(40, OPTION_Y + OPTION_STEP * 2 - band_y, "Mute", GAME_UI_INK);
    render_text(196, OPTION_Y + OPTION_STEP * 2 - band_y,
                audio_settings_muted() ? "On" : "Off", GAME_UI_ACCENT);
    render_text(40, OPTION_Y + OPTION_STEP * 3 - band_y, "Volume", GAME_UI_INK);
    snprintf(value, sizeof(value), "%u%%", audio_settings_volume());
    render_text(212-render_text_width(value), OPTION_Y + OPTION_STEP * 3-band_y, value, GAME_UI_ACCENT);
    render_text(40, OPTION_Y + OPTION_STEP * 4 - band_y, "Back up save", GAME_UI_INK);
    render_text(40, OPTION_Y + OPTION_STEP * 5 - band_y, "Import save", GAME_UI_INK);
    render_text(40, OPTION_Y + OPTION_STEP * 6 - band_y, "Wi-Fi sync", GAME_UI_INK);
    render_text(40, OPTION_Y + OPTION_STEP * OPTION_BRIGHTNESS - band_y, "Brightness", GAME_UI_INK);
    snprintf(value, sizeof(value), "%u%%", display_settings_brightness());
    render_text(212-render_text_width(value), OPTION_Y + OPTION_STEP * OPTION_BRIGHTNESS-band_y, value, GAME_UI_ACCENT);
    render_text(40, OPTION_Y + OPTION_STEP * OPTION_BATTERY-band_y, "Battery icon", GAME_UI_INK);
    render_text(196, OPTION_Y + OPTION_STEP * OPTION_BATTERY-band_y,
                display_settings_battery_visible() ? "On" : "Off", GAME_UI_ACCENT);
    render_text(40, OPTION_Y + OPTION_STEP * OPTION_RETURN - band_y, "Back to menu", GAME_UI_INK);
    game_ui_cursor(band_y, 20, OPTION_Y + s_option_selected * OPTION_STEP + 4);
    const char *hint;
    if (s_option_selected == OPTION_MUTE) {
        snprintf(value, sizeof(value), "%s", audio_settings_muted() ? "Music and SFX are off" : "Music and SFX are on");
        hint = s_audio_save_failed ? "Save failed. C to retry" : "C to toggle. Kept on reboot";
    } else if (s_option_selected == OPTION_VOLUME) {
        snprintf(value, sizeof(value), "%s", audio_settings_muted() ? "Muted. Stays muted" : "Music, SFX, alerts");
        hint = s_audio_save_failed ? "Save failed. Retry" : "Volume kept on reboot";
    } else if (s_option_selected == OPTION_BRIGHTNESS) {
        snprintf(value, sizeof(value), "C to adjust brightness");
        hint = "10%-100%. Saved on change";
    } else if (s_option_selected == OPTION_BATTERY) {
        snprintf(value, sizeof(value), "Show battery on menu/care");
        hint = s_display_save_failed ? "Save failed. C to retry" : "C to toggle. Kept on reboot";
    } else if (s_option_selected == OPTION_BACKUP || s_option_selected == OPTION_IMPORT) {
        snprintf(value,sizeof(value),"Manage saves over USB");
        hint=s_option_selected==OPTION_IMPORT?"Pick file on PC, confirm here":"Open PC save page first";
    } else if (s_option_selected == OPTION_WIFI) {
        snprintf(value,sizeof(value),"Recover stamina while off");
        hint="Set up home Wi-Fi first";
    } else if (s_option_selected == OPTION_NAMES) {
        snprintf(value, sizeof(value), "Names are shown in English");
        hint = "Fixed in this version";
    } else {
        snprintf(value, sizeof(value), "Screen off after %lus idle", (unsigned long)(screen_idle_timeout_ms() / 1000));
        hint = s_option_selected == OPTION_SCREEN_OFF ? "C: screen off. Any key wakes" : "Back to the previous menu";
    }
    game_ui_text_centered(band_y, 12, 240, 216, 16, value, GAME_UI_MUTED);
    game_ui_text_centered(band_y, 12, 260, 216, 16, hint, GAME_UI_INK);
    game_ui_footer(band_y, s_volume_edit ? "[A]+ [B]- [C]Done" : GAME_UI_NAV_HINT);
}

static void draw_brightness(int band_y)
{
    char value[8];
    uint8_t brightness = display_settings_brightness();
    game_ui_title(band_y, "Brightness", "");
    game_ui_box(band_y, 8, 56, 224, 160);
    game_ui_text_centered(band_y, 24, 76, 192, 16, "Brightness", GAME_UI_MUTED);
    snprintf(value, sizeof(value), "%u%%", brightness);
    game_ui_text_centered(band_y, 24, 112, 192, 16, value, GAME_UI_ACCENT);
    game_ui_meter(band_y, 28, 148, 184, brightness);
    render_text(28, 184 - band_y, "10%", GAME_UI_MUTED);
    render_text(212 - render_text_width("100%"), 184 - band_y, "100%", GAME_UI_MUTED);
    game_ui_text_centered(band_y, 12, 236, 216, 16,
                          s_display_save_failed ? "Save failed. Retry" : "Applies and saves at once",
                          GAME_UI_INK);
    game_ui_text_centered(band_y, 12, 260, 216, 16, "Hold B: back", GAME_UI_MUTED);
    game_ui_footer(band_y, "[A]+ [B]- [C]Done");
}

static void draw_backup(int band_y)
{
    usb_backup_state_t st=usb_backup_state();
    bool importing=usb_backup_import_mode_active();
    game_ui_title(band_y,importing?"Import save":"Back up save", "USB");
    if(st==USB_RESTORE_OFFER) {
        game_ui_text_centered(band_y,8,64,224,16,"Overwrite current save?",GAME_UI_INK);
        game_ui_text_centered(band_y,8,96,224,16,"Progress is backed up first",GAME_UI_MUTED);
        game_ui_text_centered(band_y,8,120,224,16,"Device restarts when done",GAME_UI_MUTED);
        game_ui_box(band_y,16,164,208,80);
        render_text(56,180-band_y,"Cancel",GAME_UI_INK);
        render_text(56,214-band_y,"Overwrite",GAME_UI_INK);
        game_ui_cursor(band_y,32,(s_import_accept?214:180)+4);
        game_ui_footer(band_y,GAME_UI_NAV_HINT);return;
    }
    game_ui_box(band_y,8,64,224,160);
    const char *a="Open PC save page first", *b="Connect and pick a folder", *c="C to retry";
    if(importing){a="Pick a backup file on PC";b="Then confirm here";c="Hold B: cancel and back";}
    if(st==USB_BACKUP_SENDING) {a="Sending save";b="Keep USB connected";c="Wait for transfer";}
    else if(st==USB_BACKUP_WAIT_ACK) {a="PC is saving";b="Wait for file write";c="Keep the page open";}
    else if(st==USB_RESTORE_RECEIVING) {a="Receiving import";b="Progress backed up to PC";c="Keep USB connected";}
    else if(st==USB_RESTORE_STAGED) {a="Save verified";b="Restarting to finish import";c="Keep device powered";}
    else if(st==USB_BACKUP_DONE) {a="Backup done";b="File saved to PC";c="Original save unchanged";}
    else if(st==USB_BACKUP_FAILED) {a=importing?"Import not done":"Backup not done";b="Check cable and folder";c=importing?"Save kept. Reconnect":"C to retry. Save kept";}
    game_ui_text_centered(band_y,12,88,216,16,a,GAME_UI_INK);
    game_ui_text_centered(band_y,12,128,216,16,b,GAME_UI_MUTED);
    game_ui_text_centered(band_y,12,184,216,16,c,GAME_UI_INK);
    bool busy=st==USB_BACKUP_SENDING||st==USB_BACKUP_WAIT_ACK||st==USB_RESTORE_RECEIVING||st==USB_RESTORE_STAGED;
    game_ui_footer(band_y,busy?"Wait until finished":importing?"Hold B: back":"[C]Backup  Hold B: back");
}

static void draw_wifi(int band_y)
{
    wifi_time_view_t v;wifi_time_view(&v);char text[64];
    game_ui_title(band_y,"Wi-Fi sync","Hold B: back");
    if(v.setup){
        game_ui_text_centered(band_y,8,48,224,16,"Phone: join this hotspot",GAME_UI_INK);
        game_ui_box(band_y,8,76,224,112);
        game_ui_text_centered(band_y,12,92,216,16,v.ssid,GAME_UI_INK);
        snprintf(text,sizeof(text),"Password %s",v.password);
        game_ui_text_centered(band_y,12,120,216,16,text,GAME_UI_INK);
        game_ui_text_centered(band_y,12,156,216,16,"192.168.4.1",GAME_UI_ACCENT);
        game_ui_text_centered(band_y,8,204,224,16,"Open the address above",GAME_UI_INK);
        game_ui_text_centered(band_y,8,232,224,16,"Enter home Wi-Fi and password",GAME_UI_MUTED);
        game_ui_footer(band_y,"[C]Close  Hold B: back");return;
    }
    const char *status="Network not set up";
    if(v.state==WIFI_TIME_CONNECTING)status="Connecting to Wi-Fi";
    else if(v.state==WIFI_TIME_WAITING)status="Online. Waiting for sync";
    else if(v.state==WIFI_TIME_READY)status="Time synced";
    else if(v.state==WIFI_TIME_OFFLINE)status="Offline. Will retry";
    else if(v.state==WIFI_TIME_ERROR)status="Sync or save failed. Retry";
    game_ui_box(band_y,8,56,224,88);
    game_ui_text_centered(band_y,12,72,216,16,status,GAME_UI_INK);
    snprintf(text,sizeof(text),"Stamina recovered: %u",v.recovered);
    game_ui_text_centered(band_y,12,108,216,16,text,GAME_UI_ACCENT);
    game_ui_text_centered(band_y,8,168,224,16,"Recovers stamina when online",GAME_UI_INK);
    game_ui_text_centered(band_y,8,196,224,16,"Full in about 1 hour",GAME_UI_MUTED);
    game_ui_text_centered(band_y,8,224,224,16,"Counts after first sync",GAME_UI_MUTED);
    game_ui_footer(band_y,"[A]Retry [C]Set up");
}

static void draw_main(int band_y)
{
    char name[48], text[48];
    battery_ui_playtime_text(text, sizeof(text));
    game_ui_title(band_y, "Menu", text);
    species_t species;
    bool found = assets_species(s_world.species, &species);
    if (found) snprintf(name, sizeof(name), "%.*s", species.name_zh_len, species.name_zh);
    else snprintf(name, sizeof(name), "#%03u", s_world.species);
    game_ui_text_fitted(band_y, 12, 40, 144, name, GAME_UI_INK);
    battery_ui_draw(band_y, 228, 40);
    snprintf(text, sizeof(text), "Lv%u", s_world.level);
    game_ui_text_centered(band_y, 8, 72, 104, 16, text, GAME_UI_MUTED);
    uint8_t size;
    const uint8_t *front = assets_front_sprite(s_world.species, &size);
    if (found && front) {
        uint16_t palette[4];
        bool shiny = s_party.count && (s_party.members[0].flags & 1);
        assets_palette_variant(species.palette, shiny, palette);
        game_ui_sprite_centered(band_y, 8, 88, 104, 96, front, size, size, 1, palette);
    }
    snprintf(text, sizeof(text), "Party %u/%u", s_party.count, PARTY_MAX);
    game_ui_text_centered(band_y, 8, 196, 104, 16, text, GAME_UI_INK);
    snprintf(text, sizeof(text), "Caught %u", dex_count_caught(world_dex()));
    game_ui_text_centered(band_y, 8, 220, 104, 16, text, GAME_UI_MUTED);

    game_ui_box(band_y, MENU_X, MENU_Y, MENU_W, MENU_H);
    unsigned first=s_selected>=MENU_VISIBLE?s_selected-MENU_VISIBLE+1:0;
    for (unsigned row = first; row < menu_count() && row < first+MENU_VISIBLE; row++) {
        int y = ROW_Y + (row-first) * ROW_STEP;
        render_text(156, y - band_y, LABELS[menu_id(row)], GAME_UI_INK);
        if(row==6&&s_claimable)render_text(212,y-band_y,"!",GAME_UI_ACCENT);
        if (row == s_selected) game_ui_cursor(band_y, 136, y + 4);
    }
    if(first)render_text(216,MENU_Y+2-band_y,"^",GAME_UI_MUTED);
    if(first+MENU_VISIBLE<menu_count())render_text(216,MENU_Y+MENU_H-16-band_y,"v",GAME_UI_MUTED);
    if(s_selected==6&&s_claimable){snprintf(text,sizeof(text),"%u rewards to claim",s_claimable);game_ui_text_centered(band_y,8,252,224,16,text,GAME_UI_INK);}
    else game_ui_text_centered(band_y, 8, 252, 224, 16, DESCRIPTIONS[menu_id(s_selected)], GAME_UI_MUTED);
    game_ui_footer(band_y, GAME_UI_NAV_HINT);
}

static void redraw_status(void)
{
    if (s_options || s_backup_view || s_wifi_view) return;
    for (int y = 0; y < 60; y += SCREEN_BAND_H) {
        screen_band_clear(GAME_UI_BG);
        draw_main(y);
        screen_push_band(y);
    }
}

static void draw_all(void)
{
    for (int band_y = 0; band_y < SCREEN_H; band_y += SCREEN_BAND_H) {
        screen_band_clear(GAME_UI_BG);
        if (s_wifi_view) draw_wifi(band_y);
        else if (s_backup_view) draw_backup(band_y);
        else if (s_brightness_edit) draw_brightness(band_y);
        else if (s_options) draw_options(band_y);
        else draw_main(band_y);
        screen_push_band(band_y);
    }
}

static void backup_refresh(lv_timer_t *timer)
{
    (void)timer;
    if(s_wifi_view){wifi_time_view_t v;wifi_time_view(&v);if(memcmp(&v,&s_wifi_shown,sizeof(v))){s_wifi_shown=v;draw_all();}}
    if(s_backup_view&&s_backup_shown!=usb_backup_state()) {
        s_backup_shown=usb_backup_state();
        if(s_backup_shown==USB_RESTORE_OFFER)s_import_accept=false;
        draw_all();
    }
}

void play_menu_enter(void)
{
    if (!nav_is_returning()) {
        s_selected = 0;
        s_options = false;
        s_backup_view = false;
        s_wifi_view = false;
        s_volume_edit = false;
        s_brightness_edit = false;
        s_display_save_failed = false;
        s_option_selected = 0;
        s_audio_save_failed = false;
    }
    s_johto_visible=world_johto_available();if(s_selected>=menu_count())s_selected=0;
    world_snapshot(&s_world);
    world_party_snapshot(&s_party);
    achievement_view_t achievements;world_achievements_snapshot(&achievements);s_claimable=0;
    for(unsigned i=0;i<ACHIEVEMENT_COUNT;i++)
        if(!achievement_claimed(&achievements,i)&&achievement_progress(&achievements,i)==achievement_info(i)->target)s_claimable++;
    s_backup_timer=lv_timer_create(backup_refresh,250,NULL);
    screen_set_redraw(draw_all);
    battery_ui_set_redraw(redraw_status);
    draw_all();
}

void play_menu_exit(void) { if(s_wifi_view){wifi_time_setup_request(false);s_wifi_view=false;} if(s_backup_timer) {lv_timer_delete(s_backup_timer);s_backup_timer=NULL;} }

void play_menu_presentation_snapshot(play_menu_view_t *out)
{
    if (out) *out = (play_menu_view_t){.selected = s_selected, .options = s_options,
                                     .option_selected = s_option_selected};
}

void play_menu_key(bsp_btn_t btn, bsp_btn_ev_t ev)
{
    if(s_wifi_view){
        wifi_time_view_t v;wifi_time_view(&v);
        if(nav_return(btn,ev)){wifi_time_setup_request(false);s_wifi_view=false;}
        else if(nav_confirm(btn,ev))wifi_time_setup_request(!v.setup);
        else if(btn==BSP_BTN_UP&&ev==BSP_BTN_CLICK&&!v.setup)wifi_time_retry();
        draw_all();return;
    }
    if(s_backup_view) {
        usb_backup_state_t st=usb_backup_state();
        if(st==USB_BACKUP_SENDING||st==USB_BACKUP_WAIT_ACK||st==USB_RESTORE_RECEIVING||st==USB_RESTORE_STAGED)return;
        if(st==USB_RESTORE_OFFER) {
            // Reset selection even if a key arrives before the redraw timer.
            if(s_backup_shown!=st){s_import_accept=false;s_backup_shown=st;}
            if(nav_return(btn,ev))usb_backup_restore_confirm(false);
            else if(nav_direction(btn,ev))s_import_accept=!s_import_accept;
            else if(nav_confirm(btn,ev))usb_backup_restore_confirm(s_import_accept);
        } else if(nav_return(btn,ev)){s_backup_view=false;usb_backup_import_mode(false);}
        else if(nav_confirm(btn,ev)&&!usb_backup_import_mode_active())usb_backup_request();
        draw_all();return;
    }
    if (s_brightness_edit) {
        if (nav_return(btn, ev)) { s_brightness_edit = false; draw_all(); return; }
        if (ev != BSP_BTN_CLICK) return;
        if (btn == BSP_BTN_OK) s_brightness_edit = false;
        else if (btn == BSP_BTN_UP || btn == BSP_BTN_DOWN) {
            int next = display_settings_brightness() + (btn == BSP_BTN_UP ? 10 : -10);
            if (next < DISPLAY_BRIGHTNESS_MIN) next = DISPLAY_BRIGHTNESS_MIN;
            if (next > 100) next = 100;
            s_display_save_failed = !display_settings_set_brightness(next);
        }
        draw_all(); return;
    }
    if (s_volume_edit) {
        if (nav_return(btn, ev)) { s_volume_edit = false; draw_all(); return; }
        if (ev != BSP_BTN_CLICK) return;
        if (btn == BSP_BTN_OK) s_volume_edit = false;
        else {
            int next = audio_settings_volume() + (btn == BSP_BTN_UP ? 5 : -5);
            if (next < 0) next = 0;
            if (next > 100) next = 100;
            s_audio_save_failed = !audio_settings_set_volume(next);
        }
        draw_all(); return;
    }
    if (nav_direction(btn, ev) != 0) {
        if (s_options)
            s_option_selected = (s_option_selected + OPTION_COUNT + nav_direction(btn, ev)) % OPTION_COUNT;
        else s_selected = (s_selected + menu_count() + nav_direction(btn, ev)) % menu_count();
        draw_all();
        return;
    }
    if (ev != BSP_BTN_CLICK && !nav_return(btn, ev)) return;
    if (nav_return(btn, ev)) {
        if (s_options) { s_options = false; draw_all(); }
        else nav_back(PAGE_IDLE);
    } else if (nav_confirm(btn, ev)) {
        if (s_options) {
            if (s_option_selected == OPTION_SCREEN_OFF) {
                screen_idle_request_off();
                return;
            }
            if (s_option_selected == OPTION_MUTE) {
                s_audio_save_failed = !audio_settings_set_muted(!audio_settings_muted());
            } else if (s_option_selected == OPTION_VOLUME) s_volume_edit = true;
            else if (s_option_selected == OPTION_BRIGHTNESS) {
                s_display_save_failed = false;
                s_brightness_edit = true;
            }
            else if (s_option_selected == OPTION_BATTERY) {
                s_display_save_failed = !display_settings_set_battery_visible(!display_settings_battery_visible());
                if (!s_display_save_failed) battery_ui_start();
            }
            else if (s_option_selected == OPTION_BACKUP) {usb_backup_import_mode(false);s_backup_view=true;usb_backup_request();}
            else if (s_option_selected == OPTION_IMPORT) {usb_backup_import_mode(true);s_import_accept=false;s_backup_view=true;}
            else if (s_option_selected == OPTION_WIFI) {s_wifi_view=true;wifi_time_view(&s_wifi_shown);}
            else if (s_option_selected == OPTION_RETURN) s_options = false;
            else pokemon_names_set_style(pokemon_names_get_style() == POKEMON_NAMES_OFFICIAL
                                             ? POKEMON_NAMES_GS_LEGACY : POKEMON_NAMES_OFFICIAL);
            draw_all();
        } else {
            switch (menu_id(s_selected)) {
            case 0: nav_open(PAGE_DEX); break;
            case 1: nav_open(PAGE_PARTY); break;
            case 2: nav_open(PAGE_BAG); break;
            case 3: nav_open(PAGE_CARE); break;
            case 4: nav_open(PAGE_TRAINER); break;
            case 5: s_options = true; s_option_selected = 0; draw_all(); break;
            case 6: nav_open(PAGE_ACHIEVEMENTS); break;
            case 7: nav_open(PAGE_EXPLORATION); break;
            case 8: nav_open(PAGE_DUNGEON); break;
            case 9: nav_open(PAGE_JOHTO); break;
            case 10: nav_open(PAGE_RADAR); break;
            case 11: nav_open(PAGE_FIELDMAP); break;
            case 12: nav_back(PAGE_IDLE); break;
            default: break;
            }
        }
    }
}
