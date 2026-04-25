#include "NavigationController.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cmath>

using json = nlohmann::json;

NavigationController::NavigationController(
    CampusGraph& graph,
    NavigationService& navService,
    ScenarioManager& scenarioManager,
    ResilienceService& resilienceService)
    : graph_(graph)
    , navService_(navService)
    , scenarioManager_(scenarioManager)
    , resilienceService_(resilienceService) {
}

void NavigationController::loadDestinationsFromJson(const std::string& jsonPath) {
    destinations_.clear();
    
    std::ifstream file(jsonPath);
    if (!file.is_open()) {
        return;
    }
    
    try {
        json data;
        file >> data;
        
        if (data.contains("nodes") && data["nodes"].is_array()) {
            for (const auto& node : data["nodes"]) {
                DestinationInfo info;
                info.id = node.value("id", "");
                info.name = node.value("name", "");
                info.type = node.value("type", "");
                info.position.x = static_cast<float>(node.value("x", 0.0));
                info.position.y = static_cast<float>(node.value("y", 0.0));
                
                if (!info.id.empty() && graph_.hasNode(info.id)) {
                    destinations_.push_back(info);
                }
            }
        }
    } catch (...) {
        // Error parsing JSON, keep empty destinations
    }
}

void NavigationController::setDestination(const std::string& destinationId) {
    currentDestination_ = destinationId;
    routeActive_ = !destinationId.empty();
}

PathResult NavigationController::calculateProfiledPath(const std::string& origin, const std::string& destination) {
    // Usar ScenarioManager para aplicar perfil de estudiante
    PathResult result = scenarioManager_.buildProfiledPath(graph_, origin, destination);
    
    if (result.found) {
        routeScenePlan_ = result.path;
        routeActive_ = true;
    } else {
        routeScenePlan_.clear();
        routeActive_ = false;
    }
    
    return result;
}

bool NavigationController::hasReachedDestination(const Vector2& playerPos, float threshold) const {
    if (currentDestination_.empty() || !graph_.hasNode(currentDestination_)) {
        return false;
    }
    
    const Node& destNode = graph_.getNode(currentDestination_);
    const Vector2 destPos{static_cast<float>(destNode.x), static_cast<float>(destNode.y)};
    
    const float dx = playerPos.x - destPos.x;
    const float dy = playerPos.y - destPos.y;
    const float distance = std::sqrt(dx * dx + dy * dy);
    
    return distance <= threshold;
}

void NavigationController::clearActiveRoute() {
    currentDestination_.clear();
    routeScenePlan_.clear();
    routeActive_ = false;
}

void NavigationController::simulateBlockage(const std::string& elementId, bool isNode) {
    if (isNode) {
        resilienceService_.blockNode(elementId);
    } else {
        // Para aristas, necesitamos from/to - se maneja en UI
        // Esta función es un placeholder para lógica más compleja
    }
}

PathResult NavigationController::recalculateAfterBlockage(const std::string& origin, const std::string& destination) {
    // Recalcular ruta con la topología actualizada (bloqueos activos)
    PathResult result = scenarioManager_.buildProfiledPath(graph_, origin, destination);
    
    if (result.found) {
        routeScenePlan_ = result.path;
    } else {
        routeScenePlan_.clear();
        routeActive_ = false;
    }
    
    return result;
}
