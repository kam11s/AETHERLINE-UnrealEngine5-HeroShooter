# Merge plan (2026-09-21)

Do not merge the Cursor stack in one click.

## Stack (each PR targets the previous branch, not main)
1. #5 live-ai-bots  (base: older main)
2. #6 carbine + full-auto + crouch  (base: #5)
3. #7 crouch blend  (base: #6)
4. #8 recoil + aim assist  (base: #7)
PRs #1-#4 are earlier slices of the same work. Treat #5-#8 as the live stack.

## Compile-lock check already done
Build.cs on #5, #6, #8 is unchanged: Core, CoreUObject, Engine, InputCore, UMG, AIModule.
No GAS. No Enhanced Input.
#8 also fixes 5.8 TObjectPtr .Get() and Mesh name shadowing.
#8 notes HandleDeath/Respawn were dropped on #6 — 0 HP may freeze fire until that is restored.

## What landed on main today (this thread)
Skirmish rematch: clock or ScoreToWin ends the match, HUD says MATCH OVER, bots are cleared, a new BotSkirmish starts after RematchDelaySeconds (6).
Does not pull Cursor branches. Owner should not git pull over D:\\Aetherline while Unreal is open.

## When Kam is home
1. Close Unreal.
2. Optional: copy the project folder.
3. If Play already has HOSTILES 5, you already have #5 locally — do not merge #5 onto that machine tonight.
4. First merge candidate later: #5 only, then Play, then stop.
