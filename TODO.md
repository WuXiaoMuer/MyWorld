# MyWorld — TODO / Backlog

Living backlog for continued development. Each item lists **where**, **why**, and **risk**.
Keep it honest: move items to *Done* when verified, delete items that turn out to be non-issues.

> Convention: `file.c:line` references are approximate — confirm before editing, the
> codebase shifts. Anything touching **save format** must bump `SAVE_VERSION` (types.h)
> and add a migration branch in `save.c`, then be tested with a real load round-trip.

---

## High priority (real, scoped)

- [ ] _(none open — chest durability/enchant persistence shipped in part 4)_

## Medium priority (multiplayer authority — architectural)

- [ ] **Server-authoritative inventory.** Host trusts client-reported inventory for block
      place (game.c:~1290) and item use. A hacked client can desync/duplicate. Proper fix:
      host owns each player's inventory and validates actions against it. Large change.
- [ ] **Sync container/station contents in multiplayer.** No packets for chest, furnace,
      or enchanting-table state — each client keeps a local copy, so shared use desyncs.

## Low priority / polish

- [ ] **Baby mobs render full-size.** Breeding works and babies have a grow timer, but
      `Draw*Sprite` (pig/cow/sheep/chicken) use hardcoded pixel coords, so babies look
      adult-sized. Each sprite fn would need a scale factor around `mob->position`. NOTE:
      do NOT shrink baby hitboxes via `GetMobW/H` until the sprites scale too, or the
      hitbox and sprite desync. (Slimes already scale because `DrawSlimeSprite` derives
      from `GetMobW`.)
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
- **Animal breeding is implemented.** Feed food to pig/cow/sheep/chicken → love mode
  (15s) → two in-love same-type mobs nearby spawn a baby with a 2-min grow timer
  (player.c:~1434, mob.c grow/heart display). Only the baby *sprite scale* is missing
  (see polish item above).

---

## Done

### Session 2026-06-26 (part 5 — multiplayer UX)
- **Open to LAN (in-game, MC-style).** Pause menu now has a contextual "Open to LAN"
  control: in single-player it calls `NetHostStart(NET_PORT)`, sets `localPlayerId=0`,
  flashes the local IP\:port, and keeps playing — the existing `NetIsHost()`-gated host
  loop already accepts late joiners. Once hosting it shows a live status line
  (`ip:port (n/max)`) instead. Hidden for clients (can't host). No new state needed.
- **Host/Join menus fixed + polished.** Replaced hardcoded English in the host-waiting
  and join screens (`Players: %d/%d`, `ESC: Cancel`, `ESC: Back | Connect`) with i18n
  strings (EN/ZH/JA). HOST_WAITING joiners now get the same starter inventory as the
  in-play late-join path (previously they joined empty-handed — an inconsistency).

> **Needs in-game / two-instance verification:** run two copies; on instance A pause →
> Open to LAN; on instance B Join → enter A's IP → confirm spawn, movement sync, and
> that B starts with the wooden-tool starter kit.

### Session 2026-06-26 (part 4 — content + UI polish)
- **Chest durability + enchantments persist (SAVE_VERSION 12 → 13).** `ChestData` now
  carries per-slot `durability[]` + `enchantments[]`; every chest transfer point (click
  take/place/stack, shift-click both directions) and the chest screen's inventory grid
  move them with the item. Looted tools (e.g. dungeon bows) now generate at full
  durability. Also fixed: the chest screen's inventory click previously dropped
  enchantments (carried durability only).
- **TNT block (new gameplay / hotspot).** New `BLOCK_TNT` (full block chain, added at the
  END of the enum to keep save IDs stable). Right-click to light the fuse (~2s), then
  `ExplodeAt()` destroys a radius-4 area, damages player + mobs by distance falloff, and
  **chain-detonates** other TNT in the blast. `ExplodeAt` is separate from the creeper's
  inline explosion to avoid regressing it. Recipe: 5 redstone → 1 TNT (needs table).
  Uses existing fuse + thunder sounds; no new asset needed.
- **Enchanted-item glint (UI polish).** Subtle animated purple shimmer over enchanted
  icons in the hotbar, inventory grid, armor slots, and both chest-screen grids
  (`DrawEnchantGlint`).

> **Needs in-game verification:** craft + ignite TNT (incl. a chain reaction); store an
> enchanted/used tool in a chest, reload, confirm durability + enchant survive; confirm
> the glint appears on enchanted items only.

### Session 2026-06-26 (part 3 — perf + mob polish)
- **Crop registry — eliminated the 524K-cell scan.** `UpdateCrops` now iterates a
  registered crop-cell list (mirrors the pressure-plate registry) instead of scanning the
  whole world every 5s. Cells register on plant / world-gen / load-rescan
  (`RebuildCropList` at startup, plus network-planted crops); stale cells (broken,
  exploded, flooded) are pruned lazily during the scan, so no destruction path needs an
  explicit unregister. Cap `MAX_CROP_CELLS = 8192`.
- **Slime split size — per-instance.** Added `GetMobW/GetMobH(const Mob*)`; small slimes
  (`slimeType==1`) are now half-size in both sprite and hitbox, kept consistent across
  collision, projectile hits, contact, and drawing (all 43 size usages live in mob.c).
- Verified **breeding already works**; documented it and filed baby-sprite-scale as polish.

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
