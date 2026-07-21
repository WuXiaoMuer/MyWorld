# MyWorld — TODO / Backlog

Living backlog for continued development. Each item lists **where**, **why**, and **risk**.
Move items to *Done* when verified, delete items that turn out to be non-issues.

> Convention: `file.c:line` references are approximate — confirm before editing, the
> codebase shifts. Anything touching **save format** must bump `SAVE_VERSION` (types.h)
> and add a migration branch in `save.c`, then be tested with a real load round-trip.

---

## High priority (real, scoped)

- [ ] _(none open)_

## Medium priority (multiplayer authority — architectural)

- [ ] **Server-authoritative inventory — milestone 2.** Milestone 1 implemented: host now
      maintains and syncs authoritative inventory, and validates remote block placement.
      Crafting and item consumption (eating, buckets, seeds, breeding) are now also
      authoritative. Remaining: arrows/bow, enchanting, ender pearl, fishing rod, cauldron,
      and pickup/drop authority. Large change.

## Low priority / polish

_(none open)_

## Ideas / not yet scoped

_(none open)_

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
  (player.c:~1434, mob.c grow/heart display). Baby sprites now scale to half size
  (part 7).

---

## Done

> Historical done entries (parts 1–15) are preserved in git history and previous commits.
> This section is intentionally kept lean.

- **Part 16:** Two new enchantments — Knockback (+50% knockback per level, swords) and Fire
  Aspect (sets mobs on fire for 1.5s/level, swords). Added `fireTimer` to Mob struct for
  fire damage over time. Applied effects in both local and remote attack paths. i18n EN/ZH/JA.
- **Part 15:** In-game chat (`T`) with message history and network relay; slash commands
  (`/help`, `/tp`, `/give`, `/time`, `/weather`); updated `README.md` and added
  `COMMANDS.md`.
- **Part 14:** Authoritative remote item use — host validates eating, bucket place/collect,
  hoe tilling, seed planting, and mob breeding, then syncs inventory.
- **Part 13:** Authoritative crafting (`PKT_CRAFT_REQUEST`, `CraftForPlayer`).
- **Part 12:** Server-authoritative inventory milestone 1 (`PKT_INVENTORY_SYNC`, remote
  block placement validation).
- **Part 11:** Enchanting table per-session state + passive-mob death-cause cleanup.
- **Part 10:** Biome-specific passive mob spawns.
- **Parts 1–9:** See git log / previous `TODO.md` revisions.
