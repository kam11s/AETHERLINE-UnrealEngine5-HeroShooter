# Briefing for other Grok bots (updated 2026-09-22)

Read AGENTS.md and COMPILE_RULES.txt first. This file is the source of truth from the main Grok thread. Owner will point you here.

## Identity
- Public name: RAVELIN (in-game title too)
- C++ module: Aetherline (do not rename)
- Engine: UE 5.8.2 only
- Repo: kam11s/AETHERLINE-UnrealEngine5-HeroShooter
- Ignore kam11s/Aetherline (GAS / Enhanced Input graveyard)

## ALArenaBuilder — do not wipe (2026-09-22)
Owner said bots accidentally wiped ALArenaBuilder locally last night.
**GitHub main still has the real file.** Do not delete it. Do not empty BeginPlay.

- `Source/Aetherline/ALArenaBuilder.h`
- `Source/Aetherline/ALArenaBuilder.cpp` (~5k, teal floor, 12 cover blocks, walls, lamps)
- `AALGameMode::SpawnArenaIfMissing()` spawns it at Play if none exists

If the owner's D:\\Aetherline copy is missing those files: copy them from main. Do not rewrite from scratch. Do not merge cursor/* to "fix" it.

## Owner status
Loop works. Graphics lag the locked mockups (settings / arena picker / training range / hero select / battle pass / dropship). Those mockups are the UI bible; public title on them should read RAVELIN. Years is OK. Not pointless.

Next product doors: 2-player Listen Server in PIE, then one friend's packaged exe. Not Steam yet.

## Order of work
feel → place → loop → package → store → live
Publisher only after a packaged Windows slice.

## main
Compile-safe spine + rematch stub + intact ALArenaBuilder.
Do not merge cursor/* into main without the main thread.
Do not git pull / checkout over D:\\Aetherline while Unreal is open.

## Cursor draft stack (do not merge)
#1–#8 gun/feel/yard/bots. #9 score loop draft. Do not stack.

## Compile lock
No GAS. No Enhanced Input. BuildSettings V7. Named UFUNCTION params. Allowed modules only: Core, CoreUObject, Engine, InputCore, UMG, AIModule.

## What not to do
- Delete or gut ALArenaBuilder / SpawnArenaIfMissing
- Shop, Worldpay, 21-player BR, publisher email
- Merge the stack to get graphics
- Change art direction away from night teal / amber industrial
