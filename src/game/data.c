#include "data.h"

#include "assets/assets.h"
#include "game/entities/functions.h"
#include "game/tiles/functions.h"
#include "main.h"

#include <stdint.h>
#include <string.h>

LE_EntityBuilder* entity_builders[256];
LE_Tileset* tilesets[256];
LE_Tileset* themes[256][256];
LE_TileData* tile_data[256][256];

#define NO_VSCODE

#define TILE_PALETTE(id, data) {                                   \
    LE_TileData** curr_tile_palette = tile_data[TILE_PALETTE_##id]; \
    data                                                             \
}

#define TILE(id, data) if (tiledata == curr_tile_palette[TILE_DATA_##id]) { data }
#define COLLISION(        func)
#define TEXTURE(          func)
#define SOLID(                )
#define LVLEDIT_HIDE(         )
#define LVLEDIT_TEXTURE(    _ )
#define LVLEDIT_PROPERTIES(...)
#define SIMPLE_ANIMATED_TEXTURE(num, delay, ...) return ((int[]){__VA_ARGS__})[(global_timer / delay) % num];
#define SIMPLE_STATIONARY_TEXTURE(_1) SIMPLE_ANIMATED_TEXTURE(1, 1, _1)

int simple_tile_texture_provider(LE_TileData* tiledata) {
#include "game/data/tiles.h"
    return -1;
}

#undef TILE
#undef COLLISION
#undef TEXTURE
#undef SOLID
#undef SIMPLE_ANIMATED_TEXTURE
#undef THEME

#define ENUM(_)
#define ENTITY(id, data) entity_builders[ENTITY_BUILDER_##id] = ({ \
    LE_EntityBuilder* builder = LE_CreateEntityBuilder();           \
    data                                                             \
    builder;                                                          \
});
#define TILE(id, data) curr_tile_palette[TILE_DATA_##id] = ({ \
    LE_TileData* tile = LE_CreateTileData();                   \
    data                                                        \
    tile;                                                        \
});
#define TILESET(id, data) tilesets[TILESET_##id] = ({    \
    LE_Tileset* tileset = LE_CreateTileset();             \
    data                                                   \
    for (int i = 0; i < 256; i++) {                         \
        LE_TilesetAddTile(tileset, (tile_data[palette])[i]); \
    }                                                         \
    tileset;                                                   \
});
#define THEME(id, data) {        \
    int theme_index = THEME_##id; \
    data                           \
}

void init_data() {
    memset(entity_builders, 0, sizeof(entity_builders));
    memset(tile_data, 0, sizeof(tile_data));
    memset(tilesets, 0, sizeof(tilesets));

#define UPDATE(   func) LE_EntityBuilderAddUpdateCallback   (builder, func );
#define COLLISION(func) LE_EntityBuilderAddCollisionCallback(builder, func );
#define TEXTURE(  func) LE_EntityBuilderAddTextureCallback  (builder, func );
#define FLAGS(   flags) LE_EntityBuilderSetFlags            (builder, flags);
#define SIZE(     w, h) LE_EntityBuilderSetHitboxSize       (builder, w, h );
#include "game/data/entities.h"
#undef UPDATE
#undef COLLISION
#undef TEXTURE
#undef FLAGS
#undef SIZE

#define COLLISION(func) LE_TileAddCollisionCallback(tile, func);
#define TEXTURE(  func) LE_TileAddTextureCallback  (tile, func);
#define SOLID(        ) LE_TileSetSolid            (tile, true);
#define SIMPLE_ANIMATED_TEXTURE(num, delay, ...) TEXTURE(simple_tile_texture_provider)
#include "game/data/tiles.h"
#undef COLLISION
#undef TEXTURE
#undef SOLID
#undef SIMPLE_ANIMATED_TEXTURE
#undef SIMPLE_STATIONARY_TEXTURE

#define TEXTURE(     tex ) LE_TilesetSetTexture   (tileset, GET_ASSET(struct Texture, tex));
#define SIZE(        w, h) LE_TilesetSetTileSize  (tileset, w, h);
#define TILES_IN_ROW(x   ) LE_TilesetSetTilesInRow(tileset, x   );
#define PALETTE(     id  ) int palette = TILE_PALETTE_##id;
#include "game/data/tilesets.h"
#undef TEXTURE
#undef SIZE
#undef TILES_IN_ROW

#define THEME_TILESET(type, id) themes[theme_index][TILESET_TYPE_##type] = tilesets[TILESET_##id];
#include "game/data/themes.h"
#undef THEME_TILESET
}

LE_EntityBuilder* get_entity_builder(enum EntityBuilderIDs id) {
    return entity_builders[id];
}

LE_Tileset* get_tileset(enum TilesetIDs id) {
    return tilesets[id];
}

LE_Tileset** get_theme(enum ThemeIDs id) {
    return themes[id];
}

LE_TileData** get_tile_palette(enum TilePaletteIDs id) {
    return tile_data[id];
}