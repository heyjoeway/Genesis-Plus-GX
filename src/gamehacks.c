#include "shared.h"
#include "backends/sound/sound_base.h"
#include "backends/video/video_base.h"
#include "cpuhook.h"

#define GameModeID_SegaScreen       0x0
#define GameModeID_TitleScreen      0x4
#define GameModeID_Demo     		0x8
#define GameModeID_Level        	0xC
#define GameModeID_SpecialStage     0x10
#define GameModeID_ContinueScreen   0x14
#define GameModeID_2PResults        0x18
#define GameModeID_2PLevelSelect    0x1C
#define GameModeID_EndingSequence   0x20
#define GameModeID_OptionsMenu      0x24
#define GameModeID_LevelSelect      0x28
#define GameModeID_Mask_TitleCard   0b01111111

#define mQueue 0xEEC2
#define mFlags 0xEEB0+1
#define Current_Zone 0xFB90-1
#define Current_Act 0xFB91-1
#define Game_Mode 0xF370+1
#define SSTrack_anim 0xDCAA+1
#define SS_Cur_Speed_Factor 0xDCB8-1
#define Option_Emulator_Scaling 0xFD3F-1
#define Option_Emulator_MirrorMode 0xFD36+1

#define mQueue_0 mQueue // music
#define mQueue_1 mQueue+1 // cmd
#define mQueue_2 mQueue+3  // sfx ??? why +3

#define mFlags_Mask_Panning     0b00000001
#define mFlags_Mask_SpeedShoes  0b00000010
#define mFlags_Mask_Underwater  0b00000100
#define mFlags_Mask_Paused      0b10000000
#define mFlags_Mask_BackedUp    0b00010000

#define id_music_start  0x0B
#define id_music_end    0x2A
#define id_music_life   0x25

#define ID_CMD_NULL     0
#define ID_CMD_STOP     3

int gamehacks_play_sfx(char id) {
    if (id == 0) return 0;

    // This literally should never be an issue unless the path is changed below
    char *path = (char *)malloc((100)*sizeof(char));

    // This is pretty much exclusively used for ring panning
    sprintf(path, "./gamehacks/sfx/%x_%s.wav",
        id,
        work_ram[mFlags] & mFlags_Mask_Panning ? "r" : "l"
    );
    if (Backend_Sound_PlaySFX(path)) return ID_CMD_NULL;
    
    // This is the part for literally every other sound effect
    sprintf(path, "./gamehacks/sfx/%x.wav", id);
    if (Backend_Sound_PlaySFX(path)) return ID_CMD_NULL;
    
    return id;
}

int gamehacks_play_music_path(char *path) {
    Backend_Sound_StopMusic();
    if (!Backend_Sound_PlayMusic(path)) return ID_CMD_NULL;
    return ID_CMD_STOP;
}

char music_id_backup;
char music_id_current;
int music_playing_1up_file;

int gamehacks_play_music(char id) {
    if (id == 0) return ID_CMD_NULL;

    // Gotta love 1-up track edge cases
    if (id == id_music_life) {
        music_id_backup = music_id_current;
        music_playing_1up_file = 1;
    } else if (music_id_current == id_music_life) {
        music_id_backup = id;
        return ID_CMD_NULL;
    }
    music_id_current = id;

    // This literally should never be an issue unless the path is changed below
    char *path = (char *)malloc((80+1)*sizeof(char));

    // Attempt to play per-act track
    sprintf(
        path,
        "./gamehacks/music/%x_%i.ogg",
        id,
        work_ram[Current_Act] + 1
    );
    // Return music stop command
    if (Backend_Sound_PlayMusic(path)) return ID_CMD_STOP;
    
    // If it wasn't found, play the normal track
    sprintf(
        path,
        "./gamehacks/music/%x.ogg",
        id
    );
    // Return music stop command
    if (Backend_Sound_PlayMusic(path)) return ID_CMD_STOP;
    
    // Welp, couldn't find anything.
    // Return the ID as it'll be passed back through to the game
    music_playing_1up_file = 0;
    return id;
}

int gamehacks_play_command(char id) {
    // I think I need to finish implementing commands
    switch (id) {
        case 0:
            return ID_CMD_NULL;
        case 2:
            Backend_Sound_FadeOutMusic(750);
        default:        
            return id;
    }
}

int gamehacks_play_sound(char id) {
    if (id < id_music_start) return gamehacks_play_command(id);
    else if (id <= id_music_end) return gamehacks_play_music(id);
    else return gamehacks_play_sfx(id);
}

void gamehacks_update_sound() {
    if (music_playing_1up_file) {
        if (!Backend_Sound_IsPlayingMusic()) {
            music_playing_1up_file = 0;
            music_id_current = 0;
            char music_id_tmp = music_id_backup;
            music_id_backup = 0;
            gamehacks_play_music(music_id_tmp);
        }
    } else if (!(work_ram[mFlags] & mFlags_Mask_BackedUp) && music_id_backup) {
        music_id_current = 0;
        int music_id_tmp = music_id_backup;
        music_id_backup = 0;
        gamehacks_play_music(music_id_tmp);
    }

    work_ram[mQueue_0] = gamehacks_play_sound(work_ram[mQueue_0]);
    work_ram[mQueue_1] = gamehacks_play_sound(work_ram[mQueue_1]);
    work_ram[mQueue_2] = gamehacks_play_sound(work_ram[mQueue_2]);

    Backend_Sound_MusicSpeed(
        (work_ram[mFlags] & mFlags_Mask_SpeedShoes) ?
        1.25 : 1
    );
    Backend_Sound_SetPause(work_ram[mFlags] & mFlags_Mask_Paused);
    Backend_Sound_MusicSetUnderwater(work_ram[mFlags] & mFlags_Mask_Underwater);
}

int option_scaling_prev = 0;
void gamehacks_update_options() {
    option_scaling = work_ram[Option_Emulator_Scaling];
    if (option_scaling != option_scaling_prev) {
        bitmap.viewport.changed |= 1;
        option_scaling_prev = option_scaling;
    }
    uint8 gamemode = work_ram[Game_Mode];
    gamemode &= GameModeID_Mask_TitleCard;
    option_mirrormode = work_ram[Option_Emulator_MirrorMode] && (
        (gamemode == GameModeID_Demo) |
        (gamemode == GameModeID_Level) |
        (gamemode == GameModeID_SpecialStage)
    );
}

void gamehacks_update() {
    gamehacks_update_sound();
    gamehacks_update_options();
}

void gamehacks_deinit() {
    // This might end up getting used for the special stage stuff
}

// SDL_Texture *ss_track;
// SDL_Texture *ss_bg;
// SDL_Rect ss_bg_src;
int ss_track_frame = 0;
char *ss_track_path;

void gamehacks_init() {
    // set_cpu_hook(&gamehacks_cpuhook);
    ss_track_path = (char *)malloc(80*sizeof(char));
    // ss_bg = (SDL_Texture *)Backend_Video_LoadImage(
    //     "./gamehacks/specialstage/bg.png"
    // );
    // ss_bg_src.w = 398;
    // ss_bg_src.h = 224;
}

void gamehacks_render_ss_track() {
    sprintf(
        ss_track_path,
        "./gamehacks/specialstage/track/%1i/%04i.png",
        work_ram[SSTrack_anim],
        ss_track_frame + 1
    );
    // ss_track = (SDL_Texture *)Backend_Video_LoadImage(ss_track_path);
    // if (ss_track == NULL) {
    //     if (ss_track_frame != 0) {
    //         ss_track_frame = 0;
    //         gamehacks_render_ss_track();
    //     }
        
    //     return;
    // }
    ss_track_frame++;

    // SDL_RenderCopy(
    //     Backend_Video_GetRenderer(),
    //     ss_track,
    //     NULL,
    //     NULL
    // );
}

void gamehacks_render_ss_bg() {
    uint16 bg_xscroll = (*(uint32 *)&vram[hscb]) >> 16;
    bg_xscroll %= 256;
    bg_xscroll = 256 - bg_xscroll;
    bg_xscroll += 184;
    // ss_bg_src.x = bg_xscroll;
    uint16 bg_yscroll = (*(uint32 *)&vsram[0]) >> 16;
    bg_yscroll  &= 0x0FF;
    bg_yscroll %= 256;
    // ss_bg_src.y = bg_yscroll;

    // SDL_RenderCopy(
    //     Backend_Video_GetRenderer(),
    //     ss_bg,
    //     &ss_bg_src,
    //     NULL
    // );
}

void gamehacks_render() {
    // int in_special_stage = work_ram[Game_Mode] == GameModeID_SpecialStage;
    // printf("%x\n", *((long *)(work_ram + SS_Cur_Speed_Factor)));
    // render_bg_disable = in_special_stage;
    // if (in_special_stage) {
    //     gamehacks_render_ss_bg();
    //     gamehacks_render_ss_track();
    // }
}