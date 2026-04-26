# Contexto Actual de EcoCampusNav

## Resumen Ejecutivo

`EcoCampusNav` es un proyecto de navegacion universitaria en `C++17` con renderizado en `raylib` y paneles de apoyo en `ImGui`. El proyecto ya no depende de un `main.cpp` monolitico: la orquestacion de arranque y ciclo de vida fue movida a una sesion de aplicacion, mientras que el runtime quedo dividido entre controladores especializados.

## Estado Actual de la Arquitectura

| Area | Estado actual |
| --- | --- |
| Entrada de la app | `src/main.cpp` solo crea `ApplicationSession`, inicializa y ejecuta |
| Bootstrap | `src/core/application/ApplicationSession.*` coordina carga, servicios, loop y shutdown |
| Menu principal | `src/runtime/StartMenuController.*` |
| Gameplay | `src/runtime/GameplayLoopController.*` |
| Escenas | `SceneBootstrap`, `SceneManager`, `TransitionService` |
| Navegacion | `NavigationService`, `RuntimeNavigationManager`, `ScenarioManager` |
| UI academica | `UIManager` y `ui/TabManager.*` |

## Responsabilidades Clave

### `ApplicationSession`
- Resuelve `campus.json` y assets.
- Inicializa ventana, audio y sprites.
- Carga el grafo del campus y exporta el runtime generado.
- Construye servicios de navegacion, resiliencia y rutas.
- Instancia controladores de menu y gameplay.
- Ejecuta el loop principal y el shutdown.

### `StartMenuController`
- Mantiene el menu principal.
- Reproduce el recorrido visual de introduccion.
- Controla seleccion, sonido y transicion a gameplay.

### `GameplayLoopController`
- Administra input, zoom, colisiones y camara.
- Recalcula rutas en tiempo real.
- Sincroniza overlays, escena actual y UI runtime.
- Aplica transiciones entre escenas sin cargar logica en `main.cpp`.

## Estado Funcional

| Capacidad | Estado |
| --- | --- |
| Menu de inicio interactivo | Operativo |
| Intro animada entre escenas | Operativa |
| Cambio a gameplay | Operativo |
| Rutas BFS / DFS / perfiladas | Operativas |
| Bloqueos y resiliencia | Operativos |
| Cambio entre pisos / escenas | Operativo segun configuracion actual |
| Build Debug | Verificada |
| Build Release | Verificada |

## Archivos Relevantes para Entrega

| Archivo | Rol |
| --- | --- |
| `src/main.cpp` | punto de entrada minimo |
| `src/core/application/ApplicationSession.*` | coordinacion principal |
| `src/runtime/StartMenuController.*` | menu e intro |
| `src/runtime/GameplayLoopController.*` | loop de juego |
| `README.md` | presentacion para GitHub |
| `.gitignore` | limpieza del repositorio |

## Build y Entrega

| Preset | Directorio |
| --- | --- |
| `debug` | `build-debug/` |
| `release` | `build/` |

Nota: `CMakePresets.json` ya incluye `configuration` explicita para `Debug` y `Release`, necesaria en generadores multi-config como Visual Studio para que la salida de produccion vaya realmente a `build/Release/`.

Comandos esperados:

```bash
cmake --preset debug
cmake --build --preset debug
cmake --preset release
cmake --build --preset release
```

## Observaciones

- El archivo `campus.generated.json` se considera artefacto generado del runtime.
- `imgui.ini` y carpetas de IDE no forman parte de la entrega final.
- La estructura actual esta preparada para seguir extrayendo modulos si se desea dividir mas el bootstrap.
