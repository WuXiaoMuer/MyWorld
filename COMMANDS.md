# MyWorld — Chat & Commands

## Chat

Press `T` to open the chat bar, type a message, and press `Enter` to send.
- `Esc` cancels the message and closes the chat bar.
- Recent messages fade out after ~12 seconds.
- In multiplayer, chat is relayed through the host.

## Commands

Commands start with `/`. They work in single-player and on the **host** in multiplayer.
Clients connected to a host can type `/help`, but other host-only commands will not execute
on their machine.

| Command | Description | Example |
|---------|-------------|---------|
| `/help` | Show the command list. | `/help` |
| `/tp x y` | Teleport to block coordinates `(x, y)`. | `/tp 100 80` |
| `/give id count` | Give yourself `count` of block/item `id`. | `/give 4 32` |
| `/time day\|night\|noon\|midnight\|value` | Set time of day. | `/time day` |
| `/weather clear\|rain\|thunder` | Set weather. | `/weather rain` |

### `/give` item IDs

Use the numeric block/item ID from `BlockType` in `types.h`. A few common IDs:

| ID | Item |
|----|------|
| 1 | Grass block |
| 2 | Dirt |
| 3 | Stone |
| 4 | Cobblestone |
| 5 | Wood log |
| 6 | Planks |
| 7 | Leaves |
| 8 | Sand |
| 9 | Glass |
| 10 | Brick |
| 11 | Stone brick |
| 12 | Bedrock |
| 13 | Coal ore |
| 14 | Iron ore |
| 15 | Gold ore |
| 16 | Diamond ore |
| 17 | Redstone ore |
| 18 | Lapis ore |
| 19 | Gravel |
| 20 | Farmland |
| 21 | Crops |
| 22 | TNT |
| 23 | Chest |
| 24 | Furnace |
| 25 | Crafting table |
| 26 | Stone pressure plate |
| 27 | Wooden pressure plate |
| 28 | Lever |
| 29 | Redstone torch |
| 30 | Redstone wire |
| 31 | Redstone lamp |
| 32 | Bookshelf |
| 33 | Cauldron |
| 34 | Enchanting table |
| 35 | Water |
| 36 | Lava |
| 37 | Mud |
| 38 | Snowy grass |
| 39 | Wooden door |
| 40 | Ladder |
| 41 | Sugar cane |
| 42 | Cactus |
| 43 | Tall grass |
| 44 | Flower |
| 45 | Torch |
| 50+ | Tools, armor, food, items (see `types.h` `BlockType` enum) |

For the full list, see the `BlockType` enum in `types.h`.
