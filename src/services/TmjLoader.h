#pragma once
#include <raylib.h>
#include <string>
#include <vector>
#include <unordered_map>

// ---------------------------------------------------------------------------
// Data structures loaded from TMJ (Tiled Map JSON) files
// ---------------------------------------------------------------------------

/// A one-directional portal as defined in a TMJ "Portals" object layer.
/// Each portal object must have these custom properties:
///   to_scene   (string) — destination scene id
///   to_spawn   (string) — spawn_id in the destination scene
///   requires_e (bool)   — 1/true if the player must press E to activate
///   portal_id  (string) — optional debug identifier
struct TmjPortalDef {
    std::string portalId;     ///< portal_id property (may be empty)
    std::string fromScene;    ///< Scene this portal lives in (set by loader)
    Rectangle   triggerRect;  ///< Activation rectangle in the scene's pixel space
    std::string toScene;      ///< Destination scene name
    std::string toSpawnId;    ///< spawn_id inside the destination scene
    bool        requiresE{false};
};

/// A named spawn point as defined in a TMJ "Spawns" object layer.
/// Each object must have a custom property:
///   spawn_id (string) — unique identifier for this spawn within the scene
struct TmjSpawnDef {
    std::string spawnId;   ///< spawn_id property
    Vector2     position;  ///< Centre of the object rectangle
};

/// A floor-menu trigger as defined in a TMJ "FloorTriggers" object layer.
/// Object name must be one of: "elevator", "stair_left", "stair_right".
struct TmjFloorTriggerDef {
    std::string triggerType;  ///< "elevator" | "stair_left" | "stair_right"
    Rectangle   triggerRect;
};

// ---------------------------------------------------------------------------
// Loader functions (all read from the given .tmj path; return empty on error)
// ---------------------------------------------------------------------------

/// Load portal definitions from the "Portals" object layer.
std::vector<TmjPortalDef>
loadPortalsFromTmj(const std::string& tmjPath, const std::string& sceneName);

/// Load spawn definitions from the "Spawns" object layer.
/// Returns spawn_id → centre position.
std::unordered_map<std::string, Vector2>
loadSpawnsFromTmj(const std::string& tmjPath);

/// Load floor-menu trigger rectangles from the "FloorTriggers" object layer.
std::vector<TmjFloorTriggerDef>
loadFloorTriggersFromTmj(const std::string& tmjPath);

/// Load collision hitboxes from all object layers that are NOT
/// "Portals", "Spawns", or "FloorTriggers" (case-insensitive check).
std::vector<Rectangle>
loadHitboxesFromTmj(const std::string& tmjPath);
