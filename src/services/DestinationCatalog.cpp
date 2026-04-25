#include "DestinationCatalog.h"

#include "StringUtils.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {
std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string normalizeLabel(const std::string& label) {
    return toLower(label);
}

bool labelsMatch(const std::string& a, const std::string& b) {
    return normalizeLabel(a) == normalizeLabel(b);
}

std::string inferScenarioTag(const std::string& type, const std::string& label) {
    const std::string loweredType = toLower(type);
    const std::string loweredLabel = toLower(label);
    if (loweredType.find("escal") != std::string::npos || loweredLabel.find("escal") != std::string::npos) {
        return "stairs";
    }
    if (loweredType.find("rampa") != std::string::npos || loweredLabel.find("rampa") != std::string::npos) {
        return "ramp";
    }
    if (loweredType.find("elev") != std::string::npos || loweredLabel.find("elev") != std::string::npos) {
        return "elevator";
    }
    if (loweredLabel.find("entrada") != std::string::npos || loweredLabel.find("acceso") != std::string::npos) {
        return "entry";
    }
    if (loweredLabel.find("bibli") != std::string::npos) {
        return "reference";
    }
    if (loweredLabel.find("soda") != std::string::npos ||
        loweredLabel.find("cafeter") != std::string::npos ||
        loweredLabel.find("comedor") != std::string::npos) {
        return "reference";
    }
    if (loweredType.find("poi") != std::string::npos) {
        return "poi";
    }
    return loweredType.empty() ? "node" : loweredType;
}

std::string inferSceneId(const json& nodeJson) {
    const std::string nodeId = toLower(nodeJson.value("id", ""));
    if (nodeJson.contains("scene") && nodeJson["scene"].is_string()) {
        return toLower(nodeJson["scene"].get<std::string>());
    }
    const std::string type = toLower(nodeJson.value("type", ""));
    if (type.find("poi") == std::string::npos) {
        return nodeId;
    }
    return "";
}

std::vector<Rectangle> matchZoneRects(const std::unordered_map<std::string, SceneData>& sceneDataMap,
                                      const std::string& sceneId,
                                      const std::string& label) {
    std::vector<Rectangle> rects;
    for (const auto& [sceneName, sceneData] : sceneDataMap) {
        if (toLower(sceneName) != sceneId) continue;
        for (const auto& zone : sceneData.interestZones) {
            if (!labelsMatch(zone.name, label)) continue;
            rects = zone.rects;
            return rects;
        }
    }
    return rects;
}

std::string makePoiKey(const std::string& sceneNodeId, const std::string& poiNodeId) {
    return toLower(sceneNodeId) + "|" + toLower(poiNodeId);
}
} // namespace

void DestinationCatalog::clear() {
    destinations_.clear();
    poiEdges_.clear();
    destinationIndexByNodeId_.clear();
    poiEdgeIndexByKey_.clear();
}

bool DestinationCatalog::loadFromGeneratedJson(
    const std::string& generatedGraphPath,
    const std::unordered_map<std::string, SceneData>& sceneDataMap) {
    clear();

    std::ifstream input(generatedGraphPath);
    if (!input.is_open()) {
        return false;
    }

    json data = json::parse(input, nullptr, false);
    if (data.is_discarded() || !data.contains("nodes") || !data.contains("edges")) {
        return false;
    }

    for (const auto& nodeJson : data["nodes"]) {
        NavigationDestination destination;
        destination.nodeId = toLower(nodeJson.value("id", ""));
        if (destination.nodeId.empty()) continue;

        destination.label = nodeJson.value("name", destination.nodeId);
        destination.type = nodeJson.value("type", "");
        destination.sceneId = inferSceneId(nodeJson);
        destination.worldPos = Vector2{
            nodeJson.value("x", 0.0f),
            nodeJson.value("y", 0.0f)
        };
        destination.isPoi = toLower(destination.type).find("poi") != std::string::npos;
        destination.scenarioTag = inferScenarioTag(destination.type, destination.label);
        if (destination.isPoi && !destination.sceneId.empty()) {
            destination.zoneRects = matchZoneRects(sceneDataMap, destination.sceneId, destination.label);
        }

        destinationIndexByNodeId_[destination.nodeId] = destinations_.size();
        destinations_.push_back(std::move(destination));
    }

    for (const auto& edgeJson : data["edges"]) {
        const std::string type = toLower(edgeJson.value("type", ""));
        if (type != "poi") continue;

        const std::string fromNodeId = toLower(edgeJson.value("from", ""));
        const std::string toNodeId = toLower(edgeJson.value("to", ""));
        auto destinationIt = destinationIndexByNodeId_.find(toNodeId);
        if (destinationIt == destinationIndexByNodeId_.end()) continue;
        NavigationDestination& destination = destinations_[destinationIt->second];
        if (destination.sceneId.empty()) {
            destination.sceneId = fromNodeId;
            if (destination.zoneRects.empty()) {
                destination.zoneRects = matchZoneRects(sceneDataMap, destination.sceneId, destination.label);
            }
        }

        PoiEdgeMetadata metadata;
        metadata.edgeId = edgeJson.value("id", fromNodeId + "_" + toNodeId);
        metadata.fromNodeId = fromNodeId;
        metadata.toNodeId = toNodeId;
        metadata.sceneId = destination.sceneId.empty() ? fromNodeId : destination.sceneId;
        metadata.label = destination.label;
        metadata.scenarioTag = destination.scenarioTag;
        metadata.worldPos = destination.worldPos;
        metadata.collisionRects = destination.zoneRects;
        if (metadata.collisionRects.empty()) {
            metadata.collisionRects.push_back(Rectangle{
                metadata.worldPos.x - 18.0f,
                metadata.worldPos.y - 18.0f,
                36.0f,
                36.0f
            });
        }

        poiEdgeIndexByKey_[makePoiKey(metadata.fromNodeId, metadata.toNodeId)] = poiEdges_.size();
        poiEdges_.push_back(std::move(metadata));
    }

    std::sort(destinations_.begin(), destinations_.end(), [](const NavigationDestination& a,
                                                             const NavigationDestination& b) {
        if (a.isPoi != b.isPoi) return a.isPoi > b.isPoi;
        return StringUtils::toLowerCopy(a.label) < StringUtils::toLowerCopy(b.label);
    });
    destinationIndexByNodeId_.clear();
    for (size_t i = 0; i < destinations_.size(); ++i) {
        destinationIndexByNodeId_[destinations_[i].nodeId] = i;
    }

    return !destinations_.empty();
}

bool DestinationCatalog::hasDestination(const std::string& nodeId) const {
    return destinationIndexByNodeId_.count(toLower(nodeId)) > 0;
}

const NavigationDestination* DestinationCatalog::findDestination(const std::string& nodeId) const {
    const auto it = destinationIndexByNodeId_.find(toLower(nodeId));
    if (it == destinationIndexByNodeId_.end()) return nullptr;
    return &destinations_[it->second];
}

const PoiEdgeMetadata* DestinationCatalog::findPoiEdge(const std::string& sceneNodeId,
                                                       const std::string& poiNodeId) const {
    const auto it = poiEdgeIndexByKey_.find(makePoiKey(sceneNodeId, poiNodeId));
    if (it == poiEdgeIndexByKey_.end()) return nullptr;
    return &poiEdges_[it->second];
}

std::vector<PoiEdgeMetadata> DestinationCatalog::poiEdgesForScene(const std::string& sceneId) const {
    std::vector<PoiEdgeMetadata> out;
    const std::string canonicalScene = toLower(sceneId);
    for (const auto& edge : poiEdges_) {
        if (edge.sceneId == canonicalScene) out.push_back(edge);
    }
    return out;
}

std::string DestinationCatalog::displayLabel(const std::string& nodeId) const {
    const NavigationDestination* destination = findDestination(nodeId);
    return destination ? destination->label : nodeId;
}

std::string DestinationCatalog::sceneIdFor(const std::string& nodeId) const {
    const NavigationDestination* destination = findDestination(nodeId);
    if (!destination) return toLower(nodeId);
    return destination->sceneId.empty() ? destination->nodeId : destination->sceneId;
}

Vector2 DestinationCatalog::worldPointFor(const std::string& nodeId) const {
    const NavigationDestination* destination = findDestination(nodeId);
    return destination ? destination->worldPos : Vector2{0.0f, 0.0f};
}

bool DestinationCatalog::isPoiDestination(const std::string& nodeId) const {
    const NavigationDestination* destination = findDestination(nodeId);
    return destination ? destination->isPoi : false;
}

std::vector<std::string> DestinationCatalog::preferredReferenceWaypoints(const CampusGraph& graph) const {
    std::vector<std::string> references;
    for (const auto& destination : destinations_) {
        if (!graph.hasNode(destination.nodeId)) continue;

        const std::string loweredLabel = toLower(destination.label);
        const bool isReference = loweredLabel.find("bibli") != std::string::npos ||
                                 loweredLabel.find("soda") != std::string::npos ||
                                 loweredLabel.find("cafeter") != std::string::npos ||
                                 loweredLabel.find("comedor") != std::string::npos ||
                                 loweredLabel.find("entrada") != std::string::npos ||
                                 loweredLabel.find("parada") != std::string::npos;
        if (isReference) {
            references.push_back(destination.nodeId);
        }
    }
    return references;
}
