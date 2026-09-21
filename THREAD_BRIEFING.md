# Briefing for other Grok bots (updated 2026-09-21 evening)

Read AGENTS.md and COMPILE_RULES.txt first. This file is the source of truth from the main Grok thread. Owner will point you here.

## Identity
- Public name: RAVELIN (in-game title too)
- C++ module: Aetherline (do not rename)
- Engine: UE 5.8.2 only
- Repo: kam11s/AETHERLINE-UnrealEngine5-HeroShooter
- Ignore kam11s/Aetherline (GAS / Enhanced Input graveyard)

## Owner status (2026-09-21 evening)
Owner is home, worked with the group, then watched TV. He asked if he is still doing fine: **yes**. Loop works. Graphics are nowhere near concept art. That gap is expected. Do not panic or restart the project.

## Order of work
feel → place → loop → package → store → live
Publisher only after a packaged Windows slice. Years is acceptable. Quality over live-service.

**Now:** loop is in good shape locally. Next real product work is making **one Play still** closer to the control-point concept, not new modes.

## main
Compile-safe spine + rematch stub.
Do not merge cursor/* into main without the main thread.
Do not git pull / checkout over D:\\Aetherline while Unreal is open.

## Cursor draft stack (do not merge)
- #1 FP gun/camera feel
- #2 FP gun silhouette
- #3 industrial night lighting
- #4 dense industrial yard
- #5 live AI bots (HOSTILES 5 locally)
- #6 carbine full-auto + crouch (dropped HandleDeath on that commit)
- #7 crouch blend
- #8 recoil + aim assist (header on this branch has half climb + RespawnDelay 2s; cpp body of that retune is local)
- #9 NEW 2026-09-21 evening: BotSkirmish score loop on **main** (kills score, DRAW, Play Again Enter/Start, 2s respawn, ScoreToWin 25). Draft. Build.cs clean. Not compiled in 5.8 by the authoring agent. Do not stack #9 onto #5–#8.

Local Play already has bots, night yard, carbine, ~2s respawn, half recoil climb, and a working loop. That folder is ahead of GitHub.

## Playtest 2026-09-21 (local)
- Fire stopping was 0 HP, not ammo.
- Recoil climb too hot; ~half RecoilPitchLightDeg / RecoilClimbMaxDeg felt good.
- Owner: could aim, die, respawn.
- Later tonight: loop works; graphics still not there.

## Graphics gap (read GRAPHICS_GAP.md)
Concept art is a finished cinematic / FPS frame. Play is a teal/amber blockout. Do not restyle. Do not start a second project. One map: RavelinYard. Steal the **gameplay** concept frame first (wet floor, cover, gantries, teal ring, real viewmodel), not the wide key art.

## Compile lock
No GAS. No Enhanced Input. BuildSettings V7. Named UFUNCTION params. No inline UFUNCTION bodies. HUD helpers not const. Allowed modules only: Core, CoreUObject, Engine, InputCore, UMG, AIModule.

## What not to do
- Shop, battle pass, Worldpay, 21-player BR, publisher email.
- Merge the stack to "get graphics."
- Change art direction away from night teal / amber industrial.
