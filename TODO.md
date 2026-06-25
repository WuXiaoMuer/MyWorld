# MyWorld — TODO / Backlog

Living backlog for continued development. Each item lists **where**, **why**, and **risk**.
Keep it honest: move items to *Done* when verified, delete items that turn out to be non-issues.

> Convention: `file.c:line` references are approximate — confirm before editing, the
> codebase shifts. Anything touching **save format** must bump `SAVE_VERSION` (types.h)
> and add a migration branch in `save.c`, then be tested with a real load round-trip.

---

## High priority (real, scoped)

- [ ] **`UpdateCrops` scans the whole world every 5s.** world.c:~2750 loops all
      2048×256 = 524,288 cells. Maintain a crop-cell list (mirror the pressure-plate
      registry pattern: register on plant, unregister on break) and iterate only that.
      Risk: low/medium — must keep the list in sync with every plant/break/explosion path.
      Note: SaveWorld also double-scans the world for the crop section; the same registry
      would speed that up too.

## Medium priority (multiplayer authority — architectural)

- [ ] **Server-authoritative inventory.** Host trusts client-reported inventory for block
      place (game.c:~1290) and item use. A hacked client can desync/duplicate. Proper fix:
      host owns each player's inventory and validates actions against it. Large change.
- [ ] **Sync container/station contents in multiplayer.** No packets for chest, furnace,
      or enchanting-table state — each client keeps a local copy, so shared use desyncs.
- [ ] **Chests lose item durability & enchantments.** `ChestData` (types.h) stores only
      `item` + `count`. Tools/armor placed in a chest come back at full durability and
      un-enchanted. Extend `ChestData` (+ save format → bump version).

## Low priority / polish

- [ ] **Slime split size.** Small slimes (`slimeType==1`) share the big slime's sprite and
      hitbox because `mobWidth[]`/`mobHeight[]` are per-*type*, not per-*instance*
      (mob.c DrawSlimeSprite ~1580, contact/collision). Needs per-instance size plumbing.
- [ ] **Dead defensive death-cause cases** for passive mobs (mob.c:838-840) are unreachable
      (`mobDamage[]==0` returns early). Harmless; remove only if doing a cleanup pass.

## Ideas / not yet scoped

- [ ] Biome-specific passive mob spawns (mob.c spawn logic is biome-agnostic).
- [ ] More achievements (fishing, enchanting, breeding); current set is 6 (game.c).
- [ ] Player names in multiplayer (`PktJoin.playerName` exists but is always "Player").

---

## Verified NON-issues — do not re-investigate

These were flagged in audits but confirmed fine; left here to avoid wasted re-checks.

- **Weather on join is not broken.** Client sets `weather.type/duration` from the welcome
  packet; with `transitionTimer==0`, `UpdateWeather` snaps `rainAlpha` to target on the
  first frame (weather.c:~68). Self-corrects.
- **Trade `nameId` being `STR_NONE` is harmless.** The trade UI renders trades by item
  name via `GetBlockName` (crafting.c:740/747); `nameId` is simply unused.
- **Enchanting-table UI exists.** Fully implemented at `DrawEnchantingTableUI`
  (rendering.c:4209), with option generation in player.c:~1062.

---

## Done

### Session 2026-06-26 (part 2 — "complete version")
- **Save bug fixed:** cauldron section was written last by `SaveWorld` but read early by
  `LoadWorld` (inside the v≥3 block), desyncing the stream for v11 saves — they loaded
  corrupt or failed. Loader now reads cauldron + crop sections after world+modified,
  matching write order. **This recovers previously-unloadable v11 saves (e.g. world7.mwsav).**
- **Feature complete — crop growth persistence (SAVE_VERSION 11 → 12).** Sparse section
  stores `(x,y,growth)` for every `BLOCK_CROPS` cell; older saves default to stage 0.
  Crops now keep their maturity across save/load.
- **Village farms** generate with deterministic initial growth (1–7) from the seed
  (world.c), so they look established instead of freshly planted.
- **Feature — ENCH_POWER (bow):** new ranged enchant, +2 arrow damage per level. Added
  enum + `Projectile.damage` field + i18n (3 langs) + both name-display paths; bow enchant
  pool now offers Power/Unbreaking instead of 3× identical Unbreaking. Mob/network arrows
  default to base damage (no signature/packet change needed).
- **Latent bug fixed:** `SpawnProjectile` now resets transient fields (`isFishing`,
  `hasBite`, `fishTimer`, `catchValue`, `damage`) so a reused slot can't inherit stale
  state (an arrow reusing a fishing-bobber slot could behave as a bobber).
- Also fixed a missing Silk Touch case in the inventory tooltip enchant-name mapping.

> **Needs in-game verification** (build is clean but GUI not runtime-tested here):
> save a world with growing/mature crops → reload → crops keep their stage; and try
> loading the existing `saves/world7.mwsav` (v11) to confirm the load-order fix.

### Session 2026-06-26 (part 1)
- Trimmed `CLAUDE.md` (dropped stale line counts & derivable prose; kept build, "add new X"
  recipes, conventions, invariants).
- Fixed signed-overflow UB in BGM wind-noise generator (sound.c:451).
- Fixed creeper explosion particle Y-offset using player distance instead of radius (mob.c:616).
- `SortInventory` now carries `itemEnchantments` so enchants don't scramble on sort (rendering.c).
- Debug-overlay weather text now uses i18n via `S()` (+ 4 new STR_ strings, 3 langs).
- Network block-place now guards `inventoryCount > 0` (game.c).
- Network joiners snap to their own surface (`FindSpawnSurfaceY`) instead of the host's Y,
  preventing suffocation when the host is underground (game.c).
- **Feature:** wheat crops now render 8 growth stages; chunk re-bakes on growth without
  flicker; planting resets growth to 0 (world.c, player.c).
