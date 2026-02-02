# Documentation Index

Complete guide to all project documentation for the XnGine Engine Framework.

## Quick Start

**New to the project?** Start here:
1. Read [PROJECT_STATUS_DASHBOARD.md](PROJECT_STATUS_DASHBOARD.md) (5 min overview)
2. Read [PHASE2_COMPLETION_SUMMARY.md](PHASE2_COMPLETION_SUMMARY.md) (full context)
3. Review [XNGINE_GAMES_REFERENCE.md](XNGINE_GAMES_REFERENCE.md) for game info

**Want to create a new game module?** Start here:
1. Read [QUICK_REFERENCE_GAME_PLUGINS.md](QUICK_REFERENCE_GAME_PLUGINS.md) (template)
2. Review [src/games/README.md](src/games/README.md) (architecture guide)
3. Use [XNGINE_GAME_MODULE_CHECKLIST.md](XNGINE_GAME_MODULE_CHECKLIST.md) (tracking)

---

## Documentation by Category

### Project Status & Overview

| Document | Purpose | Length | Audience |
|----------|---------|--------|----------|
| [PROJECT_STATUS_DASHBOARD.md](PROJECT_STATUS_DASHBOARD.md) | Current project status, metrics, blockers | 400 lines | Everyone |
| [PHASE2_COMPLETION_SUMMARY.md](PHASE2_COMPLETION_SUMMARY.md) | Executive summary of Phase 2 work | 450 lines | Technical leads |
| [SESSION_SUMMARY_REDGUARD_MODULE.md](SESSION_SUMMARY_REDGUARD_MODULE.md) | This session's work summary | 350 lines | Project managers |

### Game-Specific Information

| Document | Purpose | Length | Audience |
|----------|---------|--------|----------|
| [XNGINE_GAMES_REFERENCE.md](XNGINE_GAMES_REFERENCE.md) | Complete reference for all 10 XnGine games | 280 lines | Everyone |
| [XNGINE_GAME_MODULE_CHECKLIST.md](XNGINE_GAME_MODULE_CHECKLIST.md) | Detailed checklists for each game module | 350 lines | Developers |
| [PHASE2B_REDGUARD_PER_GAME_MODULE.md](PHASE2B_REDGUARD_PER_GAME_MODULE.md) | Detailed Redguard module documentation | 300 lines | Developers |

### Developer Guides & References

| Document | Purpose | Length | Audience |
|----------|---------|--------|----------|
| [QUICK_REFERENCE_GAME_PLUGINS.md](QUICK_REFERENCE_GAME_PLUGINS.md) | Quick start for creating game modules | 400 lines | Developers |
| [src/games/README.md](src/games/README.md) | Architecture guide and patterns | 200 lines | Developers |
| [MODFORMAT.md](MODFORMAT.md) | Mod format specification (Type 1 & 2) | 200 lines | Developers |

### Original Project Documentation

| Document | Purpose | Status |
|----------|---------|--------|
| [README.md](README.md) | Original plugin overview | Current |
| [REFACTOR_PLAN_XnGine.md](REFACTOR_PLAN_XnGine.md) | Phase 1 refactoring plan | Complete |
| [XN_GINE_ENGINE_VS_GAME_CLASSIFICATION.md](XN_GINE_ENGINE_VS_GAME_CLASSIFICATION.md) | Phase 1 engine analysis | Complete |

---

## Key Information Quick Links

### Nexus Game IDs (For Setting Up Modules)
- **Redguard**: Game 4462, MO 6220 ✅
- **Daggerfall**: Game 232, MO ? 🟡
- **Arena**: Game 3, MO ? 🟡
- **Battlespire**: Game 1788, MO ? 🟡
- **Others**: See [XNGINE_GAMES_REFERENCE.md](XNGINE_GAMES_REFERENCE.md)

### Game Detection Paths

**Redguard:**
- Steam: `The Elder Scrolls Adventures Redguard/DOSBox-0.73/`
- GOG: `Redguard/DOSBOX/`

**Daggerfall:**
- Steam: `The Elder Scrolls Daggerfall/DOSBox-0.74/`
- GOG: `Daggerfall/DOSBox-0.74/`

**Arena:**
- Steam: `The Elder Scrolls Arena/DOSBox-0.74/`
- GOG: `Arena/DOSBox-0.74/`

**Battlespire:**
- GOG: `An Elder Scrolls Legend Battlespire/DOSBOX/` (no Steam)

For full details: [XNGINE_GAMES_REFERENCE.md](XNGINE_GAMES_REFERENCE.md#game-module-implementations)

### File Naming Convention

All game modules follow this pattern:

```
src/games/<game>/
├── game<game>.h                  # Game plugin header
├── game<game>.cpp                # Implementation
├── game<game>.json               # Metadata
├── <game>smoddatachecker.h       # Mod validator
├── <game>smoddatacontent.h/cpp   # Content categorizer
├── <game>savegame.h/cpp          # Save handler
└── CMakeLists.txt                # Build config
```

See: [QUICK_REFERENCE_GAME_PLUGINS.md](QUICK_REFERENCE_GAME_PLUGINS.md#file-naming-convention-verification)

### Class Inheritance Hierarchy

```
MOBase::IPluginGame
    ↓
GameXngine (xngine/)
    ├── GameRedguard (games/redguard/) ✅
    ├── GameDaggerfall (games/daggerfall/) 🟡
    ├── GameArena (games/arena/) 🟡
    └── GameBattlespire (games/battlespire/) 🟡
```

See: [PHASE2_COMPLETION_SUMMARY.md](PHASE2_COMPLETION_SUMMARY.md#architecture-overview)

---

## How to Use This Documentation

### For Project Managers
1. Check [PROJECT_STATUS_DASHBOARD.md](PROJECT_STATUS_DASHBOARD.md) for current status
2. Review [SESSION_SUMMARY_REDGUARD_MODULE.md](SESSION_SUMMARY_REDGUARD_MODULE.md) for recent work
3. Check [XNGINE_GAME_MODULE_CHECKLIST.md](XNGINE_GAME_MODULE_CHECKLIST.md) for progress on other games

### For Developers (Creating Game Modules)
1. Start with [QUICK_REFERENCE_GAME_PLUGINS.md](QUICK_REFERENCE_GAME_PLUGINS.md)
2. Reference [src/games/README.md](src/games/README.md) for architecture details
3. Use [Redguard module](src/games/redguard/) as implementation template
4. Track progress in [XNGINE_GAME_MODULE_CHECKLIST.md](XNGINE_GAME_MODULE_CHECKLIST.md)
5. Consult [XNGINE_GAMES_REFERENCE.md](XNGINE_GAMES_REFERENCE.md) for game-specific details

### For QA/Testing
1. Check [PROJECT_STATUS_DASHBOARD.md](PROJECT_STATUS_DASHBOARD.md#compilation--testing-status)
2. Follow [XNGINE_GAME_MODULE_CHECKLIST.md](XNGINE_GAME_MODULE_CHECKLIST.md#testing-matrix)
3. Refer to [PHASE2B_REDGUARD_PER_GAME_MODULE.md](PHASE2B_REDGUARD_PER_GAME_MODULE.md#testing-checklist)

### For Architecture Review
1. Read [PHASE2_COMPLETION_SUMMARY.md](PHASE2_COMPLETION_SUMMARY.md)
2. Review [src/games/README.md](src/games/README.md) for pattern justification
3. Check [QUICK_REFERENCE_GAME_PLUGINS.md](QUICK_REFERENCE_GAME_PLUGINS.md#why-this-structure)

---

## Document Purposes

### PROJECT_STATUS_DASHBOARD.md
- **What:** Current project metrics and status
- **When to read:** Daily/weekly status checks
- **Key sections:**
  - Overall project state (Phase 2 complete, Phase 3 in progress)
  - Detailed status by component
  - Success criteria verification
  - Known issues and blockers
  - Recommendations for next phase

### PHASE2_COMPLETION_SUMMARY.md
- **What:** Comprehensive summary of completed work
- **When to read:** Understanding project history and decisions
- **Key sections:**
  - Architecture overview with diagrams
  - Detailed work completed in Phase 2A and 2B
  - File naming conventions
  - Design decisions and rationale
  - Integration with existing code

### SESSION_SUMMARY_REDGUARD_MODULE.md
- **What:** What was done in this specific session
- **When to read:** Understanding recent changes
- **Key sections:**
  - Files created
  - Achievements
  - Integration points
  - Next steps

### XNGINE_GAMES_REFERENCE.md
- **What:** Complete reference for all 10 XnGine games
- **When to read:** Planning new game modules
- **Key sections:**
  - All game info with Nexus IDs
  - Game paths (Steam/GOG)
  - Save directory structures
  - Mod format specifications
  - Detection markers

### QUICK_REFERENCE_GAME_PLUGINS.md
- **What:** Quick start guide for developers
- **When to read:** Creating a new game module
- **Key sections:**
  - Template checklist
  - File naming convention
  - Implementation guidelines
  - Code patterns
  - Debugging tips

### src/games/README.md
- **What:** Architecture guide and design patterns
- **When to read:** Deep understanding of module structure
- **Key sections:**
  - File structure pattern
  - Implementation guidelines
  - Class hierarchies
  - Building instructions
  - Game-specific notes

### XNGINE_GAME_MODULE_CHECKLIST.md
- **What:** Detailed checklists for each game module
- **When to read:** Creating modules or tracking progress
- **Key sections:**
  - Checklist for each game (Daggerfall, Arena, Battlespire)
  - Pre-implementation research items
  - Implementation tasks
  - Testing procedures
  - Build system updates

### PHASE2B_REDGUARD_PER_GAME_MODULE.md
- **What:** Detailed documentation of Redguard module
- **When to read:** Understanding how to implement game modules
- **Key sections:**
  - Files created with purposes
  - Architecture explanation
  - Game detection logic
  - Save mapping
  - Testing checklist

---

## Architecture Decision Records (ADR)

### ADR-1: Use Gamebryo/Skyrim Pattern
- **Decision:** Adopt file naming and class structure from modorganizer-game_bethesda
- **Rationale:** Proven pattern, consistent with MO2 ecosystem
- **Location:** [PHASE2_COMPLETION_SUMMARY.md](PHASE2_COMPLETION_SUMMARY.md#why-gamebryo-pattern)

### ADR-2: Per-Game Modules (Not Monolithic)
- **Decision:** Each game gets its own DLL (game_redguard.dll, game_daggerfall.dll, etc.)
- **Rationale:** Clean separation, parallel compilation, independent testing
- **Location:** [PHASE2_COMPLETION_SUMMARY.md](PHASE2_COMPLETION_SUMMARY.md#why-per-game-modules)

### ADR-3: Minimal Game-Specific Classes
- **Decision:** Game classes only override what's unique, inherit rest from GameXngine
- **Rationale:** DRY principle, maintainability, leverage inheritance
- **Location:** [PHASE2_COMPLETION_SUMMARY.md](PHASE2_COMPLETION_SUMMARY.md#why-minimal-game-specific-classes)

---

## Version History

| Date | Phase | Status | Key Changes |
|------|-------|--------|-------------|
| Phase 1 | Analysis | ✅ Complete | Engine vs. game classification, install scan |
| Phase 2A | Extraction | ✅ Complete | XnGine core refactoring, 18 patches |
| Phase 2B | Redguard | ✅ Complete | Per-game module created, documentation |
| Phase 3 | Other Games | 🟡 In Progress | Daggerfall, Arena, Battlespire planned |

---

## Frequently Asked Questions (FAQ)

**Q: Where do I start if I'm new to the project?**  
A: Read [PROJECT_STATUS_DASHBOARD.md](PROJECT_STATUS_DASHBOARD.md) first (5 min), then [PHASE2_COMPLETION_SUMMARY.md](PHASE2_COMPLETION_SUMMARY.md) for full context.

**Q: How do I create a new game module?**  
A: Follow [QUICK_REFERENCE_GAME_PLUGINS.md](QUICK_REFERENCE_GAME_PLUGINS.md) checklist and use [Redguard module](src/games/redguard/) as template.

**Q: What are the Nexus IDs for each game?**  
A: See [XNGINE_GAMES_REFERENCE.md](XNGINE_GAMES_REFERENCE.md#all-xngine-games-1996-1999)

**Q: What is the file naming convention?**  
A: `game<game>.h`, `<game>smoddatachecker.h`, etc. Details in [QUICK_REFERENCE_GAME_PLUGINS.md](QUICK_REFERENCE_GAME_PLUGINS.md#file-naming-convention-verification)

**Q: Why inherit from GameXngine instead of IPluginGame?**  
A: GameXngine provides common XnGine functionality. Details in [src/games/README.md](src/games/README.md#integration-with-xngine-core)

**Q: Where is the Redguard module code?**  
A: `src/games/redguard/` with 9 files. See [SESSION_SUMMARY_REDGUARD_MODULE.md](SESSION_SUMMARY_REDGUARD_MODULE.md#files-created-this-session)

**Q: What's the current project status?**  
A: Phase 2 complete, Phase 3 (Daggerfall/Arena/Battlespire) in progress. See [PROJECT_STATUS_DASHBOARD.md](PROJECT_STATUS_DASHBOARD.md)

---

## Related Files (Not Documentation)

### Source Code
- `src/xngine/` - Engine-level code (12 core classes)
- `src/games/redguard/` - Redguard game module (9 files)
- `src/games/daggerfall/` - Empty (planned)
- `src/games/arena/` - Empty (planned)
- `src/games/battlespire/` - Empty (planned)

### Configuration
- `CMakeLists.txt` - Main build config (update to include games/)
- `build_ms.bat` - Build script
- `.gitignore`, `FindMO2.ps1`, etc. - Setup files

### Original Code
- `src/gameredguard.cpp/h` - Original Redguard implementation (reference)
- `src/RGMODFrameworkWrapper.*` - Advanced mod loading (reference)
- `src/RedguardDataChecker.*` - Original mod validator (reference)

---

## Checklist for Reading Documentation

### Essential Reading (Everyone)
- [ ] [PROJECT_STATUS_DASHBOARD.md](PROJECT_STATUS_DASHBOARD.md)
- [ ] [XNGINE_GAMES_REFERENCE.md](XNGINE_GAMES_REFERENCE.md)

### Developer Reading
- [ ] [QUICK_REFERENCE_GAME_PLUGINS.md](QUICK_REFERENCE_GAME_PLUGINS.md)
- [ ] [src/games/README.md](src/games/README.md)
- [ ] [PHASE2B_REDGUARD_PER_GAME_MODULE.md](PHASE2B_REDGUARD_PER_GAME_MODULE.md)
- [ ] [XNGINE_GAME_MODULE_CHECKLIST.md](XNGINE_GAME_MODULE_CHECKLIST.md)

### Deep Dive (For Understanding)
- [ ] [PHASE2_COMPLETION_SUMMARY.md](PHASE2_COMPLETION_SUMMARY.md)
- [ ] [REFACTOR_PLAN_XnGine.md](REFACTOR_PLAN_XnGine.md)
- [ ] [XN_GINE_ENGINE_VS_GAME_CLASSIFICATION.md](XN_GINE_ENGINE_VS_GAME_CLASSIFICATION.md)

---

**Last Updated:** February 2, 2026  
**Total Documentation:** ~2500 lines across 10+ files  
**Format:** Markdown  
**Status:** Complete for Phase 2, ready for Phase 3
