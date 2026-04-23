#include "TmjLoader.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}

/// Read a custom-property value from a Tiled object's "properties" array.
/// Returns an empty json if the property is not found.
static json getProperty(const json& obj, const std::string& propName) {
    if (!obj.contains("properties") || !obj["properties"].is_array()) return {};
    for (const auto& p : obj["properties"]) {
        if (p.value("name", "") == propName) return p["value"];
    }
    return {};
}

static bool loadTmj(const std::string& path, json& out) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[TmjLoader] Cannot open: " << path << "\n";
        return false;
    }
    try {
        file >> out;
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "[TmjLoader] Parse error in " << path << ": " << ex.what() << "\n";
        return false;
    }
}

/// True for layers whose name indicates they hold portal/spawn/floor data
/// (and therefore should NOT be treated as hitboxes).
static bool isNonCollisionLayerName(const std::string& name) {
    const std::string lower = toLower(name);
    return lower == "portals" || lower == "spawns" || lower == "floortriggers";
}

// ---------------------------------------------------------------------------
// loadHitboxesFromTmj
// ---------------------------------------------------------------------------

std::vector<Rectangle> loadHitboxesFromTmj(const std::string& tmjPath) {
    json tmj;
    if (!loadTmj(tmjPath, tmj)) return {};

    std::vector<Rectangle> hitboxes;
    if (!tmj.contains("layers") || !tmj["layers"].is_array()) return hitboxes;

    for (const auto& layer : tmj["layers"]) {
        if (layer.value("type", "") != "objectgroup") continue;
        if (isNonCollisionLayerName(layer.value("name", ""))) continue;
        if (!layer.contains("objects") || !layer["objects"].is_array()) continue;

        for (const auto& obj : layer["objects"]) {
            if (!obj.contains("x") || !obj.contains("y") ||
                !obj.contains("width") || !obj.contains("height")) continue;
            Rectangle r{};
            r.x      = obj["x"].get<float>();
            r.y      = obj["y"].get<float>();
            r.width  = obj["width"].get<float>();
            r.height = obj["height"].get<float>();
            hitboxes.push_back(r);
        }
    }
    return hitboxes;
}

// ---------------------------------------------------------------------------
// loadPortalsFromTmj
// ---------------------------------------------------------------------------

std::vector<TmjPortalDef>
loadPortalsFromTmj(const std::string& tmjPath, const std::string& sceneName) {
    json tmj;
    if (!loadTmj(tmjPath, tmj)) return {};

    std::vector<TmjPortalDef> portals;
    if (!tmj.contains("layers") || !tmj["layers"].is_array()) return portals;

    for (const auto& layer : tmj["layers"]) {
        if (layer.value("type", "") != "objectgroup") continue;
        if (toLower(layer.value("name", "")) != "portals") continue;
        if (!layer.contains("objects") || !layer["objects"].is_array()) continue;

        for (const auto& obj : layer["objects"]) {
            const json toSceneVal  = getProperty(obj, "to_scene");
            const json toSpawnVal  = getProperty(obj, "to_spawn");
            if (toSceneVal.is_null() || toSpawnVal.is_null()) {
                std::cerr << "[TmjLoader] Portal object missing to_scene/to_spawn in "
                          << tmjPath << "\n";
                continue;
            }

            TmjPortalDef p;
            p.fromScene   = sceneName;
            p.toScene     = toSceneVal.get<std::string>();
            p.toSpawnId   = toSpawnVal.get<std::string>();
            const json reqE = getProperty(obj, "requires_e");
            p.requiresE   = reqE.is_boolean() ? reqE.get<bool>()
                          : reqE.is_number()  ? (reqE.get<int>() != 0)
                          : false;
            const json pidVal = getProperty(obj, "portal_id");
            p.portalId    = pidVal.is_string() ? pidVal.get<std::string>()
                                               : obj.value("name", "");

            p.triggerRect.x      = obj.value("x",      0.0f);
            p.triggerRect.y      = obj.value("y",      0.0f);
            p.triggerRect.width  = obj.value("width",  1.0f);
            p.triggerRect.height = obj.value("height", 1.0f);

            portals.push_back(p);
        }
    }
    return portals;
}

// ---------------------------------------------------------------------------
// loadSpawnsFromTmj
// ---------------------------------------------------------------------------

std::unordered_map<std::string, Vector2>
loadSpawnsFromTmj(const std::string& tmjPath) {
    json tmj;
    if (!loadTmj(tmjPath, tmj)) return {};

    std::unordered_map<std::string, Vector2> spawns;
    if (!tmj.contains("layers") || !tmj["layers"].is_array()) return spawns;

    for (const auto& layer : tmj["layers"]) {
        if (layer.value("type", "") != "objectgroup") continue;
        if (toLower(layer.value("name", "")) != "spawns") continue;
        if (!layer.contains("objects") || !layer["objects"].is_array()) continue;

        for (const auto& obj : layer["objects"]) {
            const json sidVal = getProperty(obj, "spawn_id");
            if (sidVal.is_null()) {
                std::cerr << "[TmjLoader] Spawn object missing spawn_id in "
                          << tmjPath << "\n";
                continue;
            }
            const std::string sid = sidVal.get<std::string>();
            const float x = obj.value("x", 0.0f) + obj.value("width",  0.0f) * 0.5f;
            const float y = obj.value("y", 0.0f) + obj.value("height", 0.0f) * 0.5f;
            spawns[sid] = Vector2{x, y};
        }
    }
    return spawns;
}

// ---------------------------------------------------------------------------
// loadFloorTriggersFromTmj
// ---------------------------------------------------------------------------

std::vector<TmjFloorTriggerDef>
loadFloorTriggersFromTmj(const std::string& tmjPath) {
    json tmj;
    if (!loadTmj(tmjPath, tmj)) return {};

    std::vector<TmjFloorTriggerDef> triggers;
    if (!tmj.contains("layers") || !tmj["layers"].is_array()) return triggers;

    for (const auto& layer : tmj["layers"]) {
        if (layer.value("type", "") != "objectgroup") continue;
        if (toLower(layer.value("name", "")) != "floortriggers") continue;
        if (!layer.contains("objects") || !layer["objects"].is_array()) continue;

        for (const auto& obj : layer["objects"]) {
            TmjFloorTriggerDef ft;
            ft.triggerType        = obj.value("name", "");
            ft.triggerRect.x      = obj.value("x",      0.0f);
            ft.triggerRect.y      = obj.value("y",      0.0f);
            ft.triggerRect.width  = obj.value("width",  1.0f);
            ft.triggerRect.height = obj.value("height", 1.0f);
            if (!ft.triggerType.empty()) triggers.push_back(ft);
        }
    }
    return triggers;
}
