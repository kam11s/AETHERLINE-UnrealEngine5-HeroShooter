# AETHERLINE locked look

Palette: void #07090D, plate #141A22, teal #2EE6FF, amber #E39A2E.
World: industrial night yard, wet concrete, cover cubes, one cyan capture ring, amber tower practicals, fog.
Camera: true first person, weapon lower-right, thin plus crosshair.
Heroes: Wraith, Bastion, Pulse, Volt, Grav, Ember.

## Night lighting rig (code-driven)

No map assets are in git, so `AALArenaBuilder` retunes whatever the loaded map already has
(`Template_Default` or the local RavelinYard) at Play and then builds its own fixtures.
Everything lives in `Source/Aetherline/ALArenaBuilder.cpp`, namespace `ALNight`.

- `NightSky()` — DirectionalLight becomes a 0.35 lux cool moon, `SkyAtmosphere` sky luminance is
  scaled to deep teal-navy, `VolumetricCloud` is hidden, SkyLight is a faint cool fill, height fog is
  a dark teal ground haze with volumetric fog on.
- `Practicals()` — teal lamp posts on the capture plate corners, amber floods over the cover field,
  lit caps + lamps on the four pylons, two teal spot masts raking the plate. All local lights are in
  candelas, no shadows (laptop budget).

Auto exposure is OFF for the project, so these are absolute values. The old 2.2 lux sun read as full
day; if the night is too dark or too bright on your machine, tune `ALNight::MoonLux` first, then the
candela numbers in `Practicals()`, then `ALNight::SkyLuminance`.

## Industrial yard (code-driven)

`AALArenaBuilder::Yard()` builds the cover field from Engine BasicShapes cubes at Play; nothing is
placed in the map. Palette and sizes live in namespace `ALYard` in `ALArenaBuilder.cpp`:
Rust, Ochre, DeepTeal, Gunmetal, Concrete, Hazard (amber stripe), Grate.

Heights are tuned against the hero capsule (176 tall, eye 152, jump apex ~138, step-up 45):

| Piece | Height | Role |
| --- | --- | --- |
| Crate (`CrateStack`) | 110, piles to 220 | vault in one jump; second tier from the first |
| Barrier (`Barrier`) | 136 | chest cover, eye clears it to shoot over |
| Container (`Container`) | 260 per tier | full block; stacked units bridge and overhang |
| Pipe rack (`PipeRack`) | lowest pipe underside 222 | walk-under lane marker |
| Catwalk (`Catwalk` + `Ramp`) | deck top 278, underside 262 | walk under, 29 deg ramp up |
| Pillar (`Pillar`) | 400 | vertical break-up behind the field |

Layout: barriers and small crate piles ring the spawn plate, container blocks and pipe racks sit
at 1000-1700, catwalks and pillars at 2000-2500, and `ContainerRing()` puts a broken line of
one- and two-high containers at 3000 so the walls are never the first thing in view. The plate
(600 radius) and every `PlayerStart` (420 radius) are kept clear via `Fits()`; a piece whose
footprint would intrude is skipped, so moving PlayerStart never buries the spawn. The lamp posts,
pylons and spot masts from `Practicals()` are untouched and the layout leaves their positions free.
