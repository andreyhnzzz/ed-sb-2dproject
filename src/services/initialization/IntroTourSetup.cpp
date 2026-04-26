#include "IntroTourSetup.h"

#include "services/AssetPathResolver.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {
std::string canonicalSceneId(std::string sceneName) {
    std::transform(sceneName.begin(), sceneName.end(), sceneName.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return sceneName;
}
}

IntroTourResources IntroTourResources::load(
    const char* executablePath,
    const std::unordered_map<std::string, std::unordered_map<std::string, Vector2>>& allSceneSpawns) {
    IntroTourResources out;
    const std::vector<std::string> defaultIntroSceneOrder = {
        "Paradadebus", "Exteriorcafeteria", "piso4", "piso1", "piso2", "piso3", "piso5",
        "piso4", "Exteriorcafeteria", "biblio", "Exteriorcafeteria", "Interiorcafeteria",
        "Exteriorcafeteria", "Paradadebus"
    };

    const std::string introTourConfigPath =
        AssetPathResolver::resolveAssetPath(executablePath, "assets/intro_tour_paths.json");
    const std::string introTourRecordingsPath =
        AssetPathResolver::resolveAssetPath(executablePath, "assets/intro_tour_recordings.json");
    out.config = loadIntroTourConfig(introTourConfigPath);
    out.sceneOrder =
        out.config.sceneOrder.size() > 1 ? out.config.sceneOrder : defaultIntroSceneOrder;
    out.secondsPerScene = std::clamp(out.config.secondsPerScene, 5.0f, 90.0f);
    out.transitionDuration = std::clamp(out.config.transitionSeconds, 0.8f, 8.0f);
    out.cameraZoom = std::clamp(out.config.cameraZoom, 1.2f, 4.0f);
    out.cameraFollowLerp = std::clamp(out.config.cameraFollowLerp, 0.8f, 10.0f);
    out.previewInitialSceneName =
        out.sceneOrder.empty() ? out.gameplayInitialSceneName : out.sceneOrder.front();
    out.recordedPathsByIndex.resize(out.sceneOrder.size());

    if (introTourRecordingsPath.empty()) {
        return out;
    }

    try {
        std::ifstream file(introTourRecordingsPath);
        if (!file.is_open()) return out;

        json root;
        file >> root;
        if (!root.contains("routes") || !root["routes"].is_array()) return out;

        std::vector<std::pair<std::string, std::vector<Vector2>>> loadedRoutes;
        for (const auto& route : root["routes"]) {
            if (!route.is_object()) continue;
            const std::string scene = route.value("scene", "");
            if (scene.empty()) continue;
            std::vector<Vector2> points;
            if (route.contains("points") && route["points"].is_array()) {
                for (const auto& point : route["points"]) {
                    if (!point.is_array() || point.size() != 2) continue;
                    if (!point[0].is_number() || !point[1].is_number()) continue;
                    points.push_back(Vector2{
                        std::clamp(point[0].get<float>(), 0.0f, 1.0f),
                        std::clamp(point[1].get<float>(), 0.0f, 1.0f)
                    });
                }
            }
            if (points.size() >= 2) {
                loadedRoutes.push_back({scene, std::move(points)});
            }
        }

        size_t routeCursor = 0;
        for (size_t i = 0; i < out.sceneOrder.size() && routeCursor < loadedRoutes.size(); ++i) {
            const std::string wantedScene = canonicalSceneId(out.sceneOrder[i]);
            size_t matched = loadedRoutes.size();
            for (size_t j = routeCursor; j < loadedRoutes.size(); ++j) {
                if (canonicalSceneId(loadedRoutes[j].first) == wantedScene) {
                    matched = j;
                    break;
                }
            }
            if (matched == loadedRoutes.size()) continue;
            out.recordedPathsByIndex[i] = loadedRoutes[matched].second;
            routeCursor = matched + 1;
        }
    } catch (const std::exception& ex) {
        std::cerr << "[IntroTour] Failed to parse intro_tour_recordings.json: "
                  << ex.what() << "\n";
    }

    return out;
}

std::vector<Vector2> IntroTourResources::buildSceneTourPath(
    size_t sceneOrderIndex,
    const std::string& sceneName,
    const std::string& nextScene,
    const MapRenderData& mapData,
    const std::unordered_map<std::string, std::unordered_map<std::string, Vector2>>& allSceneSpawns,
    const std::function<std::string(const std::string&)>& resolveSceneName) const {
    std::vector<Vector2> pathPoints;
    const std::string resolvedSceneName = resolveSceneName(sceneName);
    const bool hasTexture = mapData.hasTexture;
    const float w = hasTexture ? static_cast<float>(mapData.texture.width) : 1000.0f;
    const float h = hasTexture ? static_cast<float>(mapData.texture.height) : 700.0f;

    auto toWorldPath = [w, h](const std::vector<Vector2>& normalized) {
        std::vector<Vector2> world;
        world.reserve(normalized.size());
        for (const Vector2 p : normalized) {
            world.push_back(Vector2{
                std::clamp(p.x, 0.0f, 1.0f) * w,
                std::clamp(p.y, 0.0f, 1.0f) * h
            });
        }
        return world;
    };

    if (sceneOrderIndex < recordedPathsByIndex.size() &&
        recordedPathsByIndex[sceneOrderIndex].size() >= 2) {
        pathPoints = toWorldPath(recordedPathsByIndex[sceneOrderIndex]);
    }

    const std::string transitionKey = makeIntroTransitionKey(sceneName, nextScene);
    if (pathPoints.size() < 2) {
        if (const auto transitionIt = config.transitionPaths.find(transitionKey);
            transitionIt != config.transitionPaths.end()) {
            pathPoints = toWorldPath(transitionIt->second);
        }
    }

    if (pathPoints.size() < 2) {
        std::string sceneKey = canonicalSceneId(sceneName);
        if (const auto fallbackIt = config.sceneFallbackPaths.find(sceneKey);
            fallbackIt != config.sceneFallbackPaths.end()) {
            pathPoints = toWorldPath(fallbackIt->second);
        }
    }

    if (pathPoints.size() < 2) {
        const auto spawnIt = allSceneSpawns.find(resolvedSceneName);
        if (spawnIt != allSceneSpawns.end()) {
            std::vector<std::string> sortedKeys;
            sortedKeys.reserve(spawnIt->second.size());
            for (const auto& [spawnId, _] : spawnIt->second) {
                sortedKeys.push_back(spawnId);
            }
            std::sort(sortedKeys.begin(), sortedKeys.end());
            for (const std::string& spawnId : sortedKeys) {
                const auto posIt = spawnIt->second.find(spawnId);
                if (posIt != spawnIt->second.end()) pathPoints.push_back(posIt->second);
            }
        }
    }

    if (pathPoints.empty()) {
        pathPoints = {
            Vector2{w * 0.20f, h * 0.75f},
            Vector2{w * 0.42f, h * 0.58f},
            Vector2{w * 0.64f, h * 0.44f},
            Vector2{w * 0.82f, h * 0.30f}
        };
    }

    std::vector<Vector2> filtered;
    filtered.reserve(pathPoints.size());
    for (const Vector2& p : pathPoints) {
        if (filtered.empty()) {
            filtered.push_back(p);
            continue;
        }
        const float dx = p.x - filtered.back().x;
        const float dy = p.y - filtered.back().y;
        if (std::sqrt(dx * dx + dy * dy) >= 4.0f) {
            filtered.push_back(p);
        }
    }
    if (!filtered.empty()) {
        pathPoints = std::move(filtered);
    }

    return pathPoints;
}

float IntroTourResources::scenePathLength(const std::vector<Vector2>& points) {
    if (points.size() < 2) return 0.0f;
    float length = 0.0f;
    for (size_t i = 1; i < points.size(); ++i) {
        const float dx = points[i].x - points[i - 1].x;
        const float dy = points[i].y - points[i - 1].y;
        length += std::sqrt(dx * dx + dy * dy);
    }
    return length;
}

void IntroTourResources::clampIntroCamera(Camera2D& camera,
                                          const MapRenderData& mapData,
                                          int screenWidth,
                                          int screenHeight) {
    if (!mapData.hasTexture) return;

    const float halfViewWidth = (static_cast<float>(screenWidth) * 0.5f) / camera.zoom;
    const float halfViewHeight = (static_cast<float>(screenHeight) * 0.5f) / camera.zoom;
    const float minX = halfViewWidth;
    const float maxX = static_cast<float>(mapData.texture.width) - halfViewWidth;
    const float minY = halfViewHeight;
    const float maxY = static_cast<float>(mapData.texture.height) - halfViewHeight;

    if (minX > maxX) camera.target.x = static_cast<float>(mapData.texture.width) * 0.5f;
    else camera.target.x = std::clamp(camera.target.x, minX, maxX);

    if (minY > maxY) camera.target.y = static_cast<float>(mapData.texture.height) * 0.5f;
    else camera.target.y = std::clamp(camera.target.y, minY, maxY);
}
