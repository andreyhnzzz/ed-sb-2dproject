#include "DataManager.h"

#include "../services/TmjLoader.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <limits>
#include <nlohmann/json.hpp>
#include <stdexcept>

using json = nlohmann::json;

namespace {
std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

Vector2 resolveAnchorPoint(
    const json& definition,
    const std::unordered_map<std::string, std::unordered_map<std::string, Vector2>>& spawnCache,
    const std::string& defaultScene,
    const std::string& defaultSpawnId) {
    if (definition.contains("x") && definition.contains("y")) {
        return Vector2{
            definition["x"].get<float>(),
            definition["y"].get<float>()
        };
    }

    const std::string scene = toLower(definition.value("scene", defaultScene));
    const std::string spawnId = definition.value("spawn_id", defaultSpawnId);
    const auto sceneIt = spawnCache.find(scene);
    if (sceneIt == spawnCache.end()) {
        throw std::runtime_error("No TMJ data for scene: " + scene);
    }
    const auto spawnIt = sceneIt->second.find(spawnId);
    if (spawnIt == sceneIt->second.end()) {
        throw std::runtime_error("Spawn not found: " + scene + "::" + spawnId);
    }
    return spawnIt->second;
}

double euclideanMeters(const Vector2& from, const Vector2& to, double pixelsToMeters) {
    const double dx = static_cast<double>(to.x - from.x);
    const double dy = static_cast<double>(to.y - from.y);
    return std::sqrt(dx * dx + dy * dy) * pixelsToMeters;
}

bool isStairType(const std::string& type) {
    const std::string lowered = toLower(type);
    return lowered.find("escalera") != std::string::npos || lowered.find("stair") != std::string::npos;
}
} // namespace

std::unordered_map<std::string, std::unordered_map<std::string, Vector2>>
DataManager::loadSpawnCache(
    const std::unordered_map<std::string, std::string>& sceneToTmjPath) const {
    std::unordered_map<std::string, std::unordered_map<std::string, Vector2>> cache;
    for (const auto& [sceneId, tmjPath] : sceneToTmjPath) {
        cache[toLower(sceneId)] = loadSpawnsFromTmj(tmjPath);
    }
    return cache;
}

CampusGraph DataManager::loadCampusGraph(
    const std::string& configPath,
    const std::unordered_map<std::string, std::string>& sceneToTmjPath,
    double pixelsToMeters) const {
    std::ifstream input(configPath);
    if (!input.is_open()) {
        throw std::runtime_error("Cannot open campus config: " + configPath);
    }

    json data = json::parse(input);
    if (!data.contains("nodes") || !data["nodes"].is_array() ||
        !data.contains("edges") || !data["edges"].is_array()) {
        throw std::runtime_error("Campus config must contain arrays: nodes, edges");
    }

    const auto spawnCache = loadSpawnCache(sceneToTmjPath);
    CampusGraph graph;
    for (const auto& jn : data["nodes"]) {
        Node node;
        node.id = toLower(jn.at("id").get<std::string>());
        node.name = jn.value("name", node.id);
        node.type = jn.value("type", "");

        const std::string scene = toLower(jn.value("scene", ""));
        const std::string spawnId = jn.value("spawn_id", "");
        const Vector2 pos = resolveAnchorPoint(jn, spawnCache, scene, spawnId);
        node.x = pos.x;
        node.y = pos.y;
        graph.addNode(node);
    }

    for (const auto& je : data["edges"]) {
        const std::string from = toLower(je.at("from").get<std::string>());
        const std::string to = toLower(je.at("to").get<std::string>());
        if (!graph.hasNode(from) || !graph.hasNode(to)) {
            throw std::runtime_error("Edge references unknown node: " + from + " -> " + to);
        }

        const json defaultFromAnchor = json{
            {"x", graph.getNode(from).x},
            {"y", graph.getNode(from).y}
        };
        const json defaultToAnchor = json{
            {"x", graph.getNode(to).x},
            {"y", graph.getNode(to).y}
        };
        const Vector2 fromAnchor = je.contains("from_anchor")
            ? resolveAnchorPoint(je["from_anchor"], spawnCache, "", "")
            : resolveAnchorPoint(defaultFromAnchor, spawnCache, "", "");
        const Vector2 toAnchor = je.contains("to_anchor")
            ? resolveAnchorPoint(je["to_anchor"], spawnCache, "", "")
            : resolveAnchorPoint(defaultToAnchor, spawnCache, "", "");

        const std::string type = je.value("type", "Portal");
        const double distanceMeters = euclideanMeters(fromAnchor, toAnchor, pixelsToMeters);

        Edge edge;
        edge.id = je.value("id", from + "_" + to + "_" + toLower(type));
        edge.from = from;
        edge.to = to;
        edge.type = type;
        edge.base_weight = distanceMeters;
        edge.blocked_for_mr = isStairType(type);
        edge.mobility_weight = edge.blocked_for_mr
            ? std::numeric_limits<double>::infinity()
            : distanceMeters;
        graph.addEdge(edge);
    }

    return graph;
}

void DataManager::exportResolvedGraph(const CampusGraph& graph, const std::string& outputPath) const {
    json out;
    out["nodes"] = json::array();
    out["edges"] = json::array();

    for (const auto& id : graph.nodeIds()) {
        const auto& node = graph.getNode(id);
        out["nodes"].push_back({
            {"id", node.id},
            {"name", node.name},
            {"type", node.type},
            {"x", node.x},
            {"y", node.y},
            {"z", node.z}
        });
    }

    for (const auto& id : graph.nodeIds()) {
        for (const auto& edge : graph.edgesFrom(id)) {
            if (edge.from > edge.to) continue;
            out["edges"].push_back({
                {"id", edge.id},
                {"from", edge.from},
                {"to", edge.to},
                {"type", edge.type},
                {"base_weight", edge.base_weight},
                {"mobility_weight", edge.mobility_weight},
                {"blocked_for_mr", edge.blocked_for_mr}
            });
        }
    }

    std::ofstream output(outputPath);
    if (!output.is_open()) {
        throw std::runtime_error("Cannot write resolved graph: " + outputPath);
    }
    output << out.dump(2);
}
