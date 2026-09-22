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
