# Briefing for other Grok bots (updated 2026-09-21)

This file is the shared memory of the main Ravelin thread. Other bots cannot see that chat. Read this plus AGENTS.md and COMPILE_RULES.txt before editing.

## Identity
- Public name: RAVELIN
- Internal C++ module: Aetherline (do not rename)
- Engine: Unreal Engine 5.8.2
- Repo: kam11s/AETHERLINE-UnrealEngine5-HeroShooter
- Do not use kam11s/Aetherline (old GAS / Enhanced Input repo)

## What works on main
- Project compiles. Play works.
- FP gun, teal tracers, HUD (RAVELIN, health, ult, score).
- Night teal/amber yard spawned at Play by ALGameMode / ALArenaBuilder.
- Desktop shortcut RAVELIN opens the editor project, not a Steam exe.
- main HEAD is the compile-safe spine. Do not fast-forward it to a cursor/* branch without the main thread.

## Cursor draft PRs (DO NOT MERGE THE STACK)
Open draft PRs #1-#8 on branches named cursor/*. They are notes + unmerged C++.
#8 is based on #7, not main. #5 (live AI bots) is the first candidate if we merge one.
#5 Build.cs was checked: unchanged. No GAS / Enhanced Input.
Those PRs were not compiled in UE 5.8 by the agent that wrote them.

## What is local only (not on GitHub main)
- RavelinYard.umap and dressed gun/map work on the owner's PC.
- Untitled tab in the editor is normal; Play rebuilds the yard from C++.
- HOSTILES 5 / shaped gun the owner already plays may be local + cursor branches, not main.
- Do not tell the owner to delete D:\\Aetherline to "sync."
- Do not git checkout cursor/* over the live project without a copy.

## Scope
Order: feel → place → loop → package → store → live.
Publisher is a later goal after a packaged Windows slice friends can run.
Finish skirmish rematch before shop / battle pass / Worldpay / 21-player BR.

## Compile lock
- BuildSettingsVersion.V7 both Target.cs
- Build.cs modules only: Core, CoreUObject, Engine, InputCore, UMG, AIModule
- Legacy input only (DefaultInput.ini). No Enhanced Input. No GAS.
- Named UFUNCTION params. Keep EALDropPhase and ALGameState.h.

## How work merges
Editor/art can stay on disk. Source/Config changes: one PR at a time, reviewed in the main thread, then merge.
