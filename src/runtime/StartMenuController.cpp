#include "runtime/StartMenuController.h"

#include "runtime/SceneManager.h"
#include "services/MapRenderService.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace {
Vector2 firstPointOrZero(const std::vector<Vector2>& path) {
    return path.empty() ? Vector2{0.0f, 0.0f} : path.front();
}
}

StartMenuController::StartMenuController(bool enabled,
                                         int screenWidth,
                                         int screenHeight,
                                         const IntroTourResources& introResources,
                                         const SceneBootstrap& sceneBootstrap,
                                         const DestinationCatalog& destinationCatalog,
                                         SceneManager& sceneManager,
                                         const RuntimeBlockerService& runtimeBlockerService)
    : enabled_(enabled),
      screenWidth_(screenWidth),
      screenHeight_(screenHeight),
      introResources_(introResources),
      sceneBootstrap_(sceneBootstrap),
      destinationCatalog_(destinationCatalog),
      sceneManager_(sceneManager),
      runtimeBlockerService_(runtimeBlockerService) {
    if (!enabled_) {
        return;
    }

    initializePreviewScene();
    rebuildIntroPath(0);

    introCamera_.offset = Vector2{screenWidth_ * 0.5f, screenHeight_ * 0.5f};
    introCamera_.target = intro_.virtualPlayerPos;
    introCamera_.rotation = 0.0f;
    introCamera_.zoom = introResources_.cameraZoom;
}

bool StartMenuController::isActive() const {
    return enabled_;
}

StartMenuAction StartMenuController::updateAndRender(float dt,
                                                     MusicService& musicService,
                                                     SoundEffectService& soundEffectService) {
    if (!enabled_) {
        return StartMenuAction::None;
    }

    musicService.playMainMenuMusic();
    updateIntro(dt);

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        menuSelection_ = (menuSelection_ + 1) % 2;
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        menuSelection_ = (menuSelection_ + 1) % 2;
    }

    const int panelW = 520;
    const int panelH = 360;
    const Rectangle panelRect{
        static_cast<float>((screenWidth_ - panelW) / 2),
        static_cast<float>((screenHeight_ - panelH) / 2),
        static_cast<float>(panelW),
        static_cast<float>(panelH)
    };

    const Rectangle startRect{panelRect.x + 68.0f, panelRect.y + 136.0f, panelRect.width - 136.0f, 52.0f};
    const Rectangle exitRect{panelRect.x + 68.0f, panelRect.y + 204.0f, panelRect.width - 136.0f, 52.0f};
    const Vector2 mouse = GetMousePosition();
    if (CheckCollisionPointRec(mouse, startRect)) {
        menuSelection_ = 0;
    }
    if (CheckCollisionPointRec(mouse, exitRect)) {
        menuSelection_ = 1;
    }

    if (menuSelection_ != previousStartMenuSelection_) {
        soundEffectService.play(SoundEffectType::BetweenOptions);
        previousStartMenuSelection_ = menuSelection_;
    }

    renderMenu();

    const bool activate =
        IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    if (!activate) {
        return StartMenuAction::None;
    }

    soundEffectService.play(SoundEffectType::SelectButton);
    if (menuSelection_ == 0) {
        enabled_ = false;
        return StartMenuAction::StartGameplay;
    }

    return StartMenuAction::Exit;
}

void StartMenuController::initializePreviewScene() {
    sceneManager_.loadScene(sceneBootstrap_.resolveSceneName(introResources_.previewInitialSceneName),
                            sceneBootstrap_.sceneMap,
                            sceneBootstrap_.sceneDataMap,
                            runtimeBlockerService_);
}

void StartMenuController::rebuildIntroPath(size_t sceneIndex) {
    intro_.sceneIndex = sceneIndex;
    const std::string currentScene = introResources_.sceneOrder[intro_.sceneIndex];
    const std::string nextScene =
        introResources_.sceneOrder[(intro_.sceneIndex + 1) % introResources_.sceneOrder.size()];
    intro_.path = introResources_.buildSceneTourPath(
        intro_.sceneIndex,
        currentScene,
        nextScene,
        sceneManager_.getMapData(),
        sceneBootstrap_.allSceneSpawns,
        [&](const std::string& sceneId) { return sceneBootstrap_.resolveSceneName(sceneId); });
    intro_.waypointIndex = intro_.path.size() > 1 ? 1 : 0;
    intro_.virtualPlayerPos = firstPointOrZero(intro_.path);
    intro_.travelSpeed = computeTravelSpeed(introResources_, intro_.path);
}

void StartMenuController::updateIntro(float dt) {
    const MapRenderData& previewMap = sceneManager_.getMapData();

    if (!intro_.transitioning) {
        intro_.completedSceneRoute = false;
        if (intro_.path.size() > 1 && intro_.waypointIndex < intro_.path.size()) {
            float remainingStep = intro_.travelSpeed * dt;
            size_t safeGuard = 0;
            while (remainingStep > 0.0f && safeGuard < intro_.path.size() * 2) {
                const Vector2 target = intro_.path[intro_.waypointIndex];
                Vector2 delta{target.x - intro_.virtualPlayerPos.x, target.y - intro_.virtualPlayerPos.y};
                const float dist = std::sqrt(delta.x * delta.x + delta.y * delta.y);

                if (dist <= 0.001f) {
                    if (intro_.waypointIndex + 1 >= intro_.path.size()) {
                        intro_.completedSceneRoute = true;
                        remainingStep = 0.0f;
                        break;
                    }
                    ++intro_.waypointIndex;
                    ++safeGuard;
                    continue;
                }

                if (remainingStep >= dist) {
                    intro_.virtualPlayerPos = target;
                    remainingStep -= dist;
                    if (intro_.waypointIndex + 1 >= intro_.path.size()) {
                        intro_.completedSceneRoute = true;
                        remainingStep = 0.0f;
                        break;
                    }
                    ++intro_.waypointIndex;
                } else {
                    intro_.virtualPlayerPos.x += (delta.x / dist) * remainingStep;
                    intro_.virtualPlayerPos.y += (delta.y / dist) * remainingStep;
                    remainingStep = 0.0f;
                }
            }
        }

        if (intro_.completedSceneRoute || intro_.path.size() <= 1) {
            intro_.transitioning = true;
            intro_.transitionTimer = 0.0f;
            intro_.swappedAtMidpoint = false;
        }
    } else {
        intro_.transitionTimer += dt;
        const float transitionDuration = introResources_.transitionDuration;
        const float transitionHalf = transitionDuration * 0.5f;

        if (!intro_.swappedAtMidpoint && intro_.transitionTimer >= transitionHalf) {
            const size_t nextSceneIndex = (intro_.sceneIndex + 1) % introResources_.sceneOrder.size();
            sceneManager_.loadScene(sceneBootstrap_.resolveSceneName(introResources_.sceneOrder[nextSceneIndex]),
                                    sceneBootstrap_.sceneMap,
                                    sceneBootstrap_.sceneDataMap,
                                    runtimeBlockerService_);
            rebuildIntroPath(nextSceneIndex);
            intro_.completedSceneRoute = false;
            intro_.swappedAtMidpoint = true;
        }

        if (intro_.transitionTimer >= transitionDuration) {
            intro_.transitioning = false;
            intro_.transitionTimer = 0.0f;
            intro_.swappedAtMidpoint = false;
        }
    }

    introCamera_.zoom = introResources_.cameraZoom;
    introCamera_.rotation = 0.0f;
    const float followFactor =
        std::clamp(dt * introResources_.cameraFollowLerp * introResources_.followScale, 0.0f, 1.0f);
    introCamera_.target.x += (intro_.virtualPlayerPos.x - introCamera_.target.x) * followFactor;
    introCamera_.target.y += (intro_.virtualPlayerPos.y - introCamera_.target.y) * followFactor;
    IntroTourResources::clampIntroCamera(introCamera_, previewMap, screenWidth_, screenHeight_);
}

void StartMenuController::renderMenu() const {
    const MapRenderData& previewMap = sceneManager_.getMapData();
    const int panelW = 520;
    const int panelH = 360;
    const Rectangle panelRect{
        static_cast<float>((screenWidth_ - panelW) / 2),
        static_cast<float>((screenHeight_ - panelH) / 2),
        static_cast<float>(panelW),
        static_cast<float>(panelH)
    };
    const Rectangle startRect{panelRect.x + 68.0f, panelRect.y + 136.0f, panelRect.width - 136.0f, 52.0f};
    const Rectangle exitRect{panelRect.x + 68.0f, panelRect.y + 204.0f, panelRect.width - 136.0f, 52.0f};

    BeginDrawing();
    ClearBackground({12, 14, 20, 255});
    BeginMode2D(introCamera_);
    if (previewMap.hasTexture) {
        MapRenderService::drawMapWithHitboxes(previewMap, false);
    }
    EndMode2D();

    if (intro_.transitioning) {
        const float phase = intro_.transitionTimer / introResources_.transitionDuration;
        const float blend = phase < 0.5f ? (phase * 2.0f) : ((1.0f - phase) * 2.0f);
        const int alpha = static_cast<int>(std::clamp(blend * 235.0f, 0.0f, 235.0f));
        DrawRectangle(0, 0, screenWidth_, screenHeight_, Color{8, 10, 16, static_cast<unsigned char>(alpha)});

        const std::string nextScene =
            introResources_.sceneOrder[(intro_.sceneIndex + 1) % introResources_.sceneOrder.size()];
        const std::string transitionText = "Transicion a " + sceneDisplayName(nextScene);
        const int txtW = MeasureText(transitionText.c_str(), 28);
        DrawText(transitionText.c_str(),
                 (screenWidth_ - txtW) / 2,
                 screenHeight_ - 92,
                 28,
                 Color{240, 240, 245, 220});
    }

    DrawRectangle(0, 0, screenWidth_, screenHeight_, Color{0, 0, 0, 120});
    DrawRectangle(static_cast<int>(panelRect.x),
                  static_cast<int>(panelRect.y),
                  static_cast<int>(panelRect.width),
                  static_cast<int>(panelRect.height),
                  Color{28, 36, 78, 238});
    DrawRectangleLinesEx(panelRect, 4.0f, Color{246, 229, 126, 255});
    DrawRectangle(static_cast<int>(panelRect.x + 10.0f),
                  static_cast<int>(panelRect.y + 10.0f),
                  static_cast<int>(panelRect.width - 20.0f),
                  52,
                  Color{46, 60, 122, 250});
    DrawRectangleLinesEx(Rectangle{panelRect.x + 10.0f, panelRect.y + 10.0f, panelRect.width - 20.0f, 52.0f},
                         3.0f,
                         Color{255, 255, 255, 230});

    const std::string title = "EcoCampusNav";
    const int titleSize = 44;
    const int titleW = MeasureText(title.c_str(), titleSize);
    const Vector2 titleMeasure{static_cast<float>(titleW), static_cast<float>(titleSize)};
    const Vector2 titlePos{panelRect.x + (panelRect.width - titleMeasure.x) * 0.5f, panelRect.y + 22.0f};
    DrawText(title.c_str(),
             static_cast<int>(titlePos.x + 2.0f),
             static_cast<int>(titlePos.y + 2.0f),
             titleSize,
             Color{0, 0, 0, 200});
    DrawText(title.c_str(),
             static_cast<int>(titlePos.x),
             static_cast<int>(titlePos.y),
             titleSize,
             Color{255, 248, 186, 255});

    const std::string sceneLabel = "Recorrido: " + sceneDisplayName(introResources_.sceneOrder[intro_.sceneIndex]);
    DrawText(sceneLabel.c_str(),
             static_cast<int>(panelRect.x + 30.0f),
             static_cast<int>(panelRect.y + 86.0f),
             24,
             Color{194, 228, 255, 245});

    const auto drawMenuButton = [&](const Rectangle& rect, const char* label, bool selected) {
        const Color bg = selected ? Color{230, 86, 74, 255} : Color{56, 72, 138, 245};
        const Color border = selected ? Color{255, 233, 130, 255} : Color{196, 214, 255, 240};
        DrawRectangle(static_cast<int>(rect.x),
                      static_cast<int>(rect.y),
                      static_cast<int>(rect.width),
                      static_cast<int>(rect.height),
                      bg);
        DrawRectangleLinesEx(rect, 3.0f, border);

        const std::string text = label;
        const int btnTextSize = 38;
        const int textW = MeasureText(text.c_str(), btnTextSize);
        const int textX = static_cast<int>(rect.x + (rect.width - static_cast<float>(textW)) * 0.5f);
        const int textY = static_cast<int>(rect.y + 6.0f);
        DrawText(text.c_str(), textX + 2, textY + 2, btnTextSize, Color{0, 0, 0, 180});
        DrawText(text.c_str(), textX, textY, btnTextSize, Color{255, 250, 238, 255});
        if (selected) {
            DrawText(">",
                     static_cast<int>(rect.x - 24.0f),
                     static_cast<int>(rect.y + 12.0f),
                     38,
                     Color{255, 237, 132, 255});
            DrawText("<",
                     static_cast<int>(rect.x + rect.width + 10.0f),
                     static_cast<int>(rect.y + 12.0f),
                     38,
                     Color{255, 237, 132, 255});
        }
    };

    drawMenuButton(startRect, "Iniciar Juego", menuSelection_ == 0);
    drawMenuButton(exitRect, "Salir", menuSelection_ == 1);

    const float creditsDividerY = panelRect.y + panelRect.height - 76.0f;
    DrawLine(static_cast<int>(panelRect.x + 24.0f),
             static_cast<int>(creditsDividerY),
             static_cast<int>(panelRect.x + panelRect.width - 24.0f),
             static_cast<int>(creditsDividerY),
             Color{100, 120, 160, 180});

    const int creditsTitleSize = 18;
    const std::string creditsTitle = "Desarrollado por";
    const int creditsTitleW = MeasureText(creditsTitle.c_str(), creditsTitleSize);
    DrawText(creditsTitle.c_str(),
             static_cast<int>(panelRect.x + (panelRect.width - creditsTitleW) * 0.5f),
             static_cast<int>(creditsDividerY + 8.0f),
             creditsTitleSize,
             Color{255, 230, 150, 255});

    const int authorFontSize = 15;
    const std::string authorOne = "Javier Mendoza Gonzalez";
    const std::string authorTwo = "Andrey Hernandez Salazar";
    DrawText(authorOne.c_str(),
             static_cast<int>(panelRect.x + (panelRect.width - MeasureText(authorOne.c_str(), authorFontSize)) *
                              0.5f),
             static_cast<int>(creditsDividerY + 28.0f),
             authorFontSize,
             Color{200, 210, 230, 220});
    DrawText(authorTwo.c_str(),
             static_cast<int>(panelRect.x + (panelRect.width - MeasureText(authorTwo.c_str(), authorFontSize)) *
                              0.5f),
             static_cast<int>(creditsDividerY + 46.0f),
             authorFontSize,
             Color{200, 210, 230, 220});

    const int yearFontSize = 16;
    const std::string yearLabel = "2026";
    DrawText(yearLabel.c_str(),
             static_cast<int>(panelRect.x + (panelRect.width - MeasureText(yearLabel.c_str(), yearFontSize)) * 0.5f),
             static_cast<int>(creditsDividerY + 64.0f),
             yearFontSize,
             Color{180, 190, 210, 200});

    DrawText("W/S o Flechas para elegir",
             static_cast<int>(panelRect.x + 28.0f),
             static_cast<int>(panelRect.y + panelRect.height - 20.0f),
             18,
             Color{220, 230, 250, 245});
    DrawText("Enter para confirmar",
             static_cast<int>(panelRect.x + panelRect.width - 212.0f),
             static_cast<int>(panelRect.y + panelRect.height - 20.0f),
             18,
             Color{220, 230, 250, 245});
    EndDrawing();
}

std::string StartMenuController::sceneDisplayName(const std::string& sceneName) const {
    return sceneBootstrap_.sceneDisplayName(sceneName, destinationCatalog_);
}

float StartMenuController::computeTravelSpeed(const IntroTourResources& resources,
                                              const std::vector<Vector2>& path) {
    return std::clamp((IntroTourResources::scenePathLength(path) / resources.secondsPerScene) *
                          resources.speedScale,
                      24.0f,
                      92.0f);
}
