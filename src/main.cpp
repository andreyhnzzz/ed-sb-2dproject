#include <raylib.h>

#include <iostream>
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

#include "core/application/AudioInitializer.h"
#include "core/application/WindowInitializer.h"
#include "runtime/GameplayLoopController.h"
#include "runtime/GameController.h"
#include "runtime/InputManager.h"
#include "runtime/RuntimeNavigationManager.h"
#include "runtime/SceneManager.h"
#include "runtime/StartMenuController.h"
#include "runtime/UIManager.h"
#include "services/AudioManager.h"
#include "services/AssetPathResolver.h"
#include "services/ComplexityAnalyzer.h"
#include "services/DataManager.h"
#include "services/DestinationCatalog.h"
#include "services/MusicService.h"
#include "services/ResilienceService.h"
#include "services/RuntimeBlockerService.h"
#include "services/ScenarioManager.h"
#include "services/SoundEffectService.h"
#include "services/WalkablePathService.h"
#include "services/initialization/FloorLinkLoader.h"
#include "services/initialization/IntroTourSetup.h"
#include "services/initialization/SceneBootstrap.h"
#include "ui/TabManager.h"

static constexpr double kPixelsToMeters = 0.10;
static constexpr bool kIntroMenuEnabled = true;

namespace {
std::string canonicalSceneId(std::string sceneName) {
    std::transform(sceneName.begin(), sceneName.end(), sceneName.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return sceneName;
}
}

int main(int argc, char* argv[]) {
    const char* executablePath = argc > 0 ? argv[0] : nullptr;
    std::string path = AssetPathResolver::findCampusJson(executablePath);
    if (path.empty()) {
        std::cerr << "campus.json was not found. Place the file in the working directory.\n";
        return 1;
    }

    WindowConfig windowConfig;
    WindowInitializer::initialize(windowConfig);
    int screenWidth = windowConfig.width;
    int screenHeight = windowConfig.height;
    AudioInitializer::initialize();

    MusicService musicService;
    SoundEffectService soundEffectService;
    AudioInitializer::loadMusicAssets(musicService, executablePath);
    AudioInitializer::loadSoundEffects(soundEffectService, executablePath);

    const SceneBootstrap sceneBootstrap = SceneBootstrap::load(executablePath, path);

    DataManager dataManager;
    CampusGraph graph;
    try {
        graph = dataManager.loadCampusGraph(path, sceneBootstrap.sceneToTmjPath,
                                            sceneBootstrap.interestZonesPath, kPixelsToMeters);
        dataManager.exportResolvedGraph(graph, sceneBootstrap.generatedGraphRuntimePath);
    } catch (const std::exception& ex) {
        std::cerr << "Error loading GIS data: " << ex.what() << "\n";
        return 1;
    }

    NavigationService navService(graph);
    ScenarioManager scenarioManager;
    ComplexityAnalyzer complexityAnalyzer(graph);
    ResilienceService resilienceService(graph);
    DestinationCatalog destinationCatalog;

    std::vector<SceneLink> sceneLinks;
    destinationCatalog.loadFromGeneratedJson(sceneBootstrap.generatedGraphRuntimePath,
                                             sceneBootstrap.sceneDataMap);
    scenarioManager.setReferenceWaypoints(destinationCatalog.preferredReferenceWaypoints(graph));

    TransitionService transitions;
    sceneBootstrap.buildPortalSceneLinks(transitions, sceneLinks);
    const auto floorLinks = FloorLinkLoader::loadFloorLinks(
        transitions, sceneBootstrap.floorScenes, sceneBootstrap.allSceneSpawns,
        sceneBootstrap.allFloorTriggers);
    sceneLinks.insert(sceneLinks.end(), floorLinks.begin(), floorLinks.end());

    RuntimeBlockerService runtimeBlockerService;
    runtimeBlockerService.rebuildOptions(graph, destinationCatalog, sceneLinks);
    transitions.setBlockerService(&runtimeBlockerService);
    std::vector<std::pair<std::string, std::string>> routeScenes;
    sceneBootstrap.buildRouteScenes(destinationCatalog, routeScenes);

    InputManager inputManager;
    GameController gameController;
    SceneManager sceneManager(executablePath);
    UIManager uiManager;
    RuntimeNavigationManager runtimeNavigation(destinationCatalog);
    auto sceneDisplayName = [&](const std::string& sceneName) {
        return sceneBootstrap.sceneDisplayName(sceneName, destinationCatalog);
    };
    auto sceneTargetPoint = [&](const std::string& sceneName) {
        return sceneBootstrap.sceneTargetPoint(sceneName);
    };

    const IntroTourResources introResources =
        IntroTourResources::load(executablePath, sceneBootstrap.allSceneSpawns);

    const std::string idlePath = AssetPathResolver::findPlayerIdleSprite(executablePath);
    const std::string walkPath = AssetPathResolver::findPlayerWalkSprite(executablePath);
    gameController.loadPlayerSprites(idlePath, walkPath);
    gameController.setCameraOffset(Vector2{screenWidth * 0.5f, screenHeight * 0.5f});

    const Vector2 gameplaySpawnPos = [&]() -> Vector2 {
        const auto sceneIt = sceneBootstrap.allSceneSpawns.find(introResources.gameplayInitialSceneName);
        if (sceneIt != sceneBootstrap.allSceneSpawns.end()) {
            const auto spawnIt = sceneIt->second.find("bus_arrive");
            if (spawnIt != sceneIt->second.end()) return spawnIt->second;
        }
        return WalkablePathService::findSpawnPoint(sceneManager.getMapData());
    }();

    TabManagerState tabState = createTabManagerState(graph, sceneBootstrap.generatedGraphRuntimePath);
    RouteRuntimeState routeState;
    routeState.routeMobilityReduced = scenarioManager.isMobilityReduced();
    routeState.routeAnchorPos = gameplaySpawnPos;

    UIManager::State uiState;
    GameplayLoopController gameplayController(
        screenWidth,
        screenHeight,
        graph,
        navService,
        scenarioManager,
        complexityAnalyzer,
        resilienceService,
        destinationCatalog,
        sceneLinks,
        inputManager,
        gameController,
        sceneManager,
        uiManager,
        runtimeNavigation,
        transitions,
        runtimeBlockerService,
        musicService,
        soundEffectService,
        sceneBootstrap,
        routeScenes,
        tabState,
        uiState,
        routeState,
        canonicalSceneId,
        sceneDisplayName,
        sceneTargetPoint);

    StartMenuController startMenuController(kIntroMenuEnabled,
                                            screenWidth,
                                            screenHeight,
                                            introResources,
                                            sceneBootstrap,
                                            destinationCatalog,
                                            sceneManager,
                                            runtimeBlockerService);

    if (!kIntroMenuEnabled) {
        gameplayController.begin(introResources.gameplayInitialSceneName, gameplaySpawnPos);
    }

    bool exitRequested = false;

    while (!WindowShouldClose() && !exitRequested) {
        const float dt = GetFrameTime();
        musicService.update();

        if (startMenuController.isActive()) {
            const StartMenuAction action =
                startMenuController.updateAndRender(dt, musicService, soundEffectService);
            if (action == StartMenuAction::StartGameplay) {
                gameplayController.begin(introResources.gameplayInitialSceneName, gameplaySpawnPos);
            } else if (action == StartMenuAction::Exit) {
                exitRequested = true;
            }
            continue;
        }
        gameplayController.runFrame(dt);
    }

    sceneManager.unload();
    gameController.unloadPlayerSprites();
    musicService.unloadAll();
    soundEffectService.unloadAll();
    AudioManager::getInstance().shutdown();
    WindowInitializer::cleanup();
    return 0;
}
