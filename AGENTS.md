# AETHERLINE — agent / bot rules

This Unreal Engine **5.8.2** C++ project compiles on the owner's machine. Do not "upgrade" the module graph. Read this before any edit.

## Engine
- Target: Unreal Engine 5.8.2 only.
- `Aetherline.uproject` EngineAssociation must stay `"5.8"`.
- Both target files must keep:
  `DefaultBuildSettings = BuildSettingsVersion.V7;`
- Do not set V5, Latest, or Unique unless the owner is on a new engine version.

## Modules (Aetherline.Build.cs)
Allowed PublicDependencyModuleNames only:
`Core`, `CoreUObject`, `Engine`, `InputCore`, `UMG`, `AIModule`

**Do not add**
- GameplayAbilities / GameplayTasks / GameplayTags
- EnhancedInput
- HTTP / JSON / WebSockets
- Niagara (code module) unless the owner asks and it compiles on their box
- OnlineSubsystem / EOS / Steam in Build.cs until a dedicated pass

## Input
- Legacy `UInputComponent` + `Config/DefaultInput.ini` Axis/Action mappings only.
- DefaultPlayerInputClass must stay `/Script/Engine.PlayerInput`.
- Do not switch to Enhanced Input.

## UHT rules (these already broke the build once)
- Every `UFUNCTION` parameter must have a name. Never `const FString&`.
- Do not put `{ ... }` bodies on `UFUNCTION`s in headers.
- Do not mark HUD helpers `const` if they call `DrawRect` / `DrawLine` / `DrawText`.
- If you add an enum used by a UPROPERTY, put it in `ALTypes.h` as `UENUM` *before* any header uses it.
- Required enums in `ALTypes.h`: `EALHero`, `EALPlaylist` (must include `BattleRoyale`), `EALDropPhase`, `EALTeam`.
- `ALGameState.h` must stay in the module. HUD includes it.

## Files you must not delete
- `Source/Aetherline/ALTypes.h`
- `Source/Aetherline/ALGameState.h` + `.cpp`
- `Source/Aetherline/ALLoadGate.h` + `.cpp`
- `Source/Aetherline.Target.cs`
- `Source/AetherlineEditor.Target.cs`
- `Source/Aetherline/Aetherline.Build.cs`
- `Aetherline.uproject`
- `Config/DefaultEngine.ini`
- `Config/DefaultInput.ini`

## How to change things safely
1. Prefer edits in existing `.cpp` files.
2. New `UCLASS` / `USTRUCT` / `UENUM` need matching `.generated.h` includes and a `.cpp` that `#include`s the header.
3. After source edits the owner rebuilds by opening `Aetherline.uproject` (Yes).
4. If compile fails, the useful line is `OtherCompilationError` / `fatal error` / `Error:` on an `AL*.h` file — not `aqProf.dll` / `VtuneApi.dll`.
5. Never tell the owner to enable GAS or Enhanced Input to "fix" a compile.

## Look lock
- Teal + amber industrial greybox. Do not restyle to a different art direction.
- Arena is spawned at Play by `AALArenaBuilder`. Do not require the owner to place actors.
- Concept shots are the target look; this repo is the playable C++ game. Improve `ALArenaBuilder` and lighting. Do not start a second project.

## Repo hygiene
- Do not commit `Binaries/`, `Intermediate/`, `Saved/`, or `.sln`.
- Do not touch other GitHub repos. This repo only: `kam11s/AETHERLINE-UnrealEngine5-HeroShooter`.
