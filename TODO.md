# MyWorld — TODO / Backlog

Living backlog for continued development. Each item lists **where**, **why**, and **risk**.
Move items to *Done* when verified, delete items that turn out to be non-issues.

> Convention: `file.c:line` references are approximate — confirm before editing, the
> codebase shifts. Anything touching **save format** must bump `SAVE_VERSION` (types.h)
> and add a migration branch in `save.c`, then be tested with a real load round-trip.

---

## High priority (real, scoped)

- [ ] _(none open)_

## Medium priority

- [ ] _(none open)_

## Low priority / polish

- [ ] _(none open)_

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
- **Animal breeding is implemented.** Feed food to pig/cow/sheep/chicken -> love mode
  (15s) -> two in-love same-type mobs nearby spawn a baby with a 2-min grow timer
  (player.c:~1434, mob.c grow/heart display). Baby sprites now scale to half size
  (part 7).

---

## Done

> Historical done entries (parts 1-16) are preserved in git history and previous commits.
> This section is intentionally kept lean.

- **Part 28:** Sprint hunger 已存在验证 (`HUNGER_SPRINT_MULT=2.0f`, player.c:1903)，无需额外实现。
- **Part 27:** 近战伤害验证。Host 端 `PKT_DAMAGE_MOB` 处理增加范围验证（`BREAK_RANGE * 1.5`）和武器伤害匹配检测，偏离过大则使用预期值。
- **Part 26:** 弓箭蓄力机制。按住右键蓄力（0-1.5秒），松开发射。速度随蓄力线性增长（0.5x-1.5x），满蓄力触发暴击（+50%伤害 + 黄色粒子）。新增 `bowChargeTimer`/`bowCharging` 字段、`BOW_CHARGE_MAX` 常量、`FireBowWithCharge()` 函数。`PktBowRequest` 新增 `charge` 字段。新增 `/heal` 和 `/list` 命令。
- **Part 25:** Sound sync. `PKT_SOUND_EVENT` with 4 sound IDs (bow fire, hurt/teleport,
  pickup/catch, splash). Host broadcasts sound events after authoritative bow fire, ender
  pearl teleport, and fishing cast. Clients play the corresponding sound on receive.
- **Part 24:** Achievement sync. `PKT_ACHIEVEMENT_UNLOCK` — client sends unlock to host,
  host relays to all other clients. Clients show achievement notification + play XP sound.
  Extracted `GetAchString()` helper shared by `UnlockAchievement` and network handler.
- **Part 23:** Multiplayer arrow damage authority. Removed client-local `DamageMob` call
  for player projectile hits (`mob.c:156` replaced with pending-hit tracking). Host
  `ProcessPendingProjectileHits()` applies damage and broadcasts `PKT_DAMAGE_MOB` to all
  clients. Client waits for authoritative damage from host's `PKT_MOB_STATE` and
  `PKT_DAMAGE_MOB` sync. Added `pendingProjectileHitCount/Index/Damage` globals.
- **Part 22:** Server-authoritative item drop. `PKT_ITEM_DROP` — client sends drop request,
  host validates inventory state, spawns entity (broadcasts `PKT_ENTITY_SPAWN`),
  broadcasts `PKT_INVENTORY_SYNC`.
- **Part 21:** Server-authoritative cauldron. `PKT_CAULDRON_SYNC` — client sends interaction
  request, host validates and updates `cauldrons[]`, broadcasts authoritative state to
  all clients.
- **Part 20:** Server-authoritative fishing rod. `PKT_FISHING_REQUEST` with CAST/RETRACT
  actions. Host spawns fishing projectile (`isFishing=true` via `PKT_PROJECTILE_SPAWN`),
  manages bite timer, determines catch, syncs inventory.
- **Part 19:** Server-authoritative enchanting. `PKT_ENCHANT_REQUEST` — client sends
  selected enchantment option with type/level/cost, `TryEnchantRemote` validates enchanting
  table, XP cost, and item eligibility. Host applies enchantment, broadcasts inventory sync.
- **Part 18:** Server-authoritative ender pearl. `PKT_ENDER_PEARL_REQUEST` + `PKT_PLAYER_TELEPORT`.
  `TryEnderPearlRemote` validates pearl possession, range, and safe landing position
  (AABB block collision check with outward scanning). Host teleports player, syncs inventory,
  broadcasts teleport position to all clients.
- **Part 17:** Server-authoritative bow/arrow. `PKT_BOW_REQUEST` — client sends fire intent
  (direction), `TryFireBowRemote` validates bow possession, durability, and arrow count.
  Host consumes arrow, reduces durability (with Unbreaking check), applies Power enchantment,
  broadcasts projectile spawn + inventory sync. Updated `PktProjectileSpawn` with `isFishing`
  field for future fishing projectile sync.
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
- **Parts 1-11:** See git log / previous `TODO.md` revisions.
