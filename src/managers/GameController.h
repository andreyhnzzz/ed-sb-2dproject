#pragma once
#include <raylib.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

#include "../services/TransitionService.h"
#include "../services/MapRenderService.h"
#include "../services/SpriteAnimationService.h"
#include "../services/WalkablePathService.h"

/**
 * GameController - Ciclo de actualización, estado de escena, cámara, jugador y transiciones
 * Responsabilidad: Manejar el game loop, estado del jugador, cámara y transiciones entre escenas
 */
class GameController {
public:
    struct GameState {
        std::string currentSceneName;
        Vector2 playerPos{0.0f, 0.0f};
        float playerSpeed{150.0f};
        float sprintMultiplier{1.8f};
        float playerRenderScale{1.6f};
        Camera2D camera{};
        float minZoom{1.2f};
        float maxZoom{4.0f};
        bool showHitboxes{false};
        bool showTriggers{false};
        bool showInterestZones{true};
    };
    
    GameController() = default;
    
    /**
     * Inicializa el estado del juego con la escena inicial
     */
    void initialize(const std::string& initialSceneName,
                    const std::unordered_map<std::string, SceneConfig>& sceneMap,
                    const std::unordered_map<std::string, SceneData>& sceneDataMap,
                    const std::unordered_map<std::string, std::unordered_map<std::string, Vector2>>& allSceneSpawns);
    
    /**
     * Actualiza el estado del jugador basado en input
     */
    void updatePlayer(float dt, float moveX, float moveY, bool sprinting, bool infoMenuOpen);
    
    /**
     * Actualiza la cámara para seguir al jugador
     */
    void updateCamera(int screenWidth, int screenHeight);
    
    /**
     * Procesa transiciones entre escenas
     */
    void processTransitions(float dt);
    
    /**
     * Verifica si necesita hacer swap de escena
     */
    bool needsSceneSwap() const { return transitions_.needsSceneSwap(); }
    
    /**
     * Obtiene la solicitud de swap pendiente
     */
    TransitionRequest getPendingSwap() const { return transitions_.getPendingSwap(); }
    
    /**
     * Notifica que el swap de escena fue completado
     */
    void notifySwapDone() { transitions_.notifySwapDone(); }
    
    /**
     * Actualiza el mapa renderizado después de un swap de escena
     */
    void updateMapAfterSwap(const std::string& targetScene,
                           const std::unordered_map<std::string, SceneConfig>& sceneMap,
                           const std::unordered_map<std::string, SceneData>& sceneDataMap);
    
    /**
     * Obtiene el estado actual del juego
     */
    const GameState& getState() const { return state_; }
    GameState& getState() { return state_; }
    
    /**
     * Obtiene referencia a TransitionService para UI
     */
    TransitionService& getTransitions() { return transitions_; }
    const TransitionService& getTransitions() const { return transitions_; }
    
    /**
     * Obtiene referencia a MapRenderData
     */
    MapRenderData& getMapData() { return mapData_; }
    const MapRenderData& getMapData() const { return mapData_; }
    
    /**
     * Obtiene referencia a SpriteAnim del jugador
     */
    SpriteAnim& getPlayerAnim() { return playerAnim_; }
    const SpriteAnim& getPlayerAnim() const { return playerAnim_; }
    
    /**
     * Obtiene spawns de todas las escenas
     */
    const std::unordered_map<std::string, std::unordered_map<std::string, Vector2>>& getAllSceneSpawns() const {
        return allSceneSpawns_;
    }
    
    /**
     * Función helper para obtener punto objetivo de una escena
     */
    Vector2 getSceneTargetPoint(const std::string& sceneName) const;

private:
    GameState state_;
    TransitionService transitions_;
    MapRenderData mapData_;
    SpriteAnim playerAnim_;
    std::unordered_map<std::string, std::unordered_map<std::string, Vector2>> allSceneSpawns_;
};
