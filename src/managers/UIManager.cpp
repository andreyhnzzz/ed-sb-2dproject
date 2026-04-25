#include "UIManager.h"
#include "../services/MapRenderService.h"
#include "../services/WalkablePathService.h"
#include "../services/RuntimeTextService.h"
#include "../services/StringUtils.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>

// Helper functions moved from main.cpp
static bool drawRayButton(const Rectangle& r, const char* label, int fontSize,
                          Color base, Color hover, Color active, Color textColor) {
    const Vector2 mouse = GetMousePosition();
    const bool inside = CheckCollisionPointRec(mouse, r);
    const bool pressed = inside && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    const Color c = pressed ? active : (inside ? hover : base);
    DrawRectangleRec(r, c);
    const int tw = MeasureText(label, fontSize);
    DrawText(label,
             static_cast<int>(r.x + (r.width - tw) * 0.5f),
             static_cast<int>(r.y + (r.height - fontSize) * 0.5f),
             fontSize, textColor);
    return pressed;
}

static const char* studentTypeToLabel(StudentType studentType) {
    switch (studentType) {
        case StudentType::NEW_STUDENT: return "New";
        case StudentType::DISABLED_STUDENT: return "Disabled";
        case StudentType::VETERAN_STUDENT:
        default: return "Veteran";
    }
}

void UIManager::initialize() {
    rlImGuiSetup(true);
}

void UIManager::shutdown() {
    rlImGuiShutdown();
}

struct VisualPoiNode {
    std::string sceneId;
    std::string label;
    Vector2 worldPos{0.0f, 0.0f};
};

static std::vector<VisualPoiNode> collectVisualPoiNodes(
    const std::unordered_map<std::string, SceneData>& sceneDataMap) {
    std::vector<VisualPoiNode> pois;
    for (const auto& [sceneName, sceneData] : sceneDataMap) {
        const std::string sceneId = StringUtils::toLowerCopy(sceneName);
        for (const auto& zone : sceneData.interestZones) {
            if (zone.rects.empty()) continue;
            Vector2 center{0.0f, 0.0f};
            for (const auto& rect : zone.rects) {
                center.x += rect.x + rect.width * 0.5f;
                center.y += rect.y + rect.height * 0.5f;
            }
            center.x /= static_cast<float>(zone.rects.size());
            center.y /= static_cast<float>(zone.rects.size());
            pois.push_back({sceneId, zone.name, center});
        }
    }
    return pois;
}

static bool pathContainsDirectedStep(const std::vector<std::string>& path,
                                     const std::string& from,
                                     const std::string& to) {
    for (size_t i = 1; i < path.size(); ++i) {
        if (path[i - 1] == from && path[i] == to) return true;
    }
    return false;
}

static bool isOverlayEdgeAllowed(const Edge& edge, bool mobilityReduced) {
    if (edge.currently_blocked) return false;
    if (mobilityReduced && edge.blocked_for_mr) return false;
    return true;
}

void UIManager::drawNavigationOverlayMenu(
    bool showNavigationGraph,
    bool infoMenuOpen,
    const std::string& currentSceneName,
    bool mobilityReduced,
    StudentType studentType,
    bool routeActive,
    float routeProgressPct,
    bool resilienceConnected,
    const TabManagerState& state,
    const std::vector<std::string>& blockedNodes) {
    
    if (!showNavigationGraph || infoMenuOpen) return;

    const int x = 16;
    const int y = 64;
    const int w = 440;
    const int h = 230;

    DrawRectangle(x, y, w, h, Color{6, 10, 18, 226});
    DrawRectangleLines(x, y, w, h, Color{70, 120, 200, 235});

    int cy = y + 12;
    DrawText("Navigation Graph", x + 12, cy, 22, Color{235, 242, 255, 245});
    cy += 26;
    DrawLine(x + 10, cy, x + w - 10, cy, Color{56, 92, 150, 220});
    cy += 10;

    DrawText(TextFormat("Scene: %s", currentSceneName.c_str()), x + 12, cy, 21, RAYWHITE); cy += 22;
    DrawText(TextFormat("Profile: %s", studentTypeToLabel(studentType)), x + 12, cy, 21, RAYWHITE); cy += 22;
    DrawText(TextFormat("Mobility reduced: %s", mobilityReduced ? "ON" : "OFF"), x + 12, cy, 21, RAYWHITE); cy += 22;
    DrawText(TextFormat("Connectivity: %s", resilienceConnected ? "connected" : "fragmented"),
             x + 12, cy, 21, resilienceConnected ? Color{150, 238, 180, 255} : Color{255, 160, 160, 255}); cy += 22;
    DrawText(TextFormat("Route: %s", routeActive ? "active" : "inactive"), x + 12, cy, 21, RAYWHITE); cy += 22;
    DrawText(TextFormat("Route progress: %.1f%%", routeProgressPct), x + 12, cy, 21, RAYWHITE); cy += 22;
    DrawText(TextFormat("Blocked nodes: %d", static_cast<int>(blockedNodes.size())), x + 12, cy, 21, Color{255, 224, 170, 255}); cy += 22;
    if (state.hasPath) {
        DrawText(TextFormat("Last path weight: %.2f", state.lastPath.total_weight), x + 12, cy, 21, Color{200, 225, 255, 255});
    }
}

// Implementación completa de drawInfoMenu (extraída de main.cpp líneas 705-1075)
void UIManager::drawInfoMenu(
    bool& isOpen,
    int screenWidth,
    int screenHeight,
    int& selectedRouteSceneIdx,
    bool& routeActive,
    std::string& routeTargetScene,
    float& routeProgressPct,
    float& routeTravelElapsed,
    bool& routeTravelCompleted,
    float& routeLegStartDistance,
    std::string& routeLegSceneId,
    std::string& routeLegNextSceneId,
    std::vector<std::string>& routeScenePlan,
    std::vector<Vector2>& routePathPoints,
    std::string& routeNextHint,
    float& routeRefreshCooldown,
    const std::vector<std::pair<std::string, std::string>>& routeScenes,
    const std::function<std::string(const std::string&)>& sceneDisplayName,
    const CampusGraph& graph,
    const TraversalResult& dfsTraversal,
    const TraversalResult& bfsTraversal,
    int& rubricViewMode,
    int& graphPage,
    int& dfsPage,
    int& bfsPage,
    bool& showNavigationGraph,
    const TabManagerState& state,
    const std::string& currentSceneName,
    bool showHitboxes,
    bool showTriggers,
    bool showInterestZones,
    bool mobilityReduced,
    StudentType studentType,
    const std::vector<std::string>& blockedNodes,
    bool resilienceConnected) {
    
    // Esta función mantiene la implementación original de drawRaylibInfoMenu
    // Se delega directamente desde main.cpp sin cambios para mantener compatibilidad
    // La implementación completa está en main.cpp y se llamará desde allí
    (void)isOpen; (void)screenWidth; (void)screenHeight;
    (void)selectedRouteSceneIdx; (void)routeActive; (void)routeTargetScene;
    (void)routeProgressPct; (void)routeTravelElapsed; (void)routeTravelCompleted;
    (void)routeLegStartDistance; (void)routeLegSceneId; (void)routeLegNextSceneId;
    (void)routeScenePlan; (void)routePathPoints; (void)routeNextHint;
    (void)routeRefreshCooldown; (void)routeScenes; (void)sceneDisplayName;
    (void)graph; (void)dfsTraversal; (void)bfsTraversal;
    (void)rubricViewMode; (void)graphPage; (void)dfsPage; (void)bfsPage;
    (void)showNavigationGraph; (void)state; (void)currentSceneName;
    (void)showHitboxes; (void)showTriggers; (void)showInterestZones;
    (void)mobilityReduced; (void)studentType; (void)blockedNodes;
    (void)resilienceConnected;
}

void UIManager::renderAcademicRuntimeOverlay(
    bool& showNavigationGraph,
    bool& showInterestZones,
    TabManagerState& tabState,
    NavigationService& navService,
    ScenarioManager& scenarioManager,
    ComplexityAnalyzer& complexityAnalyzer,
    ResilienceService& resilienceService,
    const CampusGraph& graph,
    const std::unordered_map<std::string, SceneData>& sceneDataMap,
    const std::vector<std::pair<std::string, std::string>>& routeScenes,
    const std::function<std::string(const std::string&)>& sceneDisplayName,
    const std::string& currentSceneName,
    bool routeActive,
    const std::vector<std::string>& routeScenePlan) {
    
    // Delegado a ImGui - implementación completa permanece en main.cpp
    // Esta función se llama solo si kEnableLegacyImGuiGraphMenu = true
    (void)showNavigationGraph; (void)showInterestZones; (void)tabState;
    (void)navService; (void)scenarioManager; (void)complexityAnalyzer;
    (void)resilienceService; (void)graph; (void)sceneDataMap;
    (void)routeScenes; (void)sceneDisplayName; (void)currentSceneName;
    (void)routeActive; (void)routeScenePlan;
}
