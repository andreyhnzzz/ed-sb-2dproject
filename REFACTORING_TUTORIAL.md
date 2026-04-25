# Tutorial de Refactorización: De main.cpp Monolítico a Arquitectura por Managers

## 📋 Resumen Ejecutivo

Este tutorial te guiará paso a paso para transformar un archivo `main.cpp` de ~1900 líneas en una arquitectura modular basada en **managers especializados**, reduciendo `main.cpp` a menos de 200 líneas de lógica ejecutable.

**Objetivo exclusivo:** Delegar responsabilidades sin cambiar comportamiento runtime ni agregar funcionalidades.

---

## 🎯 Principios Fundamentales

### Reglas de Oro
1. **NO modificar lógica de negocio existente** - Solo mover código
2. **NO cambiar firmas de métodos en servicios** - Solo consumirlos
3. **NO agregar nuevas features** - Mantener comportamiento idéntico
4. **Cada manager = Una única responsabilidad clara**
5. **Los managers no se conocen entre sí** - Se comunican vía structs de datos

### Arquitectura Objetivo

```
┌─────────────────────────────────────────────────────────────┐
│                        main.cpp                             │
│                    (~200 líneas ejecutables)                │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Instancia todos los managers y servicios            │   │
│  │  Configura ventana, cámara, sistemas base            │   │
│  │  Ejecuta loop: poll input → update → render          │   │
│  │  Gestiona cierre limpio de recursos                  │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        ▼                     ▼                     ▼
┌───────────────┐   ┌─────────────────┐   ┌───────────────┐
│ InputManager  │   │ GameController  │   │  UIManager    │
│               │   │                 │   │               │
│ - poll()      │   │ - update()      │   │ - render()    │
│ - WASD, Shift │   │ - jugador       │   │ - ImGui       │
│ - TAB, M      │   │ - cámara        │   │ - menús       │
│ - wheel, click│   │ - colisiones    │   │ - overlays    │
└───────────────┘   └─────────────────┘   └───────────────┘
        │                     │                     │
        └─────────────────────┼─────────────────────┘
                              ▼
                    ┌─────────────────┐
                    │ RenderContext   │
                    │ (datos readonly)│
                    └─────────────────┘
```

---

## 📦 Paso 1: Definir las Estructuras de Comunicación

### 1.1 Crear `InputState.h`

**Ubicación:** `src/runtime/InputState.h`

```cpp
#pragma once

struct InputState {
    // Navegación
    float moveX{0.0f};
    float moveY{0.0f};
    bool sprinting{false};
    int facingDirection{0};  // 0=down, 1=left, 2=right, 3=up
    
    // Acciones UI
    bool toggleInfoMenu{false};
    bool toggleInterestZones{false};
    bool confirmAction{false};  // Tecla E
    
    // Zoom
    float zoomWheelDelta{0.0f};
    
    // Estado del sistema
    bool uiActive{false};  // Si hay UI interactiva abierta
};
```

### 1.2 Crear `RenderContext.h`

**Ubicación:** `src/runtime/RenderContext.h`

```cpp
#pragma once
#include <raylib.h>
#include <string>
#include <vector>
#include "core/runtime/SceneRuntimeTypes.h"

struct RenderContext {
    // Escena actual
    const std::string& currentSceneName;
    const MapRenderData& mapData;
    
    // Jugador
    const Vector2& playerPos;
    const SpriteAnim& playerAnim;
    
    // Cámara
    const Camera2D& camera;
    
    // Estado de UI
    bool showHitboxes;
    bool showTriggers;
    bool showInterestZones;
    bool showNavigationGraph;
    bool infoMenuOpen;
    
    // Rutas activas
    const std::vector<Vector2>* routePathPoints;
    const std::vector<std::string>* routeScenePlan;
    bool routeActive;
    
    // Dimensiones de pantalla
    int screenWidth;
    int screenHeight;
};
```

---

## 📦 Paso 2: Crear InputManager

### 2.1 Header: `InputManager.h`

**Ubicación:** `src/runtime/InputManager.h`

```cpp
#pragma once
#include "InputState.h"

class InputManager {
public:
    InputState poll(bool uiActive) const;
    
private:
    void processKeyboard(InputState& state, bool uiActive) const;
    void processMouse(InputState& state, bool uiActive) const;
    int calculateFacingDirection(float moveX, float moveY) const;
};
```

### 2.2 Implementación: `InputManager.cpp`

**Ubicación:** `src/runtime/InputManager.cpp`

```cpp
#include "InputManager.h"
#include <raylib.h>

InputState InputManager::poll(bool uiActive) const {
    InputState state;
    state.uiActive = uiActive;
    
    processKeyboard(state, uiActive);
    processMouse(state, uiActive);
    
    if (state.moveX != 0.0f || state.moveY != 0.0f) {
        state.facingDirection = calculateFacingDirection(state.moveX, state.moveY);
    }
    
    return state;
}

void InputManager::processKeyboard(InputState& state, bool uiActive) const {
    // Toggle menú información (tecla M)
    if (IsKeyPressed(KEY_M)) {
        state.toggleInfoMenu = true;
    }
    
    // Toggle zonas de interés (tecla TAB)
    if (IsKeyPressed(KEY_TAB)) {
        state.toggleInterestZones = true;
    }
    
    // Si la UI está activa, no procesamos movimiento
    if (uiActive) return;
    
    // Movimiento WASD
    float x = 0.0f;
    float y = 0.0f;
    
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    y -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  y += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  x -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) x += 1.0f;
    
    // Normalizar vector diagonal
    if (x != 0.0f && y != 0.0f) {
        const float length = std::sqrt(x * x + y * y);
        x /= length;
        y /= length;
    }
    
    state.moveX = x;
    state.moveY = y;
    
    // Sprint (Shift)
    state.sprinting = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    
    // Confirmar acción (tecla E)
    if (IsKeyPressed(KEY_E)) {
        state.confirmAction = true;
    }
}

void InputManager::processMouse(InputState& state, bool uiActive) const {
    if (uiActive) return;
    
    // Zoom con rueda del mouse
    const float wheel = GetMouseWheelMove();
    state.zoomWheelDelta = wheel;
}

int InputManager::calculateFacingDirection(float moveX, float moveY) const {
    // Prioridad: vertical > horizontal
    if (std::abs(moveY) > std::abs(moveX)) {
        return moveY > 0.0f ? 0 : 3;  // down o up
    }
    if (moveX != 0.0f) {
        return moveX > 0.0f ? 2 : 1;  // right o left
    }
    return 0;  // default: down
}
```

---

## 📦 Paso 3: Crear GameController

### 3.1 Header: `GameController.h`

**Ubacidad:** `src/runtime/GameController.h`

```cpp
#pragma once
#include <raylib.h>
#include <string>
#include "InputState.h"
#include "RenderContext.h"
#include "core/runtime/SceneRuntimeTypes.h"
#include "services/WalkablePathService.h"

struct PlayerState {
    Vector2 position{0.0f, 0.0f};
    int direction{0};
    bool isMoving{false};
    bool isSprinting{false};
};

class GameController {
public:
    struct Config {
        float baseSpeed{150.0f};
        float sprintMultiplier{1.8f};
        float renderScale{1.6f};
        float minZoom{1.2f};
        float maxZoom{4.0f};
    };
    
    explicit GameController(Config config = {});
    
    void update(float dt, const InputState& input, MapRenderData& mapData);
    void applyZoom(float wheelDelta);
    void clampCameraToMap(const MapRenderData& mapData, int screenWidth, int screenHeight);
    
    // Getters
    PlayerState getPlayerState() const;
    const Camera2D& getCamera() const;
    const SpriteAnim& getPlayerAnim() const;
    Vector2 getPlayerPos() const;
    
    // Setters para inicialización
    void setPlayerPosition(const Vector2& pos);
    void setPlayerAnim(const SpriteAnim& anim);
    void loadPlayerSprites(const std::string& idlePath, const std::string& walkPath);
    
private:
    Config config_;
    Vector2 playerPos_{0.0f, 0.0f};
    Camera2D camera_{};
    SpriteAnim playerAnim_{};
    bool isMoving_{false};
};
```

### 3.2 Implementación: `GameController.cpp`

**Ubicación:** `src/runtime/GameController.cpp`

```cpp
#include "GameController.h"
#include <algorithm>
#include <cmath>

GameController::GameController(Config config) : config_(config) {
    // Configuración inicial de cámara
    camera_.offset = {0.0f, 0.0f};  // Se ajusta en init
    camera_.target = playerPos_;
    camera_.rotation = 0.0f;
    camera_.zoom = 2.2f;
}

void GameController::update(float dt, const InputState& input, MapRenderData& mapData) {
    // Actualizar dirección del sprite
    playerAnim_.direction = input.facingDirection;
    
    // Calcular velocidad actual
    const float currentSpeed = config_.baseSpeed * (input.sprinting ? config_.sprintMultiplier : 1.0f);
    
    // Movimiento en X
    Vector2 candidate = playerPos_;
    candidate.x += input.moveX * currentSpeed * dt;
    
    if (mapData.hasTexture) {
        candidate.x = std::clamp(candidate.x, 8.0f, 
                                  static_cast<float>(mapData.texture.width) - 8.0f);
    }
    
    if (!WalkablePathService::intersectsAny(
            WalkablePathService::playerColliderAt(candidate), mapData.hitboxes)) {
        playerPos_.x = candidate.x;
    }
    
    // Movimiento en Y
    candidate = playerPos_;
    candidate.y += input.moveY * currentSpeed * dt;
    
    if (mapData.hasTexture) {
        candidate.y = std::clamp(candidate.y, 14.0f, 
                                  static_cast<float>(mapData.texture.height));
    }
    
    if (!WalkablePathService::intersectsAny(
            WalkablePathService::playerColliderAt(candidate), mapData.hitboxes)) {
        playerPos_.y = candidate.y;
    }
    
    // Actualizar estado de movimiento
    isMoving_ = (input.moveX != 0.0f || input.moveY != 0.0f);
    
    // Actualizar animación
    if (isMoving_) {
        playerAnim_.timer += dt;
        const float frameStep = input.sprinting ? (1.0f / 16.0f) : (1.0f / 12.0f);
        if (playerAnim_.timer >= frameStep) {
            playerAnim_.timer = 0.0f;
            const int activeFrames = playerAnim_.hasWalk ? playerAnim_.walkFrames : playerAnim_.idleFrames;
            playerAnim_.frame = (playerAnim_.frame + 1) % 
                                SpriteAnimationService::directionalFrameCount(activeFrames);
        }
    } else {
        playerAnim_.timer = 0.0f;
        playerAnim_.frame = 0;
    }
    
    // Actualizar target de cámara
    camera_.target = playerPos_;
}

void GameController::applyZoom(float wheelDelta) {
    if (wheelDelta != 0.0f) {
        camera_.zoom = std::clamp(camera_.zoom + wheelDelta * 0.15f, 
                                   config_.minZoom, config_.maxZoom);
    }
}

void GameController::clampCameraToMap(const MapRenderData& mapData, 
                                       int screenWidth, int screenHeight) {
    if (!mapData.hasTexture) return;
    
    const float halfViewWidth = (static_cast<float>(screenWidth) * 0.5f) / camera_.zoom;
    const float halfViewHeight = (static_cast<float>(screenHeight) * 0.5f) / camera_.zoom;
    const float minX = halfViewWidth;
    const float maxX = static_cast<float>(mapData.texture.width) - halfViewWidth;
    const float minY = halfViewHeight;
    const float maxY = static_cast<float>(mapData.texture.height) - halfViewHeight;
    
    if (minX > maxX) {
        camera_.target.x = static_cast<float>(mapData.texture.width) * 0.5f;
    } else {
        camera_.target.x = std::clamp(camera_.target.x, minX, maxX);
    }
    
    if (minY > maxY) {
        camera_.target.y = static_cast<float>(mapData.texture.height) * 0.5f;
    } else {
        camera_.target.y = std::clamp(camera_.target.y, minY, maxY);
    }
}

// Getters
PlayerState GameController::getPlayerState() const {
    return {playerPos_, playerAnim_.direction, isMoving_, false};
}

const Camera2D& GameController::getCamera() const {
    return camera_;
}

const SpriteAnim& GameController::getPlayerAnim() const {
    return playerAnim_;
}

Vector2 GameController::getPlayerPos() const {
    return playerPos_;
}

// Setters
void GameController::setPlayerPosition(const Vector2& pos) {
    playerPos_ = pos;
    camera_.target = pos;
}

void GameController::setPlayerAnim(const SpriteAnim& anim) {
    playerAnim_ = anim;
}

void GameController::loadPlayerSprites(const std::string& idlePath, 
                                        const std::string& walkPath) {
    if (!idlePath.empty()) {
        playerAnim_.idle = LoadTexture(idlePath.c_str());
        playerAnim_.hasIdle = playerAnim_.idle.id != 0;
        
        if (playerAnim_.hasIdle) {
            playerAnim_.frameHeight = playerAnim_.idle.height;
            playerAnim_.frameWidth = 16;
            if (playerAnim_.idle.width % playerAnim_.frameWidth != 0) {
                playerAnim_.frameWidth = std::max(1, playerAnim_.idle.height);
            }
            playerAnim_.idleFrames = std::max(1, playerAnim_.idle.width / playerAnim_.frameWidth);
        }
    }
    
    if (!walkPath.empty()) {
        playerAnim_.walk = LoadTexture(walkPath.c_str());
        playerAnim_.hasWalk = playerAnim_.walk.id != 0;
        
        if (playerAnim_.hasWalk) {
            if (playerAnim_.walk.width % playerAnim_.frameWidth != 0) {
                playerAnim_.frameWidth = std::max(1, playerAnim_.walk.height);
            }
            playerAnim_.walkFrames = std::max(1, playerAnim_.walk.width / 
                                               std::max(1, playerAnim_.frameWidth));
        }
    }
}
```

---

## 📦 Paso 4: Crear UIManager

### 4.1 Header: `UIManager.h`

**Ubicación:** `src/runtime/UIManager.h`

```cpp
#pragma once
#include <raylib.h>
#include <string>
#include <vector>
#include <functional>
#include "InputState.h"
#include "RenderContext.h"
#include "ui/TabManager.h"
#include "services/ScenarioManager.h"
#include "services/ResilienceService.h"
#include "services/DestinationCatalog.h"

class UIManager {
public:
    struct State {
        bool showNavigationGraph{false};
        bool showHitboxes{false};
        bool showTriggers{false};
        bool showInterestZones{true};
        bool infoMenuOpen{false};
        
        // Estado del menú Raylib
        int selectedRouteSceneIdx{0};
        int rubricViewMode{0};
        int graphViewPage{0};
        int dfsViewPage{0};
        int bfsViewPage{0};
        int selectedBlockedNodeIdx{0};
        int selectedBlockedEdgeIdx{0};
        
        // Traversal views
        TraversalResult dfsTraversalView;
        TraversalResult bfsTraversalView;
    };
    
    void handleInput(const InputState& input, State& state);
    void render(const RenderContext& ctx, State& state,
                const CampusGraph& graph,
                ScenarioManager& scenarioManager,
                ResilienceService& resilienceService,
                const DestinationCatalog& destinationCatalog,
                TabManagerState& tabState,
                const std::vector<SceneLink>& sceneLinks,
                const std::vector<std::pair<std::string, std::string>>& routeScenes,
                const std::function<std::string(const std::string&)>& sceneDisplayName);
    
    void refreshTraversalViews(State& state,
                               const std::string& currentSceneId,
                               NavigationService& navService,
                               bool mobilityReduced);
    
private:
    void renderInfoMenu(const RenderContext& ctx, State& state, /*... parámetros ...*/);
    void renderNavigationOverlay(const RenderContext& ctx, State& state, /*...*/);
    void renderMinimap(const RenderContext& ctx, const SpriteAnim& playerAnim);
    void renderFadeOverlay(float alpha, int screenWidth, int screenHeight);
    void renderEPrompt(const std::string& hint, int screenWidth, int screenHeight);
    void renderCoordinateDisplay(const Vector2& playerPos, int screenWidth, int screenHeight);
};
```

### 4.2 Implementación: `UIManager.cpp` (Esqueleto)

**Ubicación:** `src/runtime/UIManager.cpp`

```cpp
#include "UIManager.h"
#include "rlImGui.h"
#include "imgui.h"

// Extraer TODAS las funciones de renderizado UI desde main.cpp:
// - drawRaylibNavigationOverlayMenu
// - drawRaylibInfoMenu  
// - drawRayButton
// - renderAcademicRuntimeOverlay
// - drawCurrentSceneNavigationOverlay
// - Funciones de minimap
// - Funciones de fade overlay
// - Funciones de "Presiona E" prompt
// - Funciones de coordinate display

void UIManager::handleInput(const InputState& input, State& state) {
    if (input.toggleInfoMenu) {
        state.infoMenuOpen = !state.infoMenuOpen;
    }
    if (input.toggleInterestZones) {
        state.showInterestZones = !state.showInterestZones;
    }
}

void UIManager::render(const RenderContext& ctx, State& state,
                       const CampusGraph& graph,
                       ScenarioManager& scenarioManager,
                       ResilienceService& resilienceService,
                       const DestinationCatalog& destinationCatalog,
                       TabManagerState& tabState,
                       const std::vector<SceneLink>& sceneLinks,
                       const std::vector<std::pair<std::string, std::string>>& routeScenes,
                       const std::function<std::string(const std::string&)>& sceneDisplayName) {
    
    // 1. Renderizar overlay de navegación (si está activo)
    if (state.showNavigationGraph && ctx.mapData.hasTexture) {
        // Llamar a drawCurrentSceneNavigationOverlay con los datos del contexto
    }
    
    // 2. Renderizar menú de información (Raylib)
    if (state.infoMenuOpen) {
        renderInfoMenu(ctx, state, graph, scenarioManager, resilienceService,
                       destinationCatalog, tabState, sceneLinks, routeScenes, 
                       sceneDisplayName);
    }
    
    // 3. Renderizar minimap (si no hay menú abierto)
    if (!state.infoMenuOpen && ctx.mapData.hasTexture) {
        renderMinimap(ctx, ctx.playerAnim);
    }
    
    // 4. Renderizar overlay de transición (fade)
    // renderFadeOverlay(...);
    
    // 5. Renderizar prompt "Presiona E"
    // renderEPrompt(...);
    
    // 6. Renderizar coordenadas
    renderCoordinateDisplay(ctx.playerPos, ctx.screenWidth, ctx.screenHeight);
    
    // 7. Renderizar ImGui (solo floor selector)
    if (!state.infoMenuOpen) {
        rlImGuiBegin();
        // transitions.drawFloorMenu();
        rlImGuiEnd();
    }
}

// ... Implementar cada método privado extrayendo código de main.cpp ...
```

---

## 📦 Paso 5: Crear SceneManager

### 5.1 Header: `SceneManager.h`

**Ubicación:** `src/runtime/SceneManager.h`

```cpp
#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "core/runtime/SceneRuntimeTypes.h"
#include "services/TransitionService.h"
#include "services/RuntimeBlockerService.h"

class SceneManager {
public:
    struct SceneState {
        std::string currentSceneName;
        MapRenderData mapData;
        bool isTransitioning{false};
        float transitionAlpha{0.0f};
    };
    
    void loadScene(const std::string& sceneName,
                   const std::unordered_map<std::string, SceneConfig>& sceneConfigs,
                   const std::unordered_map<std::string, SceneData>& sceneDataMap,
                   const RuntimeBlockerService& blockerService);
    
    void updateTransitions(float dt, TransitionService& transitions,
                          const Rectangle& playerCollider,
                          std::string& currentSceneName);
    
    bool needsSceneSwap() const;
    TransitionRequest getPendingSwap() const;
    void notifySwapDone();
    
    void refreshHitboxes(MapRenderData& mapData,
                         const SceneData& sceneData,
                         const std::vector<Rectangle>& runtimeBlockers);
    
    const SceneState& getState() const;
    const std::string& getCurrentSceneName() const;
    const MapRenderData& getMapData() const;
    
private:
    SceneState state_;
};
```

---

## 📦 Paso 6: Crear NavigationOrchestrator

### 6.1 Header: `NavigationOrchestrator.h`

**Ubicación:** `src/runtime/NavigationOrchestrator.h`

```cpp
#pragma once
#include <vector>
#include <string>
#include <functional>
#include "services/NavigationService.h"
#include "services/ResilienceService.h"
#include "services/ScenarioManager.h"
#include "services/ComplexityAnalyzer.h"
#include "ui/TabManager.h"

struct PathResultWrapper {
    PathResult result;
    std::string algorithm;
    long long elapsedUs{0};
    int nodesVisited{0};
};

class NavigationOrchestrator {
public:
    explicit NavigationOrchestrator(NavigationService& navService,
                                    ResilienceService& resilienceService,
                                    ScenarioManager& scenarioManager,
                                    ComplexityAnalyzer& complexityAnalyzer);
    
    // Ejecución de algoritmos
    TraversalResult runDFS(const std::string& startNode);
    TraversalResult runBFS(const std::string& startNode);
    bool checkConnectivity();
    
    // Búsqueda de caminos
    PathResult findPathDFS(const std::string& from, const std::string& to);
    PathResult findAlternatePath(const std::string& from, const std::string& to);
    
    // Gestión de bloqueos
    void blockEdge(const std::string& from, const std::string& to);
    void blockNode(const std::string& nodeId);
    void unblockAll();
    
    // Análisis de complejidad
    std::vector<ComplexityStats> analyzeComplexity(const std::string& startNode);
    AlgorithmComparison compareAlgorithms(const std::string& start, const std::string& end);
    
    // Ruta perfilada (con waypoints del ScenarioManager)
    PathResult runProfiledPath(const std::string& origin, const std::string& destination);
    
private:
    NavigationService& navService_;
    ResilienceService& resilienceService_;
    ScenarioManager& scenarioManager_;
    ComplexityAnalyzer& complexityAnalyzer_;
    
    PathResult mergeProfiledSegments(const std::vector<PathResult>& segments);
};
```

---

## 📦 Paso 7: Refactorizar main.cpp

### 7.1 Estructura Final de main.cpp

```cpp
// Includes
#include <raylib.h>
#include "rlImGui.h"
#include "imgui.h"
#include <filesystem>
#include <iostream>

// Servicios existentes (NO MODIFICAR)
#include "services/NavigationService.h"
#include "services/DataManager.h"
#include "services/ScenarioManager.h"
#include "services/ComplexityAnalyzer.h"
#include "services/ResilienceService.h"
#include "services/TransitionService.h"
#include "services/TmjLoader.h"
#include "services/StringUtils.h"
#include "services/InterestZoneLoader.h"
#include "services/AssetPathResolver.h"
#include "services/ScenePlanService.h"
#include "services/WalkablePathService.h"
#include "services/SpriteAnimationService.h"
#include "services/RuntimeTextService.h"
#include "services/MapRenderService.h"
#include "services/DestinationCatalog.h"
#include "services/RuntimeBlockerService.h"

// Managers nuevos
#include "runtime/InputManager.h"
#include "runtime/GameController.h"
#include "runtime/UIManager.h"
#include "runtime/SceneManager.h"
#include "runtime/NavigationOrchestrator.h"
#include "runtime/RenderContext.h"

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    // ================================================================
    // 1. INICIALIZACIÓN DE SERVICIOS BASE
    // ================================================================
    
    // Cargar configuración y grafo
    std::string path = AssetPathResolver::findCampusJson(argc > 0 ? argv[0] : nullptr);
    if (path.empty()) {
        std::cerr << "campus.json was not found.\n";
        return 1;
    }
    
    DataManager dataManager;
    CampusGraph graph = dataManager.loadGraph(path);
    
    // Cargar escena definitions
    const std::vector<SceneConfig> allScenes = {/* ... */};
    std::unordered_map<std::string, SceneConfig> sceneMap;
    for (const auto& sc : allScenes) sceneMap[sc.name] = sc;
    
    // Cargar SceneData (hitboxes, interest zones)
    std::unordered_map<std::string, SceneData> sceneDataMap;
    // ... carga de assets ...
    
    // Inicializar servicios
    NavigationService navService(graph);
    ScenarioManager scenarioManager;
    ComplexityAnalyzer complexityAnalyzer(graph);
    ResilienceService resilienceService(graph);
    TransitionService transitions;
    DestinationCatalog destinationCatalog;
    RuntimeBlockerService runtimeBlockerService;
    
    // Cargar catálogo de destinos
    destinationCatalog.loadFromGeneratedJson(/* ... */, sceneDataMap);
    scenarioManager.setReferenceWaypoints(destinationCatalog.preferredReferenceWaypoints(graph));
    
    // Cargar transiciones (portales, elevadores)
    // ... código existente de carga de portales ...
    
    // Construir lista de escenas para rutas
    std::vector<std::pair<std::string, std::string>> routeScenes;
    for (const auto& dest : destinationCatalog.destinations()) {
        routeScenes.push_back({dest.nodeId, dest.label});
    }
    
    // ================================================================
    // 2. CONFIGURACIÓN DE VENTANA Y SISTEMAS
    // ================================================================
    
    int screenWidth = 1280;
    int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "EcoCampusNav (Raylib)");
    const int monitor = GetCurrentMonitor();
    screenWidth = GetMonitorWidth(monitor);
    screenHeight = GetMonitorHeight(monitor);
    SetWindowSize(screenWidth, screenHeight);
    ToggleFullscreen();
    SetTargetFPS(60);
    
    rlImGuiSetup(true);
    
    // ================================================================
    // 3. INSTANCIAR MANAGERS
    // ================================================================
    
    InputManager inputManager;
    GameController gameController;
    UIManager uiManager;
    SceneManager sceneManager;
    NavigationOrchestrator navOrchestrator(navService, resilienceService, 
                                           scenarioManager, complexityAnalyzer);
    
    // Cargar escena inicial
    const std::string initialSceneName = "Paradadebus";
    sceneManager.loadScene(initialSceneName, sceneMap, sceneDataMap, runtimeBlockerService);
    
    // Cargar sprites del jugador
    const std::string idlePath = AssetPathResolver::findPlayerIdleSprite(argc > 0 ? argv[0] : nullptr);
    const std::string walkPath = AssetPathResolver::findPlayerWalkSprite(argc > 0 ? argv[0] : nullptr);
    gameController.loadPlayerSprites(idlePath, walkPath);
    
    // Posicionar jugador en spawn
    Vector2 spawnPos = /* obtener spawn de TMJ o hitbox scan */;
    gameController.setPlayerPosition(spawnPos);
    
    // Configurar cámara inicial
    Camera2D& camera = const_cast<Camera2D&>(gameController.getCamera());
    camera.offset = {screenWidth * 0.5f, screenHeight * 0.5f};
    
    // Estado de UI
    UIManager::State uiState;
    TabManagerState tabState = createTabManagerState(graph, /* generatedGraphPath */);
    
    // Helper functions (como lambdas o miembros de UIManager)
    auto sceneDisplayName = [&](const std::string& sceneName) -> std::string {
        // ... implementación existente ...
    };
    
    auto sceneTargetPoint = [&](const std::string& sceneName) -> Vector2 {
        // ... implementación existente ...
    };
    
    // ================================================================
    // 4. GAME LOOP PRINCIPAL
    // ================================================================
    
    while (!WindowShouldClose()) {
        const float dt = GetFrameTime();
        
        // 4.1 Poll Input
        const InputState input = inputManager.poll(uiState.infoMenuOpen);
        
        // 4.2 Handle UI Input
        uiManager.handleInput(input, uiState);
        
        // 4.3 Aplicar zoom (si no hay UI activa)
        if (!uiState.infoMenuOpen && input.zoomWheelDelta != 0.0f) {
            gameController.applyZoom(input.zoomWheelDelta);
        }
        
        // 4.4 Refresh hitboxes de escena actual
        const auto& currentSceneData = sceneDataMap.at(sceneManager.getCurrentSceneName());
        sceneManager.refreshHitboxes(/* mapData */, currentSceneData,
                                     runtimeBlockerService.collisionRectsForScene(
                                         sceneManager.getCurrentSceneName()));
        
        // 4.5 Update Juego (jugador, cámara, colisiones)
        gameController.update(dt, input, /* mapData */);
        gameController.clampCameraToMap(/* mapData */, screenWidth, screenHeight);
        
        // 4.6 Update Transiciones de escena
        sceneManager.updateTransitions(dt, transitions, 
                                       WalkablePathService::playerColliderAt(
                                           gameController.getPlayerPos()),
                                       /* currentSceneName */);
        
        // 4.7 Realizar swap de escena si es necesario
        if (sceneManager.needsSceneSwap()) {
            const TransitionRequest req = sceneManager.getPendingSwap();
            // Unload textura anterior
            // Load nueva textura
            // Actualizar hitboxes
            // Posicionar jugador en spawn
            gameController.setPlayerPosition(req.spawnPos);
            camera.zoom = 2.2f;
            gameController.clampCameraToMap(/* mapData */, screenWidth, screenHeight);
            sceneManager.notifySwapDone();
        }
        
        // 4.8 Refresh traversal views (si el menú está abierto)
        if (uiState.infoMenuOpen) {
            uiManager.refreshTraversalViews(uiState, 
                                           StringUtils::toLowerCopy(
                                               sceneManager.getCurrentSceneName()),
                                           navService,
                                           scenarioManager.isMobilityReduced());
        }
        
        // ============================================================
        // 5. RENDERIZADO
        // ============================================================
        
        BeginDrawing();
        ClearBackground({18, 20, 28, 255});
        
        BeginMode2D(camera);
        
        // 5.1 Renderizar mapa y hitboxes
        if (/* mapData.hasTexture */) {
            MapRenderService::drawMapWithHitboxes(/* mapData */, uiState.showHitboxes);
        }
        
        // 5.2 Renderizar zonas de interés
        if (uiState.showInterestZones) {
            MapRenderService::drawInterestZones(/* mapData.interestZones */);
        }
        
        // 5.3 Renderizar overlays de navegación
        if (uiState.showNavigationGraph) {
            // Dibujar overlay de navegación actual
            // Dibujar rutas activas (DFS, alternate, route)
        }
        
        // 5.4 Renderizar triggers (debug)
        if (uiState.showTriggers) {
            // Dibujar rectángulos de portales y elevadores
        }
        
        // 5.5 Renderizar jugador
        const SpriteAnim& playerAnim = gameController.getPlayerAnim();
        const Vector2 playerPos = gameController.getPlayerPos();
        // Código de DrawTexturePro existente...
        
        EndMode2D();
        
        // 5.6 Renderizar UI (screen space)
        RenderContext ctx{
            .currentSceneName = sceneManager.getCurrentSceneName(),
            .mapData = /* mapData */,
            .playerPos = playerPos,
            .playerAnim = playerAnim,
            .camera = camera,
            .showHitboxes = uiState.showHitboxes,
            .showTriggers = uiState.showTriggers,
            .showInterestZones = uiState.showInterestZones,
            .showNavigationGraph = uiState.showNavigationGraph,
            .infoMenuOpen = uiState.infoMenuOpen,
            .routePathPoints = /* &routeState.routePathPoints */,
            .routeScenePlan = /* &routeState.routeScenePlan */,
            .routeActive = /* routeState.routeActive */,
            .screenWidth = screenWidth,
            .screenHeight = screenHeight
        };
        
        uiManager.render(ctx, uiState, graph, scenarioManager, resilienceService,
                        destinationCatalog, tabState, sceneLinks, routeScenes,
                        sceneDisplayName);
        
        EndDrawing();
    }
    
    // ================================================================
    // 6. CLEANUP DE RECURSOS
    // ================================================================
    
    if (/* mapData.hasTexture */) {
        UnloadTexture(/* mapData.texture */);
    }
    if (playerAnim.hasIdle) UnloadTexture(playerAnim.idle);
    if (playerAnim.hasWalk) UnloadTexture(playerAnim.walk);
    
    rlImGuiShutdown();
    CloseWindow();
    
    return 0;
}
```

---

## ✅ Checklist de Verificación

### Después de cada paso, verifica:

- [ ] El proyecto compila sin errores
- [ ] No hay warnings nuevos
- [ ] El comportamiento es idéntico al original

### Al finalizar toda la refactorización:

- [ ] `main.cpp` tiene ≤200 líneas de lógica ejecutable
- [ ] Cada manager tiene ≤300 líneas
- [ ] No se modificaron archivos en `core/graph/`, `repositories/`, `services/`
- [ ] El movimiento del jugador es idéntico
- [ ] La cámara funciona igual
- [ ] Las transiciones de escena funcionan igual
- [ ] La UI se ve y comporta igual
- [ ] Los algoritmos DFS/BFS producen los mismos resultados
- [ ] El sistema de colisiones funciona igual
- [ ] Las animaciones del jugador son idénticas

---

## 🔧 Técnicas de Extracción de Código

### Para extraer funciones grandes:

1. **Identificar dependencias:** ¿Qué variables locales/global usa?
2. **Crear parámetros:** Convertir dependencias en parámetros de función
3. **Usar structs de contexto:** Si hay muchos parámetros, usar un struct
4. **Mantener firma compatible:** No cambiar tipos de retorno ni parámetros clave
5. **Testear incrementalmente:** Compilar y probar después de cada extracción

### Ejemplo de extracción:

```cpp
// ANTES (en main.cpp):
while (!WindowShouldClose()) {
    // 50 líneas de lógica de movimiento...
    Vector2 candidate = playerPos;
    candidate.x += moveX * currentSpeed * dt;
    // ... más código ...
}

// DESPUÉS (en GameController::update):
void GameController::update(float dt, const InputState& input, MapRenderData& mapData) {
    // Mismo código, ahora encapsulado
}
```

---

## 📝 Consejos Finales

1. **Trabaja incrementalmente:** Extrae un manager a la vez
2. **Usa el control de versiones:** Commit después de cada manager funcionando
3. **No optimices prematuramente:** Primero haz que funcione, luego mejora
4. **Mantén los nombres originales:** Usa los mismos nombres de variables cuando sea posible
5. **Documenta las dependencias:** Comenta qué servicios consume cada manager
6. **Prueba constantemente:** Ejecuta el juego después de cada cambio significativo

---

## 🚀 Siguiente Nivel (Opcional)

Una vez completada la refactorización básica:

- Agregar tests unitarios para cada manager
- Implementar patrón Observer para comunicación entre managers
- Extraer configuración a archivos JSON/YAML
- Agregar logging estructurado
- Implementar hot-reload de assets

---

**¡Éxito en tu refactorización!** 🎉
