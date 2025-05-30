#include "font/font.h"
#include "functions.h"

#include <lunarengine.h>
#include <math.h>

#include "game/overlay/transition.h"
#include "game/savefile.h"
#include "game/data.h"
#include "game/input.h"
#include "game/camera.h"
#include "game/level.h"
#include "game/overlay/hud.h"
#include "io/io.h"
#include "main.h"
#include "rng.h"
#include "context.h"

#define arrsize(x) (sizeof(x) / sizeof(*(x)))

static void draw_dead_player(void* context, float dstx, float dsty, float dstw, float dsth, int srcx, int srcy, int srcw, int srch, unsigned int color) {
    float timer = context_get_float(context, "timer");
    float shake_intensity = max(0, (30 - timer) / 30) * 8;
    float x = random_range(-shake_intensity, shake_intensity);
    float y = random_range(-shake_intensity, shake_intensity);
    graphics_flush(false);
    graphics_push_shader("iris");
    graphics_shader_set_float("u_iris_posx", 0);
    graphics_shader_set_float("u_iris_posy", 0);
    graphics_shader_set_float("u_iris_radius", 0);
    gfxcmd_process(gfxcmd_important(gfxcmd_texture("images/entities/player.png")), dstx + x, dsty + y, dstw, dsth, srcx, srcy, srcw, srch, color);
    graphics_flush(false);
    graphics_pop_shader();
    if (timer >= 60) {
        shake_intensity = max(0, (30 - (timer - 60)) / 30) * 8;
        x = random_range(-shake_intensity, shake_intensity);
        y = random_range(-shake_intensity, shake_intensity);
        float width = 0, height = 0;
        const char* msg = "${^200}DEFEAT";
        LE_DrawList* dl = LE_CreateDrawList();
        text_size(&width, &height, msg);
        render_text(dl, (WIDTH - width) / 2 + x, 64 + y, msg);
        LE_Render(dl, gfxcmd_process);
        LE_DestroyDrawList(dl);
    }
    context_destroy(context);
}

static void draw_player(void* context, float dstx, float dsty, float dstw, float dsth, int srcx, int srcy, int srcw, int srch, unsigned int color) {
    bool is_hidden = context_get_int(context, "hidden");
    float iframes = context_get_float(context, "iframes");
    int powerup_id = context_get_int(context, "powerup_state");
    int powerup_color = powerup_id != -1 ? get_powerup(powerup_id)->color : -1;

    graphics_flush(false);
    if (powerup_color != -1) {
        int alpha = sin_range(global_timer / 300.f, 0.8, 1.0) * 255;
        graphics_push_shader("rotation");
        graphics_shader_set_float("u_rot_posx", dstx + fabsf(dstw) / 2);
        graphics_shader_set_float("u_rot_posy", dsty + fabsf(dsth) / 2);
        graphics_shader_set_float("u_rot_angle", global_timer / 180.f * 2 * M_PI);
        gfxcmd_process(gfxcmd_texture("images/entities/player_glow.png"), dstx, dsty, dstw, dsth, 0, 0, 16, 16, ((powerup_color & 0xFFFFFF) << 8) | alpha);
        graphics_flush(false);
        graphics_pop_shader();
    }
    if (is_hidden) graphics_push_shader("noise");
    else graphics_push_shader(NULL);
    graphics_push_shader("dither");
    graphics_shader_set_int("u_dither_amount", min(8, iframes / 180 * 8));
    graphics_shader_set_float("u_dither_offx", remainder(-dstx, 1));
    graphics_shader_set_float("u_dither_offy", remainder(-dsty, 1));
    gfxcmd_process(gfxcmd_texture("images/entities/player.png"), dstx, dsty, dstw, dsth, srcx, srcy, srcw, srch, color);
    if (powerup_color != -1) {
        gfxcmd_process(gfxcmd_texture("images/entities/player.png"), dstx, dsty, dstw, dsth, srcx, srcy + 16*1, srcw, srch, 0xFFFFFFFF);
        gfxcmd_process(gfxcmd_texture("images/entities/player.png"), dstx, dsty, dstw, dsth, srcx, srcy + 16*2, srcw, srch, ((powerup_color & 0xFFFFFF) << 8) | 0xFF);
    }
    graphics_flush(false);
    graphics_pop_shader();
    graphics_pop_shader();
    context_destroy(context);
}

static int idle_anim_table[] = { 0, 1, 2, 3, 2, 1 };

entity_texture(player) {
    if (get(entity, "sleeping", Bool, false)) {
        *srcX = 16 * ((global_timer / 30) % 2 + 11);
        *srcY = 0;
        *srcW = 16;
        *srcH = 16;
        *w = 16;
        *h = 16;
        return gfxcmd_texture("images/entities/player.png");
    }
    if (get(entity, "powerup_state", Int, POWERUP_base) == POWERUP_death) {
        *srcX = 9 * 16;
        *srcY = 0;
        *srcW = 16;
        *srcH = 16;
        *w = 16 * (get(entity, "death_left", Bool, false) ? 1 : -1);
        *h = 16;
        return gfxcmd_important(gfxcmd_custom(draw_dead_player, context_create(
            context_float("timer", get(entity, "dead_timer", Float, 0))
        )));
    }
    int sprite = idle_anim_table[(global_timer / 10) % 6];
    bool pouncing    = get(entity, "pouncing",    Bool, false);
    bool facing_left = get(entity, "facing_left", Bool, false);
    if (is_button_pressed(BUTTON_MOVE_LEFT))  facing_left = true;
    if (is_button_pressed(BUTTON_MOVE_RIGHT)) facing_left = false;
    if (is_button_released(BUTTON_MOVE_RIGHT) && is_button_down(BUTTON_MOVE_LEFT)) facing_left = true;
    if (is_button_released(BUTTON_MOVE_LEFT) && is_button_down(BUTTON_MOVE_RIGHT)) facing_left = false;
    if (fabs(entity->velX) > 0) sprite = (int)(entity->posX) % 2 + 4;
    if (
        (entity->velX < 0 && is_button_down(BUTTON_MOVE_RIGHT)) ||
        (entity->velX > 0 && is_button_down(BUTTON_MOVE_LEFT))
    ) sprite = 8;
    if (entity->velY > 0) sprite = 7;
    if (entity->velY < 0) sprite = 6;
    if (pouncing)         sprite = 10;
    set(entity, "facing_left", Bool, facing_left);
    *srcX = sprite * 16;
    *srcY = 0;
    *srcW = 16;
    *srcH = 16;
    *w = facing_left ? -16 : 16;
    *h = 16;
    entity_apply_squish(entity, w, h);
    return gfxcmd_custom(draw_player, context_create(
        context_int("hidden", get(entity, "hidden", Bool, false)),
        context_float("iframes", get(entity, "iframes", Float, 0)),
        context_int("powerup_state", get(entity, "powerup_state", Int, 0))
    ));
}

entity_update(player_spawner) {
    if (get(entity, "spawned", Bool, false)) return;
    camera = camera_create();
    if (current_level->default_cambound >= 0 && current_level->default_cambound < current_level->num_cambounds) {
        camera_set_bounds(camera, current_level->cambounds[current_level->default_cambound]);
    }
    LE_CreateEntity(LE_EntityGetList(entity), get_entity_builder_by_id(player), entity->posX, entity->posY);
    set(entity, "spawned", Bool, true);
}

entity_update(player) {
    int prev_id = get(entity, "prev_powerup_state", Int, POWERUP_base);
    int id = get(entity, "powerup_state", Int, POWERUP_base);
    float iframes = get(entity, "iframes", Float, 0);
    if (id == hurt) {
        if (iframes > 0) id = prev_id;
        else {
            if (savefile->hearts > 0) {
                savefile->hearts--;
                camera_screenshake(camera, 30, .5f, .5f);
                LE_Entity* heart1 = LE_CreateEntity(LE_EntityGetList(entity), get_entity_builder_by_id(broken_heart), entity->posX, entity->posY);
                LE_Entity* heart2 = LE_CreateEntity(LE_EntityGetList(entity), get_entity_builder_by_id(broken_heart), entity->posX, entity->posY);
                set(heart1, "sprite", Int, 0);
                set(heart2, "sprite", Int, 1);
                heart1->velY = heart2->velY = -.3f;
                heart1->velX = -.1f;
                heart2->velX =  .1f;
                id = prev_id;
            }
            else {
                struct Powerup* parent = get_powerup(prev_id)->parent;
                if (parent == NULL) parent = get_powerup_by_id(death);
                id = (int)((uintptr_t)parent - (uintptr_t)get_powerup(0)) / sizeof(struct Powerup);
            }
            audio_play_oneshot(GET_ASSET(struct Audio, "audio/hit.sfs"));
            iframes = 300;
        }
        savefile->powerup = id;
        set(entity, "powerup_state", Int, id);
    }
    iframes -= delta_time;
    if (iframes < 0) iframes = 0;
    set(entity, "iframes", Float, iframes);

    set(entity, "prev_powerup_state", Int, id);
    struct Powerup* powerup = get_powerup(id);
    while (powerup) {
        if (!powerup->callback(entity)) break;
        powerup = powerup->parent;
    }
}

powerup(death) {
    if (!(entity->flags & LE_EntityFlags_DisableCollision)) {
        float xpos, ypos;
        set_pause_state(PAUSE_FLAG_NO_UPDATE_CAMERA);
        LE_EntityLastDrawnPos(entity, &xpos, &ypos);
        set(entity, "xpos", Float, xpos);
        set(entity, "ypos", Float, ypos);
        set(entity, "gravity", Float, 0.001);
        entity->velX =  0.05f * (get(entity, "death_left", Bool, false) ? -1 : 1);
        entity->velY = -0.05f;
        entity->flags |= LE_EntityFlags_DisableCollision;
        audio_play_oneshot(GET_ASSET(struct Audio, "audio/death.sfs"));
        return true;
    }
    float dead_timer = get(entity, "dead_timer", Float, 0);
    dead_timer += delta_time;
    set(entity, "dead_timer", Float, dead_timer);
    if (dead_timer >= 120) start_transition(reload_level, 60, LE_Direction_Up, cubic_in_out);
    return true;
}

powerup(base) {
    bool disable_input = get(entity, "sleeping", Bool, false);
    bool pouncing = get(entity, "pouncing", Bool, false);
    float pounce_timer = get(entity, "pounce_timer", Float, 0);
    float stomp_timer = get(entity, "stomp_timer", Float, 0);
    if (!pouncing) {
        bool l = is_button_down(BUTTON_MOVE_LEFT) && !disable_input;
        bool r = is_button_down(BUTTON_MOVE_RIGHT) && !disable_input;
        if (entity->flags & LE_EntityFlags_OnGround) {
            if (is_button_pressed(BUTTON_MOVE_LEFT ) && !disable_input) entity_spawn_dust(entity, false, true, 0.2f + entity->velX);
            if (is_button_pressed(BUTTON_MOVE_RIGHT) && !disable_input) entity_spawn_dust(entity, true, false, 0.2f - entity->velX);
        }
        float accel = 0.04;
        float decel = 0.02;
        if (!(entity->flags & LE_EntityFlags_OnGround)) {
            accel /= 2;
            decel /= 4;
        }
        if (l && !r && entity->velX >= -0.2f) entity->velX -= accel * delta_time;
        else if (!l && r && entity->velX <= 0.2f) entity->velX += accel * delta_time;
        else {
            if (entity->velX < 0) {
                entity->velX += decel * delta_time;
                if (entity->velX > 0) entity->velX = 0;
            }
            if (entity->velX > 0) {
                entity->velX -= decel * delta_time;
                if (entity->velX < 0) entity->velX = 0;
            }
        }
        if (get(entity, "jumping", Bool, false)) {
            LE_Direction collided_dir;
            float height = get(entity, "jumping_from", Float, entity->posY) - entity->posY;
            set(entity, "coyote", Float, 999);
            if (height > 3 || !(is_button_down(BUTTON_JUMP) || disable_input) || (entity_collided(entity, &collided_dir) && collided_dir == LE_Direction_Down)) set(entity, "jumping", Bool, false);
            else entity->velY = -0.3f;
        }
        else if (entity_jump_requested(entity, is_button_pressed(BUTTON_JUMP) && !disable_input) & (int)entity_can_jump(entity)) {
            set(entity, "squish", Float, 1.5f);
            set(entity, "coyote", Float, 999);
            set(entity, "jumping", Bool, true);
            set(entity, "jumping_from", Float, entity->posY);
            if (!l && !r) entity->velX *= 0.6f;
            entity_spawn_dust(entity, true, true, 0.2f);
            audio_play_oneshot(GET_ASSET(struct Audio, "audio/jump.sfs"));
        }
        if (is_button_pressed(BUTTON_POUNCE) && !disable_input) {
            set(entity, "squish", Float, 1.75f);
            bool facing_left = get(entity, "facing_left", Bool, false);
            pouncing = true;
            pounce_timer = 5;
            entity->velX +=  0.3f * (facing_left ? -1 : 1);
            entity->velY  = -0.2f;
            if (entity->velX >  0.35f) entity->velX =  0.35f;
            if (entity->velX < -0.35f) entity->velX = -0.35f;
            entity_spawn_dust(entity, true, true, 0.2f);
            camera_screenshake(camera, 10, 0.5, 0.5);
            audio_play_oneshot(GET_ASSET(struct Audio, "audio/pounce.sfs"));
        }
    }
    else pounce_timer -= delta_time;
    if (stomp_timer > 0) stomp_timer -= delta_time;
    if (stomp_timer < 0) stomp_timer  = 0;
    float peak_height = get(entity, "peak_height", Float, entity->posY);
    bool prev_in_air = get(entity, "prev_in_air", Bool, false);
    if (prev_in_air && (entity->flags & LE_EntityFlags_OnGround) && entity->posY - peak_height > 5) {
        entity_spawn_dust(entity, true, true, 0.2f);
    }
    if ((entity->flags & LE_EntityFlags_OnGround) && pounce_timer <= 0) pouncing = false;
    hud_update(entity);
    if (!disable_input) {
        float camX = entity->posX;
        float camY = entity->posY - 0.5f;
        LE_LayerListIter* iter = LE_LayerListGetIter(current_level->layers);
        while (iter) {
            LE_Layer* layer = LE_LayerListGet(iter);
            if (LE_LayerGetDataPointer(layer) == LE_EntityGetList(entity)) {
                LE_LayerToGlobalSpace(layer, camX, camY, &camX, &camY);
                break;
            }
            iter = LE_LayerListNext(iter);
        }
        camera_set_focus(camera, camX, camY);
    }
    entity_fall_squish(entity, 10, .5f, .25f);
    entity_update_squish(entity, 5);
    set(entity, "pouncing", Bool, pouncing);
    set(entity, "pounce_timer", Float, pounce_timer);
    set(entity, "stomp_timer", Float, stomp_timer);
    return false;
}

powerup(test) {
    bool hidden = get(entity, "hidden", Bool, false);
    set(entity, "prev_hidden", Bool, hidden);
    hidden = is_button_down(BUTTON_MOVE_DOWN);
    set(entity, "hidden", Bool, hidden);
    return true;
}
