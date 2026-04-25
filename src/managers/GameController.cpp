#include "GameController.h"
#include "../services/AssetPathResolver.h"
#include "../services/StringUtils.h"
#include <algorithm>
#include <cmath>

void GameController::initialize(
    const std::string& initialSceneName,
    const std::unordered_map<std::string, SceneConfig>& sceneMap,
    const std::unordered_map<std::string, SceneData>& sceneDataMap,
    const std::unordered_map<std::string, std::unordered_map<std::string, Vector2>>& allSceneSpawns) {
    
    state_.currentSceneName = initialSceneName;
    allSceneSpawns_ = allSceneSpawns;
    
    // Cargar mapa inicial
    const auto& initConfig = sceneMap.at(initialSceneName);
    const std::string pngPath = AssetPathResolver::resolveAssetPath(nullptr, initConfig.pngPath);
    if (!pngPath.empty()) {
        mapData_.texture = LoadTexture(pngPath.c_str());
        mapData_.hasTexture = mapData_.texture.id != 0;
    }
    
    const auto sdIt = sceneDataMap.find(initialSceneName);
    if (sdIt != sceneDataMap.end() && sdIt->second.isValid) {
        mapData_.hitboxes = sdIt->second.hitboxes;
        mapData_.interestZones = sdIt->second.interestZones;
    }
    
    // Inicializar posición del jugador desde spawn point
    const auto scIt = allSceneSpawns_.find(initialSceneName);
    if (scIt != allSceneSpawns_.end()) {
        const auto spIt = scIt->second.find("bus_arrive");
        if (spIt != scIt->second.end()) {
            state_.playerPos = spIt->second;
        } else if (!scIt->second.empty()) {
            state_.playerPos = scIt->second.begin()->second;
        } else {
            state_.playerPos = WalkablePathService::findSpawnPoint(mapData_);
        }
    } else {
        state_.playerPos = WalkablePathService::findSpawnPoint(mapData_);
    }
    
    // Configurar cámara
    state_.camera.offset = Vector2{800.0f * 0.5f, 600.0f * 0.5f}; // Valores por defecto, se actualizan en runtime
    state_.camera.target = state_.playerPos;
    state_.camera.rotation = 0.0f;
    state_.camera.zoom = 2.2f;
    
    // Inicializar animación del jugador
    playerAnim_.direction = 0;
    playerAnim_.frame = 0;
    playerAnim_.timer = 0.0f;
}

void GameController::updatePlayer(float dt, float moveX, float moveY, bool sprinting, bool infoMenuOpen) {
    if (infoMenuOpen) return;
    
    const float currentSpeed = state_.playerSpeed * (sprinting ? state_.sprintMultiplier : 1.0f);
    
    // Actualizar dirección de animación
    if (moveY < 0.0f) playerAnim_.direction = 1; // up
    else if (moveY > 0.0f) playerAnim_.direction = 3; // down
    else if (moveX < 0.0f) playerAnim_.direction = 2; // left
    else if (moveX > 0.0f) playerAnim_.direction = 0; // right
    
    // Movimiento en X
    Vector2 candidate = state_.playerPos;
    candidate.x += moveX * currentSpeed * dt;
    if (mapData_.hasTexture) {
        candidate.x = std::clamp(candidate.x, 8.0f, static_cast<float>(mapData_.texture.width) - 8.0f);
    }
    if (!WalkablePathService::intersectsAny(WalkablePathService::playerColliderAt(candidate), mapData_.hitboxes)) {
        state_.playerPos.x = candidate.x;
    }
    
    // Movimiento en Y
    candidate = state_.playerPos;
    candidate.y += moveY * currentSpeed * dt;
    if (mapData_.hasTexture) {
        candidate.y = std::clamp(candidate.y, 14.0f, static_cast<float>(mapData_.texture.height));
    }
    if (!WalkablePathService::intersectsAny(WalkablePathService::playerColliderAt(candidate), mapData_.hitboxes)) {
        state_.playerPos.y = candidate.y;
    }
    
    // Actualizar animación
    const bool isMoving = (moveX != 0.0f || moveY != 0.0f);
    if (isMoving && playerAnim_.hasWalk) {
        playerAnim_.timer += dt;
        const float frameStep = sprinting ? (1.0f / 16.0f) : (1.0f / 12.0f);
        if (playerAnim_.timer >= frameStep) {
            playerAnim_.timer = 0.0f;
            playerAnim_.frame = (playerAnim_.frame + 1) % SpriteAnimationService::directionalFrameCount(playerAnim_.walkFrames);
        }
    } else {
        playerAnim_.timer = 0.0f;
        playerAnim_.frame = 0;
    }
}

void GameController::updateCamera(int screenWidth, int screenHeight) {
    state_.camera.target = state_.playerPos;
    
    // Clamp camera target to map bounds
    if (!mapData_.hasTexture) return;
    
    const float halfViewWidth = (static_cast<float>(screenWidth) * 0.5f) / state_.camera.zoom;
    const float halfViewHeight = (static_cast<float>(screenHeight) * 0.5f) / state_.camera.zoom;
    const float minX = halfViewWidth;
    const float maxX = static_cast<float>(mapData_.texture.width) - halfViewWidth;
    const float minY = halfViewHeight;
    const float maxY = static_cast<float>(mapData_.texture.height) - halfViewHeight;
    
    if (minX > maxX) {
        state_.camera.target.x = static_cast<float>(mapData_.texture.width) * 0.5f;
    } else {
        state_.camera.target.x = std::clamp(state_.camera.target.x, minX, maxX);
    }
    if (minY > maxY) {
        state_.camera.target.y = static_cast<float>(mapData_.texture.height) * 0.5f;
    } else {
        state_.camera.target.y = std::clamp(state_.camera.target.y, minY, maxY);
    }
}

void GameController::processTransitions(float dt) {
    transitions_.update(WalkablePathService::playerColliderAt(state_.playerPos), state_.currentSceneName, dt);
}

void GameController::updateMapAfterSwap(
    const std::string& targetScene,
    const std::unordered_map<std::string, SceneConfig>& sceneMap,
    const std::unordered_map<std::string, SceneData>& sceneDataMap) {
    
    if (mapData_.hasTexture) {
        UnloadTexture(mapData_.texture);
        mapData_.texture = {};
        mapData_.hasTexture = false;
    }
    mapData_.hitboxes.clear();
    mapData_.interestZones.clear();
    
    const auto scIt = sceneMap.find(targetScene);
    if (scIt != sceneMap.end()) {
        const std::string pngPath = AssetPathResolver::resolveAssetPath(nullptr, scIt->second.pngPath);
        if (!pngPath.empty()) {
            mapData_.texture = LoadTexture(pngPath.c_str());
            mapData_.hasTexture = mapData_.texture.id != 0;
        }
        
        const auto sdIt = sceneDataMap.find(targetScene);
        if (sdIt != sceneDataMap.end() && sdIt->second.isValid) {
            mapData_.hitboxes = sdIt->second.hitboxes;
            mapData_.interestZones = sdIt->second.interestZones;
        }
        
        state_.currentSceneName = targetScene;
    }
    
    // Actualizar posición del jugador al spawn point de la nueva escena
    const auto scSpawnIt = allSceneSpawns_.find(targetScene);
    if (scSpawnIt != allSceneSpawns_.end() && !scSpawnIt->second.empty()) {
        const auto spawnIt = scSpawnIt->second.find("elevator_arrive");
        if (spawnIt != scSpawnIt->second.end()) {
            state_.playerPos = spawnIt->second;
        } else {
            state_.playerPos = scSpawnIt->second.begin()->second;
        }
    }
    
    state_.camera.target = state_.playerPos;
    state_.camera.zoom = 2.2f;
}

Vector2 GameController::getSceneTargetPoint(const std::string& sceneName) const {
    const auto spawnMapIt = allSceneSpawns_.find(sceneName);
    if (spawnMapIt == allSceneSpawns_.end() || spawnMapIt->second.empty()) {
        return Vector2{0.0f, 0.0f};
    }
    
    const auto& spawnMap = spawnMapIt->second;
    for (const std::string& preferred : {
             std::string("bus_arrive"),
             std::string("ext_from_bus"),
             std::string("intcafe_arrive"),
             std::string("biblio_main_arrive"),
             std::string("elevator_arrive"),
             std::string("piso4_main_L_arrive")
         }) {
        const auto it = spawnMap.find(preferred);
        if (it != spawnMap.end()) return it->second;
    }
    
    return spawnMap.begin()->second;
}
