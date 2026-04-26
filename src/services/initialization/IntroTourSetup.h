#pragma once

#include "core/runtime/SceneRuntimeTypes.h"
#include "services/IntroTourConfigLoader.h"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

struct IntroTourResources {
    IntroTourConfig config;
    std::vector<std::string> sceneOrder;
    std::vector<std::vector<Vector2>> recordedPathsByIndex;
    float secondsPerScene{10.0f};
    float transitionDuration{2.0f};
    float cameraZoom{2.2f};
    float cameraFollowLerp{4.0f};
    float speedScale{0.45f};
    float followScale{0.55f};
    std::string gameplayInitialSceneName{"Paradadebus"};
    std::string previewInitialSceneName{"Paradadebus"};

    static IntroTourResources load(
        const char* executablePath,
        const std::unordered_map<std::string, std::unordered_map<std::string, Vector2>>& allSceneSpawns);

    std::vector<Vector2> buildSceneTourPath(
        size_t sceneOrderIndex,
        const std::string& sceneName,
        const std::string& nextScene,
        const MapRenderData& mapData,
        const std::unordered_map<std::string, std::unordered_map<std::string, Vector2>>& allSceneSpawns,
        const std::function<std::string(const std::string&)>& resolveSceneName) const;

    static float scenePathLength(const std::vector<Vector2>& points);
    static void clampIntroCamera(Camera2D& camera,
                                 const MapRenderData& mapData,
                                 int screenWidth,
                                 int screenHeight);
};
