#pragma once

#include "core/runtime/SceneRuntimeTypes.h"
#include "services/DestinationCatalog.h"
#include "services/TmjLoader.h"
#include "services/TransitionService.h"

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

struct SceneBootstrap {
    std::vector<SceneConfig> allScenes;
    std::unordered_map<std::string, SceneConfig> sceneMap;
    std::unordered_map<std::string, std::string> sceneToTmjPath;
    std::unordered_map<std::string, SceneData> sceneDataMap;
    std::unordered_map<std::string, std::unordered_map<std::string, Vector2>> allSceneSpawns;
    std::unordered_map<std::string, std::vector<TmjFloorTriggerDef>> allFloorTriggers;
    std::vector<std::pair<std::string, std::string>> floorScenes;
    std::string interestZonesPath;
    std::string generatedGraphRuntimePath;

    static SceneBootstrap load(const char* executablePath, const std::string& campusJsonPath);

    void buildPortalSceneLinks(TransitionService& transitions, std::vector<SceneLink>& sceneLinks) const;
    void buildRouteScenes(const DestinationCatalog& destinationCatalog,
                          std::vector<std::pair<std::string, std::string>>& routeScenes) const;

    std::string resolveSceneName(const std::string& sceneId) const;
    std::string sceneDisplayName(const std::string& sceneName,
                                 const DestinationCatalog& destinationCatalog) const;
    Vector2 sceneTargetPoint(const std::string& sceneName) const;
};
