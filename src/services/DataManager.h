#pragma once

#include "../core/graph/CampusGraph.h"
#include <raylib.h>
#include <string>
#include <unordered_map>

class DataManager {
public:
    CampusGraph loadCampusGraph(
        const std::string& configPath,
        const std::unordered_map<std::string, std::string>& sceneToTmjPath,
        const std::string& interestZonesPath = "",
        double pixelsToMeters = 0.10) const;

    void exportResolvedGraph(const CampusGraph& graph, const std::string& outputPath) const;

private:
    std::unordered_map<std::string, std::unordered_map<std::string, Vector2>>
    loadSpawnCache(const std::unordered_map<std::string, std::string>& sceneToTmjPath) const;
};
