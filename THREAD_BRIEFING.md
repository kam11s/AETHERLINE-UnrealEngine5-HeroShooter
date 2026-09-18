# Briefing for other Grok bots (2026-09-18)

This file is the shared memory of the main Ravelin thread. Other bots cannot see that chat. Read this plus AGENTS.md and COMPILE_RULES.txt before editing.

## Identity
- Public name: RAVELIN
- Internal C++ module: Aetherline (do not rename)
- Engine: Unreal Engine 5.8.2
- Repo: kam11s/AETHERLINE-UnrealEngine5-HeroShooter
- Do not use kam11s/Aetherline (old GAS / Enhanced Input repo)

## What works
- Project compiles. Play works.
- FP gun, teal tracers, HUD (RAVELIN, health, ult, score).
- Night teal/amber yard spawned at Play by ALGameMode / ALArenaBuilder.
- Desktop shortcut RAVELIN opens the editor project, not a Steam exe.

## What is local only (not on GitHub)
- RavelinYard.umap and any dressed gun/map work on the owner's PC.
- Untitled tab in the editor is normal; Play rebuilds the yard from C++.
- Do not tell the owner to delete D:\\Aetherline to "sync."

## Scope (years of quality, not fake live service)
Order: feel → place → loop → package → store → live.
Finish skirmish match loop (score to win, rematch) before shop, battle pass, or 21-player BR.

## Compile lock
- BuildSettingsVersion.V7 both Target.cs
- Build.cs modules only: Core, CoreUObject, Engine, InputCore, UMG, AIModule
- Legacy input only (DefaultInput.ini). No Enhanced Input. No GAS.
- Named UFUNCTION params. Keep EALDropPhase and ALGameState.h.

## How work merges
Editor/art can stay on disk. Anything that touches Source/ or Config/ must be reviewed in the main thread before it is treated as safe on main.
