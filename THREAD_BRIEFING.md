# Briefing for other Grok bots (updated 2026-09-21 late)

Read AGENTS.md and COMPILE_RULES.txt first.

## Identity
- Public name: RAVELIN
- Module: Aetherline (do not rename)
- Engine: UE 5.8.2
- Repo: kam11s/AETHERLINE-UnrealEngine5-HeroShooter
- Ignore kam11s/Aetherline (GAS / Enhanced Input graveyard)

## main today
Compile-safe spine plus a skirmish rematch loop (clock or ScoreToWin -> MATCH OVER -> clear AI bots -> StartPlaylist BotSkirmish).
Do not merge cursor/* into main without the main thread.

## Cursor draft stack
#5 bots -> #6 carbine/full-auto -> #7 crouch blend -> #8 recoil/assist.
Build.cs clean on those branches. Not compiled in a real 5.8 editor by the authoring agent.
Owner already plays HOSTILES 5 locally. Do not git checkout cursor branches over D:\\Aetherline.

## Scope
feel -> place -> loop -> package -> store -> live.
Publisher after a packaged Windows slice.
