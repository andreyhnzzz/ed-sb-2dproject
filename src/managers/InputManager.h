#pragma once
#include <raylib.h>
#include <functional>

/**
 * InputManager - Captura de teclas y eventos de input
 * Responsabilidad: Manejar entrada de teclado (TAB, M, WASD, Shift, UI toggles)
 * Sin estado de juego, solo delega eventos mediante callbacks
 */
class InputManager {
public:
    // Callbacks para eventos de input
    std::function<void()> onMenuToggle;        // Tecla M
    std::function<void()> onPoiToggle;         // Tecla TAB
    std::function<void(float)> onMouseWheel;   // Zoom de cámara
    
    // Estado de teclas de movimiento
    struct MoveInput {
        float x{0.0f};
        float y{0.0f};
        bool sprinting{false};
    };
    
    InputManager() = default;
    
    /**
     * Procesa inputs globales (no bloqueados por UI)
     * @return true si hubo algún input relevante
     */
    bool processGlobalInputs();
    
    /**
     * Obtiene el estado actual de movimiento del jugador
     * @param infoMenuOpen Si el menú está abierto, no procesa movimiento
     * @return MoveInput con dirección y estado de sprint
     */
    MoveInput getMoveInput(bool infoMenuOpen) const;
    
    /**
     * Procesa el zoom de la cámara con la rueda del mouse
     * @param cameraZoom Zoom actual
     * @param minZoom Zoom mínimo permitido
     * @param maxZoom Zoom máximo permitido
     * @return Nuevo valor de zoom
     */
    float processCameraZoom(float cameraZoom, float minZoom, float maxZoom);
    
    /**
     * Verifica si la ventana debería cerrarse
     */
    bool shouldClose() const { return WindowShouldClose(); }
};
