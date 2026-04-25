#pragma once
#include <raylib.h>
#include "rlImGui.h"
#include "imgui.h"
#include <string>
#include <vector>
#include <utility>
#include <functional>

#include "../services/NavigationService.h"
#include "../services/ScenarioManager.h"
#include "../services/ComplexityAnalyzer.h"
#include "../services/ResilienceService.h"
#include "../ui/TabManager.h"

/**
 * UIManager - Gestión de paneles ImGui y renderizado de overlays
 * Responsabilidad: Manejar toda la UI (menús, paneles, minimap, overlays)
 * Sin estado de juego, solo renderiza basado en datos proporcionados
 */
class UIManager {
public:
    UIManager() = default;
    
    /**
     * Inicializa rlImGui (debe llamarse después de InitWindow)
     */
    void initialize();
    
    /**
     * Limpia recursos de UI
     */
    void shutdown();
    
    /**
     * Renderiza el overlay de navegación estilo Raylib (esquina inferior derecha)
     */
    void drawNavigationOverlayMenu(
        bool showNavigationGraph,
        bool infoMenuOpen,
        const std::string& currentSceneName,
        bool mobilityReduced,
        StudentType studentType,
        bool routeActive,
        float routeProgressPct,
        bool resilienceConnected,
        const TabManagerState& state,
        const std::vector<std::string>& blockedNodes);
    
    /**
     * Renderiza el menú de información completo (tecla M)
     */
    void drawInfoMenu(
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
        bool resilienceConnected);
    
    /**
     * Renderiza el overlay académico con grafo visual (ImGui)
     */
    void renderAcademicRuntimeOverlay(
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
        const std::vector<std::string>& routeScenePlan);
    
    /**
     * Begin/End frame de rlImGui
     */
    void beginFrame() { rlImGuiBegin(); }
    void endFrame() { rlImGuiEnd(); }
};
