# Bread

Bread is a password-driven table generator and byte-confusion engine. Its job is to take a tiny human password and unroll it into a behemoth amount of deterministic data, then keep pushing that data through layers that are intentionally difficult to narrate backward. The project does not stop at "derive a key and feed a normal cipher." It expands, twists, fills, topples, matches, repaints, flushes, pathfinds, and respawns until the original password is many state transitions away from the final byte field.

At a high level, Bread starts from a small password, expands it into fixed-size seed regions, uses several counter families and 16 custom byte twisters to produce wide seed material, and then drives puzzle-game and maze simulations that emit bytes according to simulation history rather than source order. The end result is a large deterministic substrate intended for masking, archive workflows, table-driven transforms, and experiments in recovery-aware encryption design.

## What This Project Ultimately Does

The short version is:

- take a small user password
- expand it into a `7680`-byte block
- balloon it into larger seed regions
- generate `22` large tables
- run those tables through puzzle and maze confusion stages
- use the finished tables as the backbone for later archive and masking workflows

Current sizes in the code:

- `PASSWORD_EXPANDED_SIZE = 7680`
- `PASSWORD_BALLOONED_SIZE = 15360`
- `BLOCK_SIZE_L1 = 261120`
- `BLOCK_SIZE_L2 = 522240`
- `BLOCK_SIZE_L3 = 1044480`

Current table counts:

- `12` `L1` tables
- `6` `L2` tables
- `4` `L3` tables
- `22` total tables

That is the core idea of Bread: a little password goes in, a great deal of deterministic, structured, password-derived memory comes out.

## Project Posture

This repository is public as a demonstration of the backbone and the rigor of the backbone. That does **not** mean the files are intended for reuse, resale, repackaging, or third-party commercialization. Unless a separate written agreement says otherwise, these files should be understood as proprietary project material published to show the architecture and the seriousness of the engineering effort, not as a public-domain component kit.

This project was built in an apartment room, not a science lab. It was not produced under a university cryptography program, a government lab, or a formal commercial security certification pipeline. That matters. The project is ambitious and highly opinionated, but many of its strongest claims are not standing on solid formal foundations. There are no proofs here that inversion is impossible. There is no formal cryptanalytic paper here. There is no external audit here. There is only code, a lot of testing, and a very strong design philosophy.

## Important Caveat

Bread is not presented as a formally audited cryptographic standard or a compliance-ready replacement for mature external crypto libraries.

What Bread *does* claim, in the language of design intent rather than theorem, is:

- the original password should be hard to recover from final tables without already knowing it
- the system should remain reproducible for the rightful user who does know the password
- the confusion stages should push the data far enough away from simple positional ancestry that a backward narrative becomes painful

Those are goals and beliefs. They are not formal proofs.

## Open Code, Closed Password

Bread is built around an openly stated attitude:

- the code can be public
- the algorithms can be studied
- the final password should still remain the secret

The guiding belief is that public visibility of the machinery is not the same thing as having the user's original password. Bread is trying to live in that gap.

## High-Level Pipeline

The current pipeline is easiest to understand as a stack of deterministic transforms:

1. repeat-fill the user's password into a `PASSWORD_EXPANDED_SIZE` source block
2. expand and chain that block through one of 16 custom byte twisters
3. fill large tables from mixed counter families and custom twisters
4. run puzzle-game confusion stages over `7680`-byte regions
5. run maze confusion stages over `15360`-byte regions
6. use the resulting tables for later archive, masking, and recovery-aware workflows

The important part is that the project aims for **width**. Bread does not want a thin keystream. It wants a large password-derived memory field.

## Why AES, ChaCha, ARIA, and Custom Counters All Appear

Bread currently fills seed families from:

- `3` AES-based fills
- `1` ChaCha-based fill
- `1` ARIA-like fill
- `16` custom byte-twister fills

The AES-like, ChaCha-like, and ARIA-like paths are there for seed diversity and domain separation. In the current code, the AES-derived fills are deliberately separated so they do not collapse into identical same-sized tables: one is used directly, one is reversed, and one is bit-inverted. ChaCha and ARIA-like paths add different state-shape and diffusion stories before the custom twisters even begin.

The practical reason to mix several seeded families is simple: if one family has a blind spot, the whole table build should not inherit that blind spot in a synchronized way. Heterogeneity is being used here as an engineering hedge, not as a magic proof.

### Why Mersenne Was Cut From the Real Password Path

Older experimentation around "Mersenne/Meserene-style" thinking does not appear in the live password path now. The design reason is straightforward: a Mersenne-style PRNG is great for many simulation tasks, but it is not the shape Bread wanted sitting directly on top of a real user's password. Bread ultimately chose families that were either standard counter-inspired constructions or much more aggressively stateful custom transforms, because a large linear recurrence was judged to be the wrong backbone for the true password-expansion route.

That is a design rationale, not a theorem. The current code reflects the outcome of that choice: AES-like, ChaCha-like, ARIA-like, and custom twisters remain; a Mersenne family does not.

## The New Byte Twisters

The current production byte twisters live in:

- [`src/Tables/password_expanders/ByteTwister.hpp`](src/Tables/password_expanders/ByteTwister.hpp)
- [`src/Tables/password_expanders/ByteTwister.cpp`](src/Tables/password_expanders/ByteTwister.cpp)

The active set is `kType00` through `kType15`. In the current new-system source they correspond to a selected candidate set:

- `224, 340, 420, 631, 682, 737, 748, 750, 786, 802, 872, 1093, 1170, 1200, 1266, 1301`

These byte twisters are not just "increment a counter and copy bytes out." Each one has four major stages:

1. `KeySeed`
2. `SaltSeed`
3. `TwistBlock`
4. `PushKeyRound`

That is the new shape of the system.

### Key Stack

The key stack is a rolling keyed state:

- depth: `16`
- bytes per row: `32`
- total keyed working state: `512` bytes

`KeySeed` builds that `16 x 32` stack from the source block using wrapped reads across the full `7680`-byte password-expanded region. It is not just copying bytes into the stack. It is selecting positions, combining bytes, and accumulating them into rotating planes. The result is a keyed grid that later rounds keep consulting.

In practical terms, the key stack gives each candidate a reusable internal memory of the original source block. Later rounds do not merely revisit the source bytes directly; they also revisit what earlier source bytes have already deposited into the stack. That is one of the core differences between the new twisters and simpler one-shot expansion code.

### Salt Buffer

The salt buffer is currently `32` bytes wide. `SaltSeed` also scans the original expanded block with candidate-specific wrapped offsets, mixes source bytes together, and accumulates them into that compact salt region. The salt is then sampled repeatedly by `TwistBlock` and `PushKeyRound`.

The salt is deliberately much smaller than the main block. That makes it a hot control surface rather than a copy of the whole source. A single source block can therefore keep feeding a much smaller, repeatedly reused bias layer during twisting.

### TwistBlock

`TwistBlock` is where the actual block transform happens. For each round, it takes:

- a source block
- an optional worker/scratch buffer
- a destination block
- the round number
- the salt buffer
- the rolling key stack

Inside the current candidates, the common pattern is:

- derive a twiddle word from constants, the round index, source bytes, and salt bytes
- sweep the block in several passes
- use wrapped reads across the whole `7680`-byte source region
- consult `KeyStackByte(...)` repeatedly
- consult salt bytes repeatedly
- feed forward lane-local state such as `lane_state`
- write through both the worker buffer and the destination buffer

In other words, the new twister is not one direct pass from source to destination. It is several interacting passes, with local feedback, wrapped long-range reads, and key/salt state mixed into the walk.

### PushKeyRound

`PushKeyRound` is the part that makes the chain *keep changing*. After a block has been twisted, the candidate derives a `32`-byte `NextRoundKeyBuffer` from the destination block, the current salt, and the current key stack. It then rotates that new key material into the key stack with `RotateKeyStack(...)`.

That means later rounds are not using a frozen seed key. The output of round `n` becomes part of the key memory consulted by round `n+1`. That "push forward" behavior is a major part of the new system.

### The Expansion Story In Plain Language

For a first block, the mental model is:

1. repeat-fill the human password until one full `7680`-byte source block exists
2. run `KeySeed`
3. run `SaltSeed`
4. run `TwistBlock(round = 0)`
5. run `PushKeyRound`

For the second block and beyond:

1. use the previous output block as the new source
2. keep the rolling key stack
3. reuse or reseed salt depending on the caller's path
4. run `TwistBlock(round = n)`
5. run `PushKeyRound` again

In the current `PasswordExpander` path, one expansion seeds key and salt once and then chains forward across rounds. In the current `FastRand` path, the key stack persists across wraps while salt is reseeded per wrap from the new sample block. Both cases follow the same new-system idea: source bytes are not the only state anymore.

### General Twisting Behavior

Across the 16 current candidates, the common behaviors are:

- whole-block wrapped reads over `PASSWORD_EXPANDED_SIZE`
- per-round twiddle evolution
- keyed lookups into a rolling stack
- repeated salt lookups
- multiple passes per round
- worker-buffer and destination-buffer interplay
- round-to-round key pushing

There is no dedicated NEON specialization for these twisters right now. The current value proposition is structural quality first. Speed tuning can happen later.

### Quality Notes From The Reports

The most relevant current quality references are:

- [`run_avalanche_report.txt`](run_avalanche_report.txt)
- [`run_bit_inclusion_report.txt`](run_bit_inclusion_report.txt)

In the current sampled avalanche report:

- all `ExpandPassword_00` through `ExpandPassword_15` land extremely close to `50%` average flipped output bits
- all `100` output blocks changed in every sampled trial
- there were `0` zero-difference trials

In the current sampled bit-inclusion report:

- all `16` expanders reached `100.0000%` inclusion
- all `6,144,000` sampled output bits were included
- `unincluded_bits = 0` for every expander

That does **not** prove cryptographic strength. It does show that the current twisters are achieving the sampled avalanche and inclusion targets the project was chasing.

## Password Expansion

The password-expansion layer lives in:

- [`src/Tables/password_expanders/PasswordExpander.hpp`](src/Tables/password_expanders/PasswordExpander.hpp)
- [`src/Tables/password_expanders/PasswordExpander.cpp`](src/Tables/password_expanders/PasswordExpander.cpp)

The current expansion model is:

1. repeat-fill the user's password into one full `PASSWORD_EXPANDED_SIZE` source block
2. seed the key stack
3. seed the salt buffer
4. twist block `0`
5. push a new round key
6. use block `0` as the source for block `1`
7. twist block `1`
8. push again
9. keep chaining until enough output exists

This is no longer "copy password into a big array and run one transform." It is a stateful chain with carried key memory.

## Puzzle-Game Flows

The puzzle stage works on `7680`-byte regions and uses an `8 x 8` board. Every tile has a tile type, a byte payload, and optionally a power-up. The board then runs a real play loop: spawn, search, move, match, trigger power-ups, topple, cascade, respawn, repeat. The emitted bytes are the payloads of tiles that get committed through these match and removal cycles. That means bytes do not leave the board in their original source order. They leave when the board history says they leave.

There are 16 game modes formed from four axes:

- move family: `tap` or `swap`
- match family: `streak` or `island`
- tile family: `four-type` or `five-type`
- play policy: `greedy` or `random`

So a board can behave like a line-matching swap game, or a blob-matching tap game, or a greedily selected five-color cascade engine, or a random-move island engine. That matters because the same seed bytes can produce very different emission histories depending on which family combination is running.

The puzzle flow is valuable because the board is not merely a permutation machine. A tile can sit quietly, become part of a legal move, get dragged into a match, trigger a power-up, vanish during a special event, or survive until a later cascade. The emitted output is therefore shaped by move discovery, board geometry, topples, re-spawns, special events, and mode rules. In that sense, the puzzle stage is a simulation residue generator more than a classic cipher round.

### Puzzle Power-Ups

Current puzzle power-ups are:

- `ZoneBomb`: clear a `3 x 3` area centered on the tile
- `Rocket`: clear the tile's full row and full column
- `ColorBomb`: clear every tile of the same type as the center tile
- `CrossBomb`: clear both diagonals through the tile
- `PlasmaBeamV`: clear the center column plus its left and right neighbors
- `PlasmaBeamH`: clear the center row plus its neighbors above and below
- `PlasmaBeamQuad`: combine the horizontal and vertical plasma beams
- `Nuke`: clear a `5 x 5` area
- `CornerBomb`: clear four `4 x 4` corner regions
- `VerticalBombs`: clear every other column based on random parity
- `HorizontalBombs`: clear every other row based on random parity

Rare board-wide special events are:

- `DragonAttack`
- `PhoenixAttack`
- `WyvernAttack`

`DragonAttack` randomly marks many surviving tiles across the board. `PhoenixAttack` clears four shuffled columns. `WyvernAttack` clears four shuffled rows.

### Example Puzzle Boards

These are illustrative board sketches, not exact runtime dumps.

Legend:

- `A B C D E` are tile types
- `*` means "this tile currently carries a power-up"

Example 1: a streak-heavy board

```text
A A A B C D B C
B C D B A C D A
C D A C B D A B
D B C D C A B C
A C B A D B C D
B D C B A C D A
C A D C B D A B
D B A D C A B C
```

Example 2: an island-heavy board

```text
A A B C C D D B
A A B C C D D B
B B A A D C C A
C D A A B B D C
C D D B B A A C
D C C B A A B D
B D C D C B A A
A B D C D B C A
```

Example 3: power-up tiles mixed into normal play

```text
A  B  C* D  A  B  C  D
B  C  D  A  B* C  D  A
C  D  A  B  C  D  A* B
D  A  B  C  D  A  B  C
A* B  C  D  A  B  C  D
B  C  D  A  B  C* D  A
C  D  A  B  C  D  A  B
D  A  B  C  D* A  B  C
```

## Game Stability

The best current reference for the board engine is [`game_stability_report.txt`](game_stability_report.txt). It runs `1250` executions per game across all `16` puzzle modes, with `7680` bytes emitted per run. In that report, every game shows `repeatability_failures=0`, `runtime_repeatability_failures=0`, `histogram_drift=0`, `broken=0`, and `inconsistent_flags=0x0`. Just as important, every listed catastrophe counter is also zero in the report: `overflow_catastrophic=0`, `overflow_cataclysmic=0`, `overflow_apocalypse=0`, and `impossible=0`. That means the current search-and-repair machinery held together across the full sampled sweep. The board found valid post-spawn and post-topple states, kept making deterministic progress, and never had to drop into the end-of-the-world fallback path during the sampled stability run.

The reason that matters is that the board is deliberately not trusting luck. Invalid or inconvenient board states fall through an ever-widening array of "security doors." After a spawn or topple, the board delegates to `GamePlayDirector`, which tries progressively more expensive repair strategies: single-tile edits, then two-tile edits (`BlueMoonCase`), then the so-called intelligent breakdown pass (`HarvestMoon`), then constructive local pattern planting, then full deterministic solver passes that try to build no-match boards, planted-match boards, or no-match boards that still preserve at least one legal move. In rough terms, the early doors are on the order of `O(V*T)` and `O(V^2*T^2)` over `V=64` tiles and `T` tile types, while the recursive deterministic solver has exponential worst-case behavior `O(T^N)` over the active search region. That solver is expensive, but it is being used as a late safety layer, not the first reflex. So far, the exhaustive search ladder has never been broken in the sampled stability run.

There is a second stability story here: even the final fallback ladder is layered. If the play loop spends too long resolving, the board records catastrophic cases. If it keeps failing to make progress over whole loop attempts, it records cataclysmic failure. Only after that does it enter the apocalypse scenario, which deterministically fills the remainder of the result buffer from the seed and reverses it. That apocalypse path exists to guarantee completion, not as the preferred confusion route. The important point for the README is that the current sampled report never needed it. In the language you asked for: the board really does pass through an ever-widening wall of security doors, and our exhaustive sampled search has not broken through to the final emergency door yet.

## Maze-Based Byte Confusion

The maze stage works on `PASSWORD_BALLOONED_SIZE = 15360` bytes and uses a `32 x 32` grid. It paints walkable cells from the seed, then runs a simulation with robots, cheese, sharks, and dolphins. Bytes leave the maze when cells are repainted or flushed during motion, collisions, special events, capture, and respawn cycles. Just like the puzzle stage, the point is that output order becomes a story about simulation history instead of a story about simple source position.

### Maze Families

Bread currently supports three generation families:

- `Custom`
- `Prim`
- `Kruskal`

Those families are defined in:

- [`src/Tables/maze/MazePolicy.hpp`](src/Tables/maze/MazePolicy.hpp)
- [`src/Tables/maze/MazePolicy.cpp`](src/Tables/maze/MazePolicy.cpp)

### The Custom Maze Strategy

The custom generator is not just "draw random walls." In the current code it does this:

1. start from a fully open `32 x 32` grid
2. shuffle all coordinates
3. turn roughly one quarter of the grid into walls
4. run `BreakDownOneCellGroups()` to destroy isolated diagonal one-cell traps
5. run `EnsureSingleConnectedOpenGroup()`

That last step is the key. `EnsureSingleConnectedOpenGroup()` uses disjoint-set / union-find style bookkeeping to detect disconnected open components, then repeatedly opens walls that either merge two components directly or at least grow a component toward a later merge. The loop cap is `20000` iterations. So the custom mode is random at the front, but connectivity-repaired and forced into one open connected group before it becomes a real playfield.

For comparison:

- `Prim` grows the maze from a random odd-cell start with randomized frontier walls
- `Kruskal` begins with isolated maze cells and removes walls only when they join different groups

Pathfinding for robots uses A* over the `32 x 32` grid, which is roughly `O(V log V)` with `V = 1024` in the heap-based implementation. Kruskal-style group merging is near-linear in practice with inverse-Ackermann union-find behavior, while Prim-style frontier expansion behaves like a randomized spanning-tree build. These are rough engineering complexity descriptions, not formal proofs.

### The Maze Actors

The maze simulation uses four actor families:

- `Robot`
- `Cheese`
- `Shark`
- `Dolphin`

Their roles are:

- `Robot`: the main collector; it pathfinds toward cheese and tries to reach it
- `Cheese`: the target object; captured cheese is later respawned
- `Shark`: the predator; sharks kill robots unless the robot is invincible
- `Dolphin`: a roaming disruptor; dolphins can steal cheese and can collide with sharks

The current probe layouts vary robot and cheese counts by maze family, keep sharks heavy, and always include dolphins. In the probe configuration table, shark count is fixed at `8`, while robots and cheeses scale by mode. Dolphin capacity is `4`, and the director uses that full dolphin roster.

### Maze Power-Ups

Current maze power-ups are:

- `Invincible`
- `Magnet`
- `Teleport`

The effects are:

- `Invincible`: if a shark collides with the robot, the shark dies instead
- `Magnet`: expands cheese capture reach to a radius of `2`
- `Teleport`: attempts a one-shot relocation to a valid walkable tile exactly `3` Manhattan steps from the target cheese

Current duration ranges:

- `Invincible`: `32` to `42` update cycles
- `Magnet`: `12` to `16` update cycles
- `Teleport`: immediate one-shot behavior

Teleport has a real fallback path. If no valid teleport location exists, the robot drops into `Magnet` instead. So even the power-up system tries not to waste a collected event.

### Maze Special Events

The maze stage has three rare-event families:

- `StarBurst`
- `ChaosStorm`
- `CometTrails`

`StarBurst` sweeps broadly across the maze and repaints or flushes most non-wall cells while avoiding the immediate `3 x 3` collect zones around living robots. `ChaosStorm` centers on sharks and repaints or flushes `3 x 3` neighborhoods around them. `CometTrails` repaints or flushes whole selected rows or columns.

### Old ASCII Diagrams

The older ASCII maze diagrams are preserved here on purpose.

Legend:

- `O` wall in the `RobotCheese` example
- `x` wall in the `Prim` and `Kruskal` examples
- ` ` open path

Example: `RobotCheese`

```text
[   OO O   O       O OO          ]
[       OO        OO   O  OO     ]
[   O  O               OO        ]
[    O     OO O    O  O O        ]
[O       OO   O      OO    O   O ]
[   O    O  O        O      O    ]
[  O     O          OO    O     O]
[   O        O        OO         ]
[   O   O O            O         ]
[  O    O  O          O          ]
[   O     O   O  O  O        OOO ]
[  O          O O               O]
[O O  O O     O    O  O  O O     ]
[ O    O     O   O    O   O  O OO]
[       O OO      OO OO         O]
[     OO   O OOOO   O     OO     ]
[ O  O O       O   O   O  O  O  O]
[ OO O O          O         OO   ]
[  O   O        OO   O OOO OO O  ]
[O   O  O  O                    O]
[  O O   O     O    O O     O  O ]
[ O         OO           O       ]
[ O O   O  O  O      OO    O O   ]
[   O           O     OO         ]
[O  OO O O       O     O     O O ]
[  O O    O   O     OO       O O ]
[       O             OO   OO O O]
[O   O O  O       O O           O]
[  O  O   O O  O   OO   O       O]
[  O O  O     O   O    O      O  ]
[O   O     OO   O O  OOO      O  ]
[     O  O O      O OO       O   ]
```

Example: `Prim`

```text
[xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx]
[x x x x     x   x x   x x     xx]
[x x x xxx xxxxx x xxx x xxx xxxx]
[x       x x       x x x       xx]
[xxx xxx x xxx xxxxx x xxxxx xxxx]
[x x x               x   x     xx]
[x xxxxxxx x xxxxxxxxx xxx xxxxxx]
[x x       x     x   x x x x   xx]
[x xxxxx xxxxxxxxx xxx x x x xxxx]
[x     x x x x x   x x         xx]
[x xxx x x x x xxx x x x x xxxxxx]
[x x         x x   x   x x x   xx]
[xxx x x xxx x xxx x xxxxxxx xxxx]
[x   x x   x   x x   x     x   xx]
[xxxxxxx xxxxxxx x xxx xxx x xxxx]
[x     x x x x   x     x x   x xx]
[xxx xxx x x xxx x xxxxx xxxxx xx]
[x   x     x     x x           xx]
[xxx xxxxx xxxxx x x xxx xxxxx xx]
[x                   x       x xx]
[x xxx xxx xxx xxx x xxxxxxxxx xx]
[x x     x x     x x x x     x xx]
[x xxx xxxxxxx x xxx x x xxx x xx]
[x x   x       x   x       x x xx]
[xxx xxx xxxxx x xxxxxxx x xxx xx]
[x   x   x     x     x x x   x xx]
[xxxxx x xxx xxx x xxx xxx xxxxxx]
[x   x x x   x   x       x x x xx]
[xxx x x x x x xxx xxxxxxx x x xx]
[x     x x x x x         x     xx]
[xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx]
[xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx]
```

Example: `Kruskal`

```text
[xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx]
[x     x   x x   x   x x   x   xx]
[x xxx xxx x xxx x x x x xxx x xx]
[x   x x x x   x   x x       x xx]
[x xxx x x x x xxx xxx xxxxxxxxxx]
[x   x       x x   x   x   x   xx]
[xxxxx xxxxxxx x xxx x x xxxxx xx]
[x x     x         x x     x   xx]
[x xxx xxxxxxxxx xxx xxx x xxx xx]
[x     x   x x x x x   x x   x xx]
[xxxxx xxx x x xxx xxx x x xxx xx]
[x x   x x   x   x x   x x     xx]
[x x xxx x xxx x x xxx x x x xxxx]
[x x     x x x x   x   x x x   xx]
[x xxxxx x x x x xxxxx xxxxxxx xx]
[x x x   x   x x x     x       xx]
[x x xxx x x x xxx x xxx x x xxxx]
[x         x x   x x x x x x   xx]
[xxx xxxxx x xxx xxx x xxxxx xxxx]
[x   x x   x x       x   x x   xx]
[x xxx xxxxx xxx xxxxx xxx xxx xx]
[x     x       x           x   xx]
[xxx xxxxxxx xxxxx x x xxx x xxxx]
[x x     x         x x x   x   xx]
[x xxxxxxxxxxx xxx xxxxx xxx x xx]
[x           x x     x   x x x xx]
[x x x x x x xxx xxx x x x x xxxx]
[x x x x x x     x x x x x x   xx]
[x xxx xxx x xxx x x xxxxx x xxxx]
[x x     x x x     x   x       xx]
[xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx]
[xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx]
```

## Maze Stability

The main reference is [`maze_stability_report.txt`](maze_stability_report.txt). It runs `1250` executions per maze across all `16` maze families. In the sampled report, every maze shows:

- `repeatability_failures=0`
- `runtime_repeatability_failures=0`
- `histogram_drift=0`
- `connectivity_failures=0`
- `topology_failures=0`
- `inconsistent_flags=0x0`
- `stall_cataclysmic=0`
- `stall_apocalypse=0`

That is a strong engineering signal. It means the sampled maze generator and simulation loop stayed deterministic, stayed connected, and never had to abandon the normal route for the apocalypse fallback.

## Authorized Recovery

Bread is not meant to deny rightful recovery. If the lawful user or lawful custodian has the actual password, the whole system is designed to regenerate deterministically. Recovery mode was treated as a first-class design concern from the beginning.

That is one of the deepest differences between Bread and a lot of conventional "just encrypt it and forget it" thinking. Bread wants:

- strong resistance without the password
- straightforward deterministic rebuildability with the password

## A Cartoon-Scale Guessing Estimate

This section is intentionally ridiculous, but it helps build intuition.

Suppose `1,000,000` monkeys are all trying to recover the original password from the final tables, and suppose, unrealistically, that the final tables give them no shortcut at all and they are reduced to blind guessing. Also suppose each monkey somehow manages `1` full password guess per second.

For a merely `16`-character printable-ASCII password, the raw search space is roughly:

- `95^16 ≈ 4.4 x 10^31` guesses

At `1,000,000` guesses per second total:

- about `4.4 x 10^25` seconds
- about `1.4 x 10^18` years

That number is absurdly larger than human history. It is also **not** a cryptographic proof. It is only a cartoon-scale metaphor for how bad blind guessing becomes if the final tables do not give a good reverse shortcut.

## Honest Limitations

The most important limitations should be said plainly:

- there is no formal proof that the original password cannot be recovered
- there is no formal proof that the byte twisters are superior to standard audited primitives
- there is no formal proof that the game and maze confusion layers create cryptographic hardness
- the ARIA-like path is an in-repo engineering component, not a formally validated ARIA library
- many of the strongest claims here are still author beliefs supported by tests, not theorems supported by published cryptanalysis

Bread should therefore be understood as:

- an ambitious deterministic confusion engine
- a table-heavy password-expansion backbone
- a recovery-aware archive experiment
- a serious engineering project with strong internal testing
- not a finished scientific conclusion

## Final Summary

Bread is trying to turn a small password into a very large, deterministically reproducible byte universe. It does that by mixing standard-inspired counter families, 16 new key-stack-and-salt byte twisters, puzzle-game confusion, maze confusion, and wide table generation. The project is unapologetically unusual. It is public but proprietary in posture. It is heavily tested but not formally proven. It was built in an apartment room, not a science lab.

That mix of ambition, rigor, honesty, and stubbornness is the reason the project exists.
