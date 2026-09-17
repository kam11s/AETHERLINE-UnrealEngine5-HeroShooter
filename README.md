# AETHERLINE (Unreal Engine 5)

**Find this repo:** [kam11s/AETHERLINE-UnrealEngine5-HeroShooter](https://github.com/kam11s/AETHERLINE-UnrealEngine5-HeroShooter)

First-person F2P hero shooter source for Unreal Engine 5.5+.
This is not a packaged `.exe`. Open `Aetherline.uproject` in the Unreal Editor, compile, press Play.

This repository is standalone. It does not modify your other GitHub repos.

## Open and play

1. Install Epic Games Launcher and Unreal Engine 5.5 (5.6/5.8 also fine).
2. Green **Code** button → **Download ZIP**, or `git clone https://github.com/kam11s/AETHERLINE-UnrealEngine5-HeroShooter.git`
3. Double-click `Aetherline.uproject` and let it compile.
4. File → New Level → Empty Level. Place `ALArenaBuilder` and a `PlayerStart`. Save as `Content/Maps/L_OrionSpire`.
5. Project Settings → Maps & Modes → Game Default Map = `L_OrionSpire`, GameMode Override = `ALGameMode`.
6. Plug in any controller, then press Play.

## Controls

Keyboard: WASD, mouse look, LMB fire, Space jump, Q/E abilities, F ult, 1-6 hero.

Any controller (Xbox, PlayStation, Switch, Steam Input):
- Left stick move
- Right stick look
- Right trigger fire
- A / Cross jump
- LB / L1 ability Q
- RB / R1 ability E
- Y / Triangle ultimate
- D-pad left/right change hero
- View scoreboard, Menu pause
