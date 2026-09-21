# Respawn + half climb (2026-09-21)

Kam does not need to be home for GitHub. Play-test still waits until he is at the PC.
Do not git pull this over D:\\Aetherline while Unreal is open. Local Play already has the working feel.

## Landed on branch `cursor/ravelin-gun-feel-recoil-0fc3`
`ALHeroCharacter.h` now has:
- RecoilPitchLightDeg 0.4 (was 0.8)
- RecoilPitchHeavyDeg 1.3 (was 2.6)
- RecoilClimbMaxDeg 3.0 (was 6.0)
- RespawnDelay 2.0
- HandleDeath() / Respawn()

## Cpp still required on that branch (other bots already applied this locally)
On 0 HP call HandleDeath from ServerApplyDamageTo_Implementation.
Tick: if dead, stop fire, count RespawnTimer down, then Respawn().
HandleDeath: no collision, disable movement, RespawnTimer = RespawnDelay (skip respawn in Battle Royale).
Respawn: Health = MaxHealth, walking, teleport via AALGameMode::FindSpawnLocation(TeamId), Z=120.
Kills in skirmish increment AllyScore / EnemyScore.

## main
Still compile-safe rematch loop only. Do not merge the Cursor stack.
