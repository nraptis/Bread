# Bread

Bread is a password-driven table-generation and byte-confusion library.

Its purpose is not to be "just another wrapper around a standard cipher." The project is built around a different idea:

- start from a user password
- expand that password into large deterministic seed regions
- build 22 large tables from several seed families
- run those tables through puzzle-game and maze-simulation confusion stages
- use the finished tables as a substrate for masking, mixing, archive workflows, and recovery-aware encryption designs

The original vision came from a gap in the archiver world.

Classic archive tools were shaped by an era where compression had to be a first-class concern. In 2025 and onward, that pressure is much lower for many real-world use cases. That opens a design space where more effort can be spent on recoverability, deterministic rebuildability, table generation, and byte confusion rather than compression ratios alone.

Recovery mode was treated as a first-class citizen in this project from the beginning.

## Important disclaimer

Bread is intended for entertainment, experimentation, and research-oriented use.

It is not presented as a formally audited cryptographic standard, a government-certified product, or a compliance-ready replacement for mature external crypto libraries.

At the same time, the author holds a very strong personal opinion:

- in the author's opinion, Bread is the backbone of the strongest cryptographic tool to date
- in the author's opinion, the final tables are not meaningfully traceable backward to recover the original password
- in the author's opinion, the byte twisters are materially stronger and more structurally interesting than many 1990s-era counter constructions

Those are author beliefs and design goals, not formal proofs.

## Open code, closed password

The project is designed with an openly stated philosophy:

- the code can be public
- the algorithms can be studied
- the table layouts can be understood
- the original user password should still remain the true secret

The author is not afraid of people seeing the code, because seeing the code is not the same thing as possessing the user's password.

## Authorized recovery

Bread is not meant to deny rightful recovery.

If a lawful owner, custodian, enterprise operator, government agency, or forensic team has the user's actual password, the system is designed so the tables can be regenerated deterministically and the protected data can be recovered through that password.

That is a major part of the project's philosophy:

- strong resistance to backward tracing without the password
- straightforward deterministic recovery with the password

## High-level architecture

Bread currently builds:

- 12 `L1` tables
- 6 `L2` tables
- 4 `L3` tables
- 22 total tables

Important sizes in the current code:

- `PASSWORD_EXPANDED_SIZE = 7680`
- `PASSWORD_BALLOONED_SIZE = 15360`
- `BLOCK_SIZE_L1 = 261120`
- `BLOCK_SIZE_L2 = 522240`
- `BLOCK_SIZE_L3 = 1044480`

Those tables are filled from:

- 3 AES-like table fills
- 1 ChaCha-like table fill
- 1 ARIA-like table fill
- 16 custom byte-twister fills

The result is a wide password-derived memory field rather than a single narrow keystream.

## The counters

Bread uses in-repo seed-generator families named:

- `AESCounter`
- `ChaCha20Counter`
- `ARIA256Counter`

The honest interpretation is:

- these are AES-like, ChaCha-like, and ARIA-like seeded stream families used inside Bread's pipeline
- `ARIA256Counter` is currently a placeholder-style ARIA-like path rather than a formally validated ARIA implementation
- these families provide multiple seed styles before the custom twister stage even begins

In the current codebase, the three AES-derived fills are intentionally separated so they do not collapse into byte-identical same-sized tables:

- one AES table is used directly
- one AES table is reversed
- one AES table is bit-inverted

That is a practical domain-separation measure inside the current implementation.

## The 16 byte twisters

The 16 production byte twisters live in:

- [`src/Tables/password_expanders/ByteTwister.hpp`](/Users/magneto/Desktop/Codex%20Playground/Bread/src/Tables/password_expanders/ByteTwister.hpp)
- [`src/Tables/password_expanders/ByteTwister.cpp`](/Users/magneto/Desktop/Codex%20Playground/Bread/src/Tables/password_expanders/ByteTwister.cpp)

The active production set is:

- `kType00`
- `kType01`
- `kType02`
- `kType03`
- `kType04`
- `kType05`
- `kType06`
- `kType07`
- `kType08`
- `kType09`
- `kType10`
- `kType11`
- `kType12`
- `kType13`
- `kType14`
- `kType15`

What these twisters do:

- operate on a full `PASSWORD_EXPANDED_SIZE` block
- use wrapped reads across the block
- repeatedly load 16-byte neighborhoods
- mix rows, columns, offsets, folds, rotates, XORs, and byte injection patterns
- apply more structure than a simple "increment counter and copy bytes" design

### Their origin

The author's account of the project is that these 16 production byte twisters were selected from a much larger search space of more than `700,000` candidate twisters.

The author's view is that these were the least-predictable survivors from that larger search.

The repo contains the final production twisters, not the full historical search archive.

### Author opinion on strength

In the author's opinion, these twisters are stronger and harder to predict than many 1990s-era counter-style designs because they use:

- extra mixing steps
- more complex matrix-like neighborhood logic
- wrapped reads over the whole expansion block
- chained block evolution
- repeated comparison pressure against AES-like, ARIA-like, and ChaCha-like counter families during development

The codebase supports that this system is more structurally elaborate than a plain counter stream.
The codebase does not provide a formal proof that these twisters are cryptographically superior to modern audited primitives.

## The password expanders

The password-expansion layer lives in:

- [`src/Tables/password_expanders/PasswordExpander.hpp`](/Users/magneto/Desktop/Codex%20Playground/Bread/src/Tables/password_expanders/PasswordExpander.hpp)
- [`src/Tables/password_expanders/PasswordExpander.cpp`](/Users/magneto/Desktop/Codex%20Playground/Bread/src/Tables/password_expanders/PasswordExpander.cpp)

The current expansion model is:

1. repeat-fill the user's password into one full `PASSWORD_EXPANDED_SIZE` source block
2. apply one byte twister to create expanded block `0`
3. copy the previous block forward
4. twist again to create block `1`
5. keep evolving the prior block until enough output exists

That means the password is not just hashed once and forgotten. It becomes the start of a deterministic evolving block chain.

### One simple example

Take a password like:

`apple wolf blazer`

The mental model is:

1. repeat that string until one full `PASSWORD_EXPANDED_SIZE` block is filled
2. run one selected twister such as `kType00`
3. that produces expanded block `0`
4. copy block `0`
5. twist again to produce block `1`
6. copy block `1`
7. twist again to produce block `2`

So the chain is:

`password -> repeated source block -> twist -> block 0 -> twist -> block 1 -> twist -> block 2 -> ...`

This is one reason the author talks about non-retraceability. The final data is many deterministic transformations away from the original human-readable password.

## Honest analysis

These are the most important claims, stated as honestly as possible.

### 1. The user's original password cannot be recovered after these operations complete

- This is the design goal of the project.
- The tables, game stages, and maze stages move the data far away from a human-readable password string.
- The code does not include a reverse path that reconstructs the original password from finished tables.
- The project does not provide a formal mathematical proof that password recovery is impossible.
- The author believes the original password is not practically recoverable from finished outputs without already knowing the password.

### 2. The user's password is expanded with impossible-to-reverse logic

- The expansion logic is multi-step and stateful, not a simple direct copy.
- The byte twisters repeatedly mix neighborhoods and evolve whole blocks.
- Later blocks are generated from prior expanded blocks rather than from the raw password alone.
- The code strongly supports the statement that reversing the pipeline is nontrivial.
- The word "impossible" should be understood here as a design claim and author opinion, not as a proven theorem.

### 3. Using these tables for masked byte operations can produce difficult-to-unweave encryption when combined with other ciphers

- This is a reasonable engineering claim about layering confusion on top of other crypto operations.
- A table-driven masked byte XOR or masked byte XOR with a noise table can make byte origin stories harder to narrate.
- When combined with external ciphers, these tables can add extra deterministic structure, hopping, and data-dependent confusion.
- The repo does not currently contain a formal cryptanalysis proving the combined construction is secure.
- The project is best understood as a strong confusion substrate, not as a substitute for standard audited cryptographic review.

### 4. The result of the game-board makes it statistically impossible to derive the original seed, aside from the histogram

- The game-board does substantially scramble byte position, byte emission order, and local causality.
- Output bytes are emitted when tiles are matched and committed, not simply copied out in seed order.
- Moves, cascades, power-ups, and rare events all push the output further away from a simple positional mapping.
- Histogram information can still survive because byte values themselves are reused and re-emitted.
- "Statistically impossible" is the author's intended claim, but the codebase does not provide a formal statistical proof.

### 5. The result of the maze makes it statistically impossible to derive the original seed, aside from the histogram

- The maze stage repaints and flushes walkable cells based on topology, robot motion, predator motion, respawns, and special events.
- Output bytes come from simulation history rather than a direct positional replay of the seed.
- The resulting byte stream is meaningfully separated from the original walkable-cell order.
- Histogram-style information can still leak in the broad sense that the same byte values continue to exist in the system.
- As with the game stage, the impossibility language is best read as design intent and author opinion, not a formal proof.

## Why byte confusion matters

Bread uses the phrase "byte confusion" to describe transforms that make a finished byte harder to explain as a direct derivative of a single input position.

That matters because a plain expanded password buffer is still, at the end of the day, just transformed bytes. A confusion layer tries to break the reverse narrative:

- where exactly did this byte come from
- what local neighborhood influenced it
- why was it emitted now and not earlier
- what simulation event caused this output order

Bread has two large confusion families:

- game-based byte confusion
- maze-based byte confusion

## Puzzle-game byte confusion

Puzzle-game byte confusion uses a `PASSWORD_EXPANDED_SIZE` seed window and turns it into an `8 x 8` board of tiles.

Each tile has:

- a tile type
- a stored byte payload
- an optional power-up type

The board then runs moves, matches, cascades, topples, rare events, and re-spawns until the output buffer has been filled.

The important point is this:

the emitted bytes are not copied out by original position. They are emitted when matched tiles are committed and removed from the board.

That makes the output feel more like simulation residue than a neat permutation.

### Why the puzzle stage is useful

- it damages direct byte-position traceability
- it introduces stateful move logic instead of one-pass arithmetic
- small seed changes can alter move selection, cascades, power-up triggers, and emission order
- output bytes are shaped by board history, not just by adjacency math

### The 16 puzzle modes

The board engine combines:

- move families: `tap`, `swap`, `slide`
- match families: `streak`, `island`
- play policies: `greedy`, `random`, `first`

Representative names include:

- `StreakSwapGreedy`
- `StreakSlideGreedy`
- `IslandSwapGreedy`
- `IslandSlideGreedy`
- `StreakTapGreedy`
- `IslandTapGreedy`
- `StreakSwapRandom`
- `IslandSwapRandom`
- `StreakSwapFirst`
- `IslandSlideFirst`

### Example puzzle boards

These are illustrative board sketches, not exact runtime dumps.

Legend:

- `A B C D` are the four tile types
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

### Streak matching

`Streak` matching is the classic match style:

- three or more same-type tiles in a horizontal line match
- three or more same-type tiles in a vertical line match

That is the `kStreak` path in the board logic.

### Island matching

`Island` matching treats same-type blobs as connected components instead of simple lines.

If enough like-typed tiles touch orthogonally and form a connected region, that region can be marked as a match.

This produces a more blob-oriented board behavior than traditional match-three rows and columns.

### Guarantee match and guarantee no-match

Bread contains explicit board-correction logic in [`src/Tables/games/GamePlayDirector.cpp`](/Users/magneto/Desktop/Codex%20Playground/Bread/src/Tables/games/GamePlayDirector.cpp).

There are four important guarantee helpers:

- `GuaranteeMatchDoesExist`
- `GuaranteeMatchDoesNotExist`
- `GuaranteeMoveExistsForNewTiles_MatchDoesExist`
- `GuaranteeMoveExistsForNewTiles_MatchDoesNotExist`

What they do:

- flip tile types until a fresh board has at least one match when the mode wants an immediate match
- flip tile types until a fresh board has no match when the mode wants a clean board
- try to preserve the stronger property that a legal move exists after new tiles appear
- restart and randomize if earlier repair passes fail

So "guarantee match" means the board shaper keeps editing tile types until a valid match exists.
"Guarantee no-match" means it keeps editing until the board is clean of immediate matches while still trying to preserve playability.

### Puzzle power-ups

Power-up tiles are spawned from the seed and stored on individual tiles.

Current power-ups in the code:

- `ZoneBomb`: marks a `3 x 3` area around the triggered tile
- `Rocket`: clears the full row and full column through the tile
- `ColorBomb`: clears every tile of the same type as the center tile
- `CrossBomb`: clears diagonals radiating from the center
- `PlasmaBeamV`: clears three vertical columns centered on the tile
- `PlasmaBeamH`: clears three horizontal rows centered on the tile
- `PlasmaBeamQuad`: combines the horizontal and vertical plasma beams
- `Nuke`: clears a `5 x 5` square around the tile
- `CornerBomb`: clears four `4 x 4` corner regions
- `VerticalBombs`: clears every other column using a random parity
- `HorizontalBombs`: clears every other row using a random parity

These are real mechanics in [`src/Tables/games/GamePowerUp.cpp`](/Users/magneto/Desktop/Codex%20Playground/Bread/src/Tables/games/GamePowerUp.cpp).

### Puzzle special events

The current puzzle engine has two named rare events:

- `RiddlerAttack`
- `DragonAttack`

`RiddlerAttack`:

- swaps every adjacent row pair
- swaps every adjacent column pair
- effectively scrambles the board geometry in-place

`DragonAttack`:

- walks the board
- randomly marks a large portion of non-matched tiles for removal
- forces a chaotic flush-and-topple situation

There is no third named puzzle special event in the current code.

### How the puzzle games work bytes

This is the important byte story:

1. a `PASSWORD_EXPANDED_SIZE` seed buffer is loaded
2. tile types are derived from seed-driven type bytes
3. each tile also stores a real byte payload from the seed
4. when a match is committed, the matched tiles' stored bytes are enqueued into the result buffer
5. power-ups can mark extra tiles
6. topples and respawns bring in fresh seed-driven tiles
7. cascades continue changing emission order

So the output is produced by:

- match timing
- board topology
- move policy
- cascade order
- special events
- power-up activation

not by simply reading the seed window left-to-right.

### Puzzle failure behavior

The board has recovery logic for stuck states and repeated failure to make output progress.

If the board cannot keep making progress after repeated rescue attempts, it eventually falls back to an apocalypse path that fills the result buffer from the seed and reverses it.

That fallback exists for deterministic completion, not as the preferred confusion path.

## Maze-based byte confusion

Maze-based byte confusion works on a larger `PASSWORD_BALLOONED_SIZE` seed window.

The maze stage builds a walkable maze, paints walkable cells from the seed, populates characters, and then lets the simulation repaint and flush bytes based on motion and events.

This creates a very different kind of confusion from the puzzle board.

The puzzle board is about matches and cascades.
The maze is about topology, pursuit, respawn, repainting, and local disruption.

### Why the maze stage is useful

- it ties bytes to map topology rather than to board matches
- it introduces moving agents with competing goals
- it repeatedly repaints and flushes walkable cells
- it makes output order depend on motion, collisions, captures, and special events

### The maze families

The code currently supports three generation families:

- `Custom`
- `Prim`
- `Kruskal`

These family names are visible in [`src/Tables/maze/MazePolicy.hpp`](/Users/magneto/Desktop/Codex%20Playground/Bread/src/Tables/maze/MazePolicy.hpp).

### Example maze sketches

These are representative illustrations, not exact runtime dumps.

Legend:

- `#` wall
- `.` open path
- `R` robot
- `C` cheese
- `D` dolphin
- `S` shark

Example: `Custom`

```text
#################
#R....#...#....C#
#.###.#.#.#.###.#
#...#...#...#...#
###.#####.###.#.#
#...#...D...#.#.#
#.###.#####.#.#.#
#.....#...#...S.#
#################
```

Example: `Prim`

```text
#################
#R#...#.....#...#
#.#.#.#.###.#.#.#
#...#...#.#...#.#
###.#####.#.###.#
#...#...#.#...#.#
#.###.#.#.###.#.#
#C....#...D..S#.#
#################
```

Example: `Kruskal`

```text
#################
#R..#...#...#..C#
###.#.#.#.#.#.###
#...#.#...#.#...#
#.###.#####.###.#
#.#...#D..#...#.#
#.#.###.#.###.#.#
#S#.....#.....#.#
#################
```

### The maze actors

The maze stage uses four named actor types:

- `Robot`
- `Cheese`
- `Dolphin`
- `Shark`

Their basic roles are:

- `Robot`: the main collector. Robots path toward cheese and try to win by reaching it.
- `Cheese`: the target object. When cheese is captured, it is marked and later respawned.
- `Dolphin`: a roaming disruptor. Dolphins can steal cheese before robots collect it.
- `Shark`: a predator. Sharks can kill robots and can also collide with dolphins.

### Deaths, victories, and lives

The maze does not use "lives" like a campaign game. It uses a repeated pulse model.

In the current implementation:

- a robot can become victorious by reaching nearby cheese
- a robot can die from shark collision
- a dolphin can die from shark collision
- a shark can die in robot or dolphin collisions
- cheese is captured and then respawned
- dead or completed actors are respawned into the simulation

So "life" in the maze means:

- an active run for that entity inside the current pulse
- followed by death or victory
- followed by deterministic respawn if the simulation continues

That is why the runtime statistics track counts such as:

- robot victories
- robot deaths
- dolphin deaths
- dolphin cheese steals

### Maze power-ups

The current code defines four maze power-up labels:

- `Invincible`: the protective power-up class
- `Magnet`: the attraction or pull-oriented power-up class
- `SuperSpeed`: the fast-traversal power-up class
- `Teleport`: the abrupt repositioning power-up class

What the current implementation definitely does:

- randomly places these power-up labels on walkable cells
- lets a robot collect a power-up from its nearby `3 x 3` neighborhood
- stores the collected type on the robot
- keeps it for a timed duration

Important honest note:

in the current code, these four power-up types are collected and timed, but they are not yet given separate downstream behavior branches. In other words, the names exist, the robot can hold them, and the state is tracked, but the movement logic does not yet branch into four distinct mechanical effects.

So the current truth is:

- the power-up names and categories are real
- the pickup and timed-hold behavior is real
- the distinct per-type gameplay effects are not fully wired yet

### Maze special events

The maze stage has three rare-event families:

- `StarBurst`
- `ChaosStorm`
- `CometTrails`

`StarBurst`:

- sweeps broadly across the maze
- repaints or flushes most non-wall cells
- deliberately avoids the immediate `3 x 3` collect boxes around living robots

`ChaosStorm`:

- centers on living sharks
- repaints or flushes `3 x 3` neighborhoods around them
- turns shark zones into local disruption fields

`CometTrails`:

- chooses rows or columns
- repaints or flushes along those full lines
- exists in horizontal and vertical forms in the code

For user-facing discussion, it is reasonable to think of horizontal and vertical comet trails as two directions of the same event family.

### How the mazes work bytes

This is the byte story for the maze stage:

1. a `PASSWORD_BALLOONED_SIZE` seed buffer is loaded
2. the maze topology is built
3. walkable cells are painted from seed bytes
4. robots, cheese, sharks, and dolphins move through the walkable region
5. cells are repainted from seed or flushed into the result buffer during movement and events
6. captures, deaths, respawns, and special events keep changing which cells are emitted and when

So the output bytes come from:

- topology
- repainting
- flushing
- entity movement
- predator collisions
- cheese capture
- special-event sweeps

not from a direct replay of original seed positions.

### Maze failure behavior

Like the puzzle stage, the maze stage has a deterministic fallback.

If the simulation cannot keep making valid progress, it can fall back to an apocalypse path that writes the seed into the result buffer and reverses it.

Again, that exists as a completion guarantee, not as the intended high-value confusion path.

## Why games and mazes are valuable confusion layers

The best way to think about Bread's game and maze layers is not "they replace audited ciphers."

The better way to think about them is:

- they are deterministic confusion engines
- they create long, password-derived, stateful byte histories
- they add causality that is harder to narrate backward
- they make byte origin depend on simulation state instead of simple arithmetic alone

The practical benefits are:

- more distance between the final output and the original password
- less obvious positional inheritance
- more opportunities to interleave with masking and table-hopping designs
- a substrate that is interesting for recoverable archive systems, byte masking, and entertainment-focused crypto experiments

## Non-recoverability and non-retraceability

Bread is built around two ideas:

- without the password, the author believes the final tables should not be realistically traceable backward to the original password
- with the password, the system should be reproducible and recoverable by the rightful user

That is why the project combines:

- password expansion
- counter families
- 16 byte twisters
- table hopping
- puzzle confusion
- maze confusion

The design tries to move the finished output far enough away from the original password that open code does not become open recovery.

## Combined use with other ciphers

Bread is especially interesting as a library layer when used to drive:

- masked byte XOR
- masked byte XOR with a second noise table
- table-driven byte substitution
- deterministic stream shaping around other ciphers
- recovery-aware archive workflows

In that role, Bread is not pretending to be a formal replacement for external cryptographic review.
It is offering a large, deterministic, password-driven confusion substrate that can make combined systems harder to unwind.

## Project posture

The strongest honest summary of Bread is this:

- it is ambitious
- it is original
- it is heavily opinionated
- it is open about its goals
- it is willing to say that recovery matters
- it is willing to say that code visibility is acceptable
- it is willing to use custom deterministic confusion engines where mainstream tools usually stop early

And the strongest positive summary is this:

Bread is trying to build a recoverable, password-centered, table-heavy, byte-confusion backbone for a new class of archive and encryption tooling.

That vision is the reason the project exists.
