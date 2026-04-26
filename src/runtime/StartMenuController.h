#pragma once

#include "services/DestinationCatalog.h"
#include "services/MusicService.h"
#include "services/RuntimeBlockerService.h"
#include "services/SoundEffectService.h"
#include "services/initialization/IntroTourSetup.h"
#include "services/initialization/SceneBootstrap.h"

#include <raylib.h>

#include <string>
#include <vector>

class SceneManager;

enum class StartMenuAction {
    None,
    StartGameplay,
    Exit
};

class StartMenuController {
public:
    StartMenuController(bool enabled,
                        int screenWidth,
                        int screenHeight,
                        const IntroTourResources& introResources,
                        const SceneBootstrap& sceneBootstrap,
                        const DestinationCatalog& destinationCatalog,
                        SceneManager& sceneManager,
                        const RuntimeBlockerService& runtimeBlockerService);

    bool isActive() const;
    StartMenuAction updateAndRender(float dt,
                                    MusicService& musicService,
                                    SoundEffectService& soundEffectService);

private:
    struct IntroTourState {
        size_t sceneIndex{0};
        size_t waypointIndex{0};
        std::vector<Vector2> path;
        Vector2 virtualPlayerPos{0.0f, 0.0f};
        float travelSpeed{110.0f};
        float transitionTimer{0.0f};
        bool completedSceneRoute{false};
        bool transitioning{false};
        bool swappedAtMidpoint{false};
    };

    void initializePreviewScene();
    void rebuildIntroPath(size_t sceneIndex);
    void updateIntro(float dt);
    void renderMenu() const;
    std::string sceneDisplayName(const std::string& sceneName) const;
    static float computeTravelSpeed(const IntroTourResources& resources,
                                    const std::vector<Vector2>& path);

    bool enabled_{false};
    int screenWidth_{0};
    int screenHeight_{0};
    const IntroTourResources& introResources_;
    const SceneBootstrap& sceneBootstrap_;
    const DestinationCatalog& destinationCatalog_;
    SceneManager& sceneManager_;
    const RuntimeBlockerService& runtimeBlockerService_;
    int menuSelection_{0};
    int previousStartMenuSelection_{0};
    IntroTourState intro_{};
    Camera2D introCamera_{};
};
