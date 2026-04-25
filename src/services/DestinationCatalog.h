#pragma once

#include "../core/graph/CampusGraph.h"
#include "../core/runtime/SceneRuntimeTypes.h"

#include <raylib.h>

#include <string>
#include <unordered_map>
#include <vector>

struct NavigationDestination {
    std::string nodeId;
    std::string label;
    std::string sceneId;
    std::string type;
    std::string scenarioTag;
    Vector2 worldPos{0.0f, 0.0f};
    bool isPoi{false};
    std::vector<Rectangle> zoneRects;
};

struct PoiEdgeMetadata {
    std::string edgeId;
    std::string fromNodeId;
    std::string toNodeId;
    std::string sceneId;
    std::string label;
    std::string scenarioTag;
    Vector2 worldPos{0.0f, 0.0f};
    std::vector<Rectangle> collisionRects;
};

class DestinationCatalog {
public:
    bool loadFromGeneratedJson(const std::string& generatedGraphPath,
                               const std::unordered_map<std::string, SceneData>& sceneDataMap);

    const std::vector<NavigationDestination>& destinations() const { return destinations_; }
    const std::vector<PoiEdgeMetadata>& poiEdges() const { return poiEdges_; }

    bool hasDestination(const std::string& nodeId) const;
    const NavigationDestination* findDestination(const std::string& nodeId) const;
    const PoiEdgeMetadata* findPoiEdge(const std::string& sceneNodeId, const std::string& poiNodeId) const;
    std::vector<PoiEdgeMetadata> poiEdgesForScene(const std::string& sceneId) const;

    std::string displayLabel(const std::string& nodeId) const;
    std::string sceneIdFor(const std::string& nodeId) const;
    Vector2 worldPointFor(const std::string& nodeId) const;
    bool isPoiDestination(const std::string& nodeId) const;
    std::vector<std::string> preferredReferenceWaypoints(const CampusGraph& graph) const;

private:
    void clear();

    std::vector<NavigationDestination> destinations_;
    std::vector<PoiEdgeMetadata> poiEdges_;
    std::unordered_map<std::string, size_t> destinationIndexByNodeId_;
    std::unordered_map<std::string, size_t> poiEdgeIndexByKey_;
};
