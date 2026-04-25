#include "InputManager.h"

bool InputManager::processGlobalInputs() {
    bool handled = false;
    
    if (IsKeyPressed(KEY_M)) {
        if (onMenuToggle) onMenuToggle();
        handled = true;
    }
    
    if (IsKeyPressed(KEY_TAB)) {
        if (onPoiToggle) onPoiToggle();
        handled = true;
    }
    
    return handled;
}

InputManager::MoveInput InputManager::getMoveInput(bool infoMenuOpen) const {
    MoveInput input;
    
    if (!infoMenuOpen) {
        if (IsKeyDown(KEY_W)) input.y -= 1.0f;
        if (IsKeyDown(KEY_S)) input.y += 1.0f;
        if (IsKeyDown(KEY_A)) input.x -= 1.0f;
        if (IsKeyDown(KEY_D)) input.x += 1.0f;
        
        input.sprinting = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    }
    
    // Normalizar vector de movimiento
    const float len = std::sqrt(input.x * input.x + input.y * input.y);
    if (len > 0.0f) {
        input.x /= len;
        input.y /= len;
    }
    
    return input;
}

float InputManager::processCameraZoom(float cameraZoom, float minZoom, float maxZoom) {
    const float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        cameraZoom = std::clamp(cameraZoom + wheel * 0.15f, minZoom, maxZoom);
    }
    return cameraZoom;
}
