#pragma once
#include <string>
#include <vector>
#include <utility>

#include "../core/graph/CampusGraph.h"
#include "../services/NavigationService.h"
#include "../services/ScenarioManager.h"
#include "../services/ResilienceService.h"

/**
 * NavigationController - Puente entre UI y servicios de navegación
 * Responsabilidad: Coordinar cálculos de ruta, gestión de destinos y perfiles
 */
class NavigationController {
public:
    struct DestinationInfo {
        std::string id;
        std::string name;
        std::string type;
        Vector2 position{0.0f, 0.0f};
    };
    
    NavigationController(CampusGraph& graph, 
                        NavigationService& navService,
                        ScenarioManager& scenarioManager,
                        ResilienceService& resilienceService);
    
    /**
     * Carga destinos desde campus_generated.json (POIs)
     */
    void loadDestinationsFromJson(const std::string& jsonPath);
    
    /**
     * Obtiene lista de destinos disponibles
     */
    const std::vector<DestinationInfo>& getDestinations() const { return destinations_; }
    
    /**
     * Establece el destino actual
     */
    void setDestination(const std::string& destinationId);
    
    /**
     * Obtiene el destino actual
     */
    const std::string& getCurrentDestination() const { return currentDestination_; }
    
    /**
     * Calcula ruta perfilada según el perfil de estudiante activo
     */
    PathResult calculateProfiledPath(const std::string& origin, const std::string& destination);
    
    /**
     * Verifica si se llegó al destino (proximidad umbral)
     */
    bool hasReachedDestination(const Vector2& playerPos, float threshold = 20.0f) const;
    
    /**
     * Limpia la ruta activa (al llegar al destino)
     */
    void clearActiveRoute();
    
    /**
     * Verifica si hay una ruta activa
     */
    bool hasActiveRoute() const { return routeActive_; }
    
    /**
     * Obtiene el plan de escenas de la ruta actual
     */
    const std::vector<std::string>& getRouteScenePlan() const { return routeScenePlan_; }
    
    /**
     * Actualiza estado de ruta activa
     */
    void setRouteActive(bool active) { routeActive_ = active; }
    
    /**
     * Simula bloqueo de nodo/arista para resiliencia
     */
    void simulateBlockage(const std::string& elementId, bool isNode);
    
    /**
     * Recalcula ruta tras bloqueo dinámico
     */
    PathResult recalculateAfterBlockage(const std::string& origin, const std::string& destination);
    
    /**
     * Obtiene referencia a ResilienceService
     */
    ResilienceService& getResilienceService() { return resilienceService_; }
    
private:
    CampusGraph& graph_;
    NavigationService& navService_;
    ScenarioManager& scenarioManager_;
    ResilienceService& resilienceService_;
    
    std::vector<DestinationInfo> destinations_;
    std::string currentDestination_;
    std::vector<std::string> routeScenePlan_;
    bool routeActive_{false};
};
