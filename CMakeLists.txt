cmake_minimum_required(VERSION 3.20)
<<<<<<< HEAD
project(EcoCampus)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

=======
project(EcoCampusNav VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
>>>>>>> 229f5d3 (migration to qt)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

<<<<<<< HEAD
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGL OpenGLWidgets)

include(FetchContent)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        0.9.9.8
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.2
=======
find_package(Qt6 REQUIRED COMPONENTS Widgets)

include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
>>>>>>> 229f5d3 (migration to qt)
)
FetchContent_MakeAvailable(nlohmann_json)

set(SOURCES
    src/main.cpp
<<<<<<< HEAD
    src/core/GameLoop.h
    src/core/GameLoop.cpp
    src/core/EventBus.h
    src/core/EventBus.cpp
    src/render/Shader.h
    src/render/Shader.cpp
    src/render/Renderer.h
    src/render/Renderer.cpp
    src/scene/VCRoad.h
    src/scene/VCRoad.cpp
    src/scene/Room.h
    src/scene/Room.cpp
    src/scene/SceneManager.h
    src/scene/SceneManager.cpp
    src/entity/Player.h
    src/entity/Player.cpp
    src/entity/Door.h
    src/entity/Door.cpp
    src/narrative/Narrator.h
    src/narrative/Narrator.cpp
    src/input/InputHandler.h
    src/input/InputHandler.cpp
)

add_executable(EcoCampus WIN32 ${SOURCES})

target_include_directories(EcoCampus PRIVATE src)

target_link_libraries(EcoCampus
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    glm
    nlohmann_json::nlohmann_json
    opengl32
)

# Copy assets to build directory
add_custom_command(TARGET EcoCampus POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:EcoCampus>/assets"
)
=======
    src/core/graph/CampusGraph.cpp
    src/core/graph/Algorithms.cpp
    src/repositories/JsonGraphRepository.cpp
    src/services/NavigationService.cpp
    src/services/ScenarioManager.cpp
    src/services/ComplexityAnalyzer.cpp
    src/services/ResilienceService.cpp
    src/ui/MainWindow.cpp
    src/ui/CampusMapView.cpp
    src/ui/NodeItem.cpp
    src/ui/EdgeItem.cpp
    src/ui/DfsTab.cpp
    src/ui/BfsTab.cpp
    src/ui/ConnectivityTab.cpp
    src/ui/PathfindTab.cpp
    src/ui/ScenarioTab.cpp
    src/ui/ComplexityTab.cpp
    src/ui/ResilienceTab.cpp
)

add_executable(EcoCampusNav ${SOURCES})

target_include_directories(EcoCampusNav PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(EcoCampusNav PRIVATE Qt6::Widgets nlohmann_json::nlohmann_json)

configure_file(${CMAKE_SOURCE_DIR}/campus.json ${CMAKE_BINARY_DIR}/campus.json COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/settings.json ${CMAKE_BINARY_DIR}/settings.json COPYONLY)
>>>>>>> 229f5d3 (migration to qt)
