#include "level.h"

#include <stdlib.h>
#include <lunarengine.h>
#include <string.h>

#include "io/assets/binary_reader.h"
#include "io/audio/nsf.h"
#include "io/audio/audio.h"
#include "camera.h"
#include "data.h"
#include "input.h"
#include "overlay/manager.h"
#include "overlay/transition.h"
#include "main.h"

struct AudioInstance* music_instance;
struct Level* current_level = NULL;
uint8_t curr_level_id = 0;
uint32_t unique_entity_id = 2;
Camera* camera;

void change_level_music(int track) {
    struct Audio* nsf = GET_ASSET(struct Audio, "audio/music.nsf");
    if (music_instance) {
        audio_stop(music_instance);
    }
    audio_nsf_select_track(nsf, track);
    music_instance = audio_play(nsf);
}

void destroy_level(struct Level* level) {
    for (int i = 0; i < level->num_cambounds; i++) {
        free(level->cambounds[i]);
    }
    free(level->cambounds);
    free(level->warps);
    LE_LayerListIter* iter = LE_LayerListGetIter(level->layers);
    while (iter) {
        LE_Layer* layer = LE_LayerListGet(iter);
        switch (LE_LayerGetType(layer)) {
            case LE_LayerType_Entity:
                LE_DestroyEntityList(LE_LayerGetDataPointer(layer));
                break;
            case LE_LayerType_Tilemap:
                LE_DestroyTilemap(LE_LayerGetDataPointer(layer));
                break;
            default:
                break;
        }
        iter = LE_LayerListNext(iter);
    }
    LE_DestroyLayerList(level->layers);
    free(level);
}

uint32_t get_unique_entity_id() {
    return unique_entity_id++;
}

struct Level* parse_level(unsigned char* data, int datalen) {
    struct Level* level = malloc(sizeof(struct Level));
    struct BinaryStream* stream = binary_stream_create(data);

    level->raw = data;
    level->raw_length = datalen;

    stream = binary_stream_goto(stream);
    BINARY_STREAM_READ(stream, level->default_theme);
    BINARY_STREAM_READ(stream, level->default_music);
    BINARY_STREAM_READ(stream, level->default_cambound);
    stream = binary_stream_close(stream);
    level->default_music = 3;

    stream = binary_stream_goto(stream);
    BINARY_STREAM_READ(stream, level->num_cambounds);
    level->cambounds = malloc(sizeof(struct CameraBounds*) * level->num_cambounds);
    for (unsigned int i = 0; i < level->num_cambounds; i++) {
        stream = binary_stream_goto(stream);
        unsigned int num_nodes, num_edges;
        BINARY_STREAM_READ(stream, num_nodes);
        BINARY_STREAM_READ(stream, num_edges);
        struct CameraBounds* nodes = malloc(sizeof(struct CameraBounds) * num_nodes);
        for (unsigned int j = 0; j < num_nodes; j++) {
            BINARY_STREAM_READ(stream, nodes[j].x);
            BINARY_STREAM_READ(stream, nodes[j].y);
        }
        for (unsigned int j = 0; j < num_edges; j++) {
            unsigned int a, b;
            BINARY_STREAM_READ(stream, a);
            BINARY_STREAM_READ(stream, b);
            if (nodes[a].x == nodes[b].x && nodes[a].y == nodes[b].y - 1) {
                nodes[a].up = &nodes[b];
                nodes[b].down = &nodes[a];
            }
            if (nodes[a].x == nodes[b].x - 1 && nodes[a].y == nodes[b].y) {
                nodes[a].left = &nodes[b];
                nodes[b].right = &nodes[a];
            }
            if (nodes[a].x == nodes[b].x && nodes[a].y == nodes[b].y + 1) {
                nodes[a].down = &nodes[b];
                nodes[b].up = &nodes[a];
            }
            if (nodes[a].x == nodes[b].x + 1 && nodes[a].y == nodes[b].y) {
                nodes[a].right = &nodes[b];
                nodes[b].left = &nodes[a];
            }
        }
        level->cambounds[i] = nodes;
        stream = binary_stream_close(stream);
    }
    stream = binary_stream_close(stream);

    stream = binary_stream_goto(stream);
    BINARY_STREAM_READ(stream, level->num_warps);
    level->warps = malloc(sizeof(struct Warp) * level->num_warps);
    for (int i = 0; i < level->num_warps; i++) {
        stream = binary_stream_goto(stream);
        BINARY_STREAM_READ(stream, level->warps[i].next_theme);
        BINARY_STREAM_READ(stream, level->warps[i].next_music);
        BINARY_STREAM_READ(stream, level->warps[i].next_cambound);
        BINARY_STREAM_READ(stream, level->warps[i].next_level);
        BINARY_STREAM_READ(stream, level->warps[i].next_layer);
        BINARY_STREAM_READ(stream, level->warps[i].next_pos_x);
        BINARY_STREAM_READ(stream, level->warps[i].next_pos_y);
        stream = binary_stream_close(stream);
    }
    stream = binary_stream_close(stream);

    stream = binary_stream_goto(stream);
    unsigned int num_layers;
    BINARY_STREAM_READ(stream, num_layers);
    level->layers = LE_CreateLayerList();
    LE_AddCustomLayer(level->layers, layer_overlay);
    for (int i = 0; i < num_layers; i++) {
        stream = binary_stream_goto(stream);
        unsigned int type;
        float smx, smy, sox, soy, scx, scy;
        BINARY_STREAM_READ(stream, type);
        BINARY_STREAM_READ(stream, smx);
        BINARY_STREAM_READ(stream, smy);
        BINARY_STREAM_READ(stream, sox);
        BINARY_STREAM_READ(stream, soy);
        BINARY_STREAM_READ(stream, scx);
        BINARY_STREAM_READ(stream, scy);
        LE_Layer* layer;
        stream = binary_stream_goto(stream);
        switch (type) {
        case LE_LayerType_Tilemap: {
            int tileset;
            unsigned int w, h;
            BINARY_STREAM_READ(stream, tileset);
            BINARY_STREAM_READ(stream, w);
            BINARY_STREAM_READ(stream, h);
            LE_Tilemap* tilemap = LE_CreateTilemap(w, h);
            for (unsigned int y = 0; y < h; y++) {
                for (unsigned int x = 0; x < w; x++) {
                    unsigned char tile;
                    BINARY_STREAM_READ(stream, tile);
                    LE_TilemapSetTile(tilemap, x, y, tile);
                }
            }
            LE_TilemapSetTileset(tilemap, get_theme(level->default_theme)[tileset]);
            layer = LE_AddTilemapLayer(level->layers, tilemap);
        } break;
        case LE_LayerType_Entity: {
            LE_EntityList* el = LE_CreateEntityList();
            unsigned int tilemap, num_entities;
            BINARY_STREAM_READ(stream, tilemap);
            BINARY_STREAM_READ(stream, num_entities);
            LE_EntityAssignTilemap(el, (LE_Tilemap*)(uintptr_t)tilemap);
            for (unsigned int j = 0; j < num_entities; j++) {
                stream = binary_stream_goto(stream);
                unsigned char entityID;
                BINARY_STREAM_READ(stream, entityID);
                binary_stream_skip(stream, 3);
                float x, y;
                BINARY_STREAM_READ(stream, x);
                BINARY_STREAM_READ(stream, y);
                LE_Entity* entity = LE_CreateEntity(el, get_entity_builder(entityID), x, y);
                unsigned int num_properties;
                BINARY_STREAM_READ(stream, num_properties);
                for (unsigned int i = 0; i < num_properties; i++) {
                    char name[256];
                    binary_stream_read_string(stream, name, 256);
                    unsigned int value;
                    BINARY_STREAM_READ(stream, value);
                    LE_EntityProperty property;
                    property.asInt = value;
                    LE_EntitySetProperty(entity, property, name);
                }
                stream = binary_stream_close(stream);
            }
            layer = LE_AddEntityLayer(level->layers, el);
        } break; }
        layer->scrollSpeedX = smx * 16;
        layer->scrollSpeedY = smy * 16;
        layer->scrollOffsetX = sox;
        layer->scrollOffsetY = soy;
        layer->scaleW = scx;
        layer->scaleH = scy;
        stream = binary_stream_close(stream);
        stream = binary_stream_close(stream);
    }
    stream = binary_stream_close(stream);

    LE_LayerListIter* iter = LE_LayerListGetIter(level->layers);
    while (iter) {
        LE_Layer* layer = LE_LayerListGet(iter);
        enum LE_LayerType type = LE_LayerGetType(layer);
        if (type == LE_LayerType_Entity) {
            LE_EntityList* entitylist = LE_LayerGetDataPointer(layer);
            int tilemap_index = (int)(uintptr_t)LE_EntityGetTilemap(entitylist);
            if (tilemap_index >= 0) {
                LE_Layer* tilemap = LE_LayerGetByIndex(level->layers, tilemap_index + 1);
                LE_EntityAssignTilemap(entitylist, LE_LayerGetDataPointer(tilemap));
                layer->scaleW *= tilemap->scaleW;
                layer->scaleH *= tilemap->scaleH;
            }
        }
        iter = LE_LayerListNext(iter);
    }

    return level;
}

void load_level(struct Binary* binary) {
    load_level_impl(binary->ptr, binary->length);
}

void load_level_impl(unsigned char* data, int datalen) {
    if (current_level) destroy_level(current_level);
    unique_entity_id = 0;
    current_level = parse_level(data, datalen);
    change_level_music(current_level->default_music);
}

void reload_level() {
    unsigned char* leveldata = malloc(current_level->raw_length);
    memcpy(leveldata, current_level->raw, current_level->raw_length);
    load_level_impl(leveldata, current_level->raw_length);
    free(leveldata);
}

void update_level() {
    LE_LayerListIter* iter = LE_LayerListGetIter(current_level->layers);
    while (iter) {
        LE_Layer* layer = LE_LayerListGet(iter);
        enum LE_LayerType type = LE_LayerGetType(layer);
        if (type == LE_LayerType_Entity) LE_UpdateEntities(LE_LayerGetDataPointer(layer));
        iter = LE_LayerListNext(iter);
    }
}

void render_level(Camera* camera, LE_DrawList* drawlist, int width, int height) {
    float x, y;
    camera_update(camera);
    camera_get(camera, &x, &y);
    LE_ScrollCamera(current_level->layers, x, y);
    LE_Draw(current_level->layers, width, height, drawlist);
}
