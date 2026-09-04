# Ariba Engine

**Ariba Engine** is a personal game engine written in **C++**, built from the ground up with a focus on understanding and implementing the core systems behind a game engine.

The project is currently in active development and serves as a foundation for experimenting with engine architecture, rendering, game states, UI, asset management and other low-level game development systems.

> **Status:** Early development

---

## Features

### Core

* C++ game engine architecture
* Application and engine context
* Game state management
* Asset management
* Logging system
* Modular engine structure

### Graphics

* [SFML 3](https://github.com/SFML/SFML) based rendering
* Window management
* 2D rendering
* Text rendering
* Texture and font assets

### UI

* Modular button system
* Interactive buttons
* Mouse interaction
* Hover animations
* Configurable horizontal and vertical alignment
* Position offsets
* Custom button actions
* Extensible UI components

### Build System

* CMake
* CMake Presets
* Ninja
* vcpkg
* Cross-platform configuration
* Debug and Release configurations

---

## Architecture

The engine is structured around independent systems that can progressively be expanded as the project evolves.

```text
Ariba Engine
│
├── Engine
│   └── Core
│       ├── Application
│       ├── Context
│       ├── State Manager
│       ├── Asset Cache
│       └── Logger
│
├── Game
│   └── States
│       ├── Menu
│       └── InGame
│
└── Resources
    └── Common
        └── Fonts / Assets
```

The goal is to keep the **engine layer** independent from the game-specific layer whenever possible.

This makes it possible to reuse the engine systems across different projects without tightly coupling them to a particular game.

---

## UI System

One of the current focuses of Ariba Engine is a modular UI system.

Buttons can be configured using horizontal and vertical alignment:

```cpp
HAlign::Left
HAlign::Center
HAlign::Right
```

and:

```cpp
VAlign::Top
VAlign::Center
VAlign::Bottom
```

They can also receive positional offsets and custom callbacks.

Example:

```cpp
LabelButton(
    "Play",
    font,
    sf::Vector2f{},
    ButtonSize,
    HAlign::Center,
    VAlign::Center,
    0.f,
    -60.f,
    [] {
        // Play action
    }
);
```

The objective is to build a flexible UI system capable of supporting menus, HUDs, settings screens and other interfaces without hard-coding their layout.

---

## Requirements

### Required

* **C++ compiler**
* **CMake 3.28+**
* **Ninja**
* **vcpkg**
* **SFML 3**
* Git

The project currently uses SFML 3 with the following modules:

* Graphics
* Window
* System
* Audio

SFML is discovered through CMake using:

```cmake
find_package(SFML 3 CONFIG REQUIRED COMPONENTS
    Graphics
    Window
    System
    Audio
)
```

---

## Building

Clone the repository:

```bash
git clone https://github.com/LilAkai/ariba_engine.git
cd ariba_engine
```

### macOS

The repository provides dedicated CMake presets for macOS.

Debug:

```bash
cmake --preset macos-debug
cmake --build --preset macos-debug
```

Release:

```bash
cmake --preset macos-release
cmake --build --preset macos-release
```

The macOS presets are configured for Ninja and vcpkg using the `arm64-osx` triplet.

### Windows

Windows presets are also provided for MinGW + vcpkg.

Debug:

```bash
cmake --preset windows-debug
cmake --build --preset windows-debug
```

Release:

```bash
cmake --preset windows-release
cmake --build --preset windows-release
```

The current Windows presets use Ninja, MinGW and the `x64-mingw-static` vcpkg triplet.

> The paths to vcpkg, MinGW and Ninja in `CMakePresets.json` are currently machine-specific. They may need to be changed when building on another Windows machine.

---

## Project Structure

```text
ariba_engine/
│
├── resources/
│   └── Common/
│       └── default.otf
│
├── src/
│   ├── Engine/
│   │   └── Core/
│   │
│   └── main.cpp
│
├── .gitattributes
├── .gitignore
├── CMakeLists.txt
├── CMakePresets.json
└── README.md
```

Resources are copied next to the executable during the build so that the engine can access them at runtime.

---

## Design Goals

Ariba Engine is primarily an **educational and experimental engine project**.

The main goals are:

* Learn how modern game engines are structured
* Build engine systems instead of relying entirely on existing frameworks
* Keep systems modular and reusable
* Experiment with C++ architecture and memory management
* Develop a flexible UI system
* Understand rendering and resource management
* Maintain a clean and portable build system
* Gradually evolve the engine toward a usable game development framework

The project prioritizes **understanding and control over abstraction**.

---

## Roadmap

The engine is still in its early stages. Planned systems include:

* [ ] Improved entity/component architecture
* [ ] Scene management
* [ ] Entity Component System (ECS)
* [ ] Improved rendering abstraction
* [ ] Sprite and animation systems
* [ ] Camera system
* [ ] Input management
* [ ] Audio management
* [ ] UI containers and layout system
* [ ] UI panels and additional widgets
* [ ] Configuration system
* [ ] Serialization
* [ ] Debug tools
* [ ] Editor tools
* [ ] Better cross-platform build configuration

The roadmap will evolve alongside the engine.

---

## Technologies

| Technology        | Purpose                          |
| ----------------- | -------------------------------- |
| **C++**           | Engine development               |
| **SFML 3**        | Windowing, graphics and audio    |
| **CMake**         | Build system                     |
| **CMake Presets** | Platform-specific configurations |
| **Ninja**         | Build generation                 |
| **vcpkg**         | Dependency management            |
| **Git**           | Version control                  |

---

## Why Ariba Engine?

Ariba Engine is not intended to compete with established engines such as Unity or Unreal Engine.

It is a **from-scratch learning project** designed to explore what happens underneath a game engine and to progressively build a reusable C++ foundation.

The project will continue to evolve as new engine systems are implemented and refined.

---

## License

License information will be added as the project matures.

```

Le README est volontairement formulé pour être **crédible sur GitHub aujourd’hui** : il distingue ce qui existe déjà de la roadmap, au lieu de présenter Ariba Engine comme un moteur complet. Ton dépôt est actuellement public et ne possède pas encore de description ni de topics GitHub, donc je te recommande aussi de mettre une courte description du type **“Modular C++ game engine built from scratch with SFML”** et d’ajouter les topics `cpp`, `cpp20`, `game-engine`, `gamedev`, `sfml`, `cmake`, `vcpkg`.

[Voir le dépôt Ariba Engine](https://github.com/LilAkai/ariba_engine?utm_source=chatgpt.com)
```
