# Bread

## Counters

The three counters in this repo are `ChaCha20Counter`, `AESCounter`, and `ARIA256Counter`.
Measured locally on this workspace with a one-off `-O3` 8 MiB generation harness, the speed order was:

| Counter | Approx. speed | Relative note |
| --- | ---: | --- |
| `ChaCha20Counter` | ~210 MB/s | Fastest of the remaining set |
| `AESCounter` | ~98 MB/s | Slower software AES-CTR |
| `ARIA256Counter` | ~49 MB/s | Slowest in this repo |

This ranking is about this codebase, not the algorithms in the abstract.
The benchmark target named `benchmark_counter_speed_comparison` is currently a placeholder, so the numbers above come from a direct local timing pass over the real counter implementations.
If you swap compilers, machines, or optimization flags, the exact numbers will move.

### Limitations vs real library versions

- `ChaCha20Counter` uses the normal 20-round ChaCha core, but it is still an in-repo implementation with no third-party audit, no SIMD/vector fast path, and no hardening claims.
- `AESCounter` is a straightforward software AES-256-CTR implementation, but it does not use AES-NI, does not claim side-channel resistance, and should not be treated as a drop-in replacement for a vetted crypto library.
- `ARIA256Counter` is explicitly not a real ARIA implementation here; its block core is a SHA-256-based placeholder, so its behavior and security are not representative of true ARIA-256 CTR mode.

### Cryptographic security read

- `ChaCha20Counter`: the strongest design in this repo on paper, because the core primitive is standard ChaCha20 and the seed derivation uses HKDF-SHA256. Even so, it still lacks the implementation assurance, review depth, misuse resistance, and constant-time confidence you would expect from a production crypto library.
- `AESCounter`: conceptually sound as AES-256 in CTR mode with HKDF-based key/IV derivation, but the implementation is homemade and not hardened. The biggest risk is not the AES design itself, but using an unaudited software implementation as if it had the guarantees of OpenSSL, libsodium, BoringSSL, or a platform crypto API.
- `ARIA256Counter`: do not treat it as cryptographically secure ARIA. In this repo it is best described as a deterministic keyed stream generator with an ARIA-shaped API, not as a real ARIA construction.

## PasswordExpander Family

- `PasswordExpanderA`: Seeds an AES-backed workspace, then runs two full passes of byte-wise mixing over the expanded buffer. It leans on multiply, add, rotate, and nearby wraparound reads to spread local changes. It is the “AES + two-pass mixer” member of the family and is used by `MatchThreeTapStreaks`.
- `PasswordExpanderB`: Seeds a ChaCha20-backed workspace, then uses two forward passes with XOR-heavy and multiply-heavy byte updates. Its indexing jumps are more irregular than `A`, so it diffuses bytes through a less local access pattern. It is the ChaCha-flavored companion used by `MatchThreeTapIslands`.
- `PasswordExpanderC`: Writes through the buffer with a hopping index and a running carry-like state before doing a cleanup diffusion pass. That gives it a more sequential state-machine feel than the purely local mixers. It is used by `MatchThreeSwapStreaks`.
- `PasswordExpanderD`: Seeds from the repo’s `ARIA256Counter`, mixes backward through the buffer, then does an even/odd pair recombination pass. It uses nibble swapping more visibly than the other expanders, so its byte structure is a little more “shuffle then blend.” It is the ARIA-family member used by `MatchThreeSwapIslands`.
- `PasswordExpanderE`: Seeds an AES-backed workspace and performs a single dense pass with subtract, multiply, rotate, and XOR interactions. It is simpler than `A` because it skips the second cleanup pass and just commits the one main transform. It is the AES-side expander used by `MatchThreeSlideStreaks`.
- `PasswordExpanderF`: Seeds from ChaCha20 and is the busiest mixer in the family. It does a main pass, then three extra diffusion rounds, then a final even/odd recombination pass. It is the most layered expander in the set, even though it is not currently wired into one of the six games.
- `PasswordExpanderG`: Walks the buffer with a single hopping-index loop driven by a carry byte. That makes it compact and cheap compared with the heavier multi-pass expanders. It is used by `MatchThreeSlideIslands`.
- `PasswordExpanderH`: Seeds from the repo’s `ARIA256Counter`, mixes backward, and uses nibble swapping plus rotated neighbor blending. It is structurally simpler than `D` because it keeps only one main pass. It is the lighter ARIA-family variant and is currently unused by the six games.

## Games

- `MatchThreeTapStreaks`: The move is “tap one cell.” A tap removes a classic streak match, meaning a horizontal or vertical run of 3 or more equal tiles. It uses collapse-style gravity and `PasswordExpanderA`.
- `MatchThreeTapIslands`: The move is still “tap one cell.” A tap removes a connected island of 3 or more equal tiles, so blobs matter instead of only straight lines. It also uses collapse-style gravity and `PasswordExpanderB`.
- `MatchThreeSwapStreaks`: The move is “swap two adjacent tiles.” A move is legal only if the swap creates a classic streak match. It uses Bejeweled-style play flow and `PasswordExpanderC`.
- `MatchThreeSwapIslands`: The move is “swap two adjacent tiles.” A move is legal only if the swap creates an island-style connected match. It uses Bejeweled-style flow and `PasswordExpanderD`.
- `MatchThreeSlideStreaks`: The move is “slide a whole row or column” by 1 through 7 cells. A move is legal only if the resulting board contains a streak match. It uses `PasswordExpanderE`.
- `MatchThreeSlideIslands`: The move is “slide a whole row or column” by 1 through 7 cells. A move is legal only if the resulting board contains an island match. It uses `PasswordExpanderG`.

### Game diagrams

All six games are `8x8`, use `4` tile types, and differ by move family (`tap`, `swap`, `slide`) and match rule (`streak`, `island`).

#### `MatchThreeTapStreaks`

```text
tap one tile inside a straight run

[ A B C D ]
[ B A A A ]  <- tap any A in the run
[ C D B C ]
[ D C B D ]

remove matched row/column of 3+
            |
            v
gravity collapses downward
            |
            v
new tiles spawn from the top
```

#### `MatchThreeTapIslands`

```text
tap one tile inside a connected blob

[ A B C D ]
[ B A A D ]
[ C A B C ]  <- tap any connected A in the island
[ D C B D ]

remove connected island of 3+
            |
            v
gravity collapses downward
            |
            v
new tiles spawn from the top
```

#### `MatchThreeSwapStreaks`

```text
swap two neighbors to create a streak

before:               after:
[ C A D ]             [ C B D ]
[ A B A ]   swap      [ A A A ]  -> now row 2 is a streak
[ E F G ]  ------>    [ E F G ]
```

#### `MatchThreeSwapIslands`

```text
swap two neighbors to create an island

before:               after:
[ C A D ]             [ C B D ]
[ A B D ]   swap      [ A A D ]
[ E A G ]  ------>    [ E A G ]  -> now A forms an L-shaped island of 3
```

#### `MatchThreeSlideStreaks`

```text
rotate one whole row or column by 1..7 cells

row before: [ A B C D E F G H ]
slide left by 3
row after : [ D E F G H A B C ]

the move is legal only if the wrapped board
now contains a row/column streak of 3+
```

#### `MatchThreeSlideIslands`

```text
rotate one whole row or column by 1..7 cells

col before:    col after sliding down by 2:
[ A ]          [ G ]
[ B ]          [ H ]
[ C ]   --->   [ A ]
[ D ]          [ B ]
[ E ]          [ C ]
[ F ]          [ D ]
[ G ]          [ E ]
[ H ]          [ F ]

the move is legal only if the wrapped board
now contains a connected island of 3+
```

## Maze

Let `N = width * height`; in this repo `N = 32 * 32 = 1024`, so every algorithm is practically bounded, but the asymptotic costs are still useful.

Pure accessors such as `IsWall`, `IsEdge`, `PathLength`, `PathNode`, `ToIndex`, `ToX`, and `ToY` are all `O(1)`.

| Algorithm | Time complexity | Notes |
| --- | --- | --- |
| `FinalizeWalls()` | `O(N)` | One full scan to rebuild wall and walkable lists |
| `GetRandomWall()` | `O(1)` after finalize | `O(N)` only if it lazily triggers `FinalizeWalls()` first |
| `GetRandomWalkable()` | `O(1)` after finalize | Same cache behavior as `GetRandomWall()` |
| `Flush(x, y)` | `O(1)` | Single tile byte flush |
| `Flush()` | `O(N)` | Full scan of byte cells |
| `FillStackAllCoords()` | `O(N)` | Writes every coordinate once |
| `FindEdgeWalls(x, y)` | `O(N)` | Flood fill over one open component plus adjacent wall discovery |
| `BreakDownOneCellGroups()` | `O(N)` | Two bounded grid scans |
| `InitializeDisjointSets()` | `O(N * alpha(N))` | Grid scan plus near-constant-time unions |
| `FindSetRoot()` | `O(alpha(N))` amortized | Path-compressed DSU find |
| `UnionSetRoots()` | `O(alpha(N))` amortized | Union by rank + path compression |
| `OpenWallAndUnion()` | `O(alpha(N))` amortized | Opens one wall and unions with up to 4 neighbors |
| `EnsureSingleConnectedOpenGroup()` | `O(N^2 * alpha(N))` worst-case | Repeated whole-grid wall scans with incremental DSU updates |
| `GeneratePrims()` | `O(N)` | Frontier walls are pushed and removed a bounded number of times |
| `InitializeKruskals()` | `O(N)` | Initializes odd-cell groups and candidate walls |
| `ExecuteKruskals()` | `O(N^2)` | Current version rewrites group ids across the odd lattice on merges |
| `IsConnected_Slow()` | `O(N)` | DFS/BFS-style reachability check |
| `FindPath()` | `O(N log N)` worst-case | A* on a 4-neighbor grid with a binary heap |
| `ReconstructPath()` | `O(N)` | Walks parent pointers back once |
| `MazeRobotCheese::Build()` | `O(N^2 * alpha(N))` | Dominated by `EnsureSingleConnectedOpenGroup()` |
| `MazeDolphinSharks::Build()` | `O(N^2 * alpha(N))` | Same dominant cost |

Two practical notes matter here.
First, the recent DSU rewrite removed the older “full regroup every iteration” behavior, which used to be the main avoidable hotspot.
Second, Kruskal is still asymptotically worse than it needs to be in this repo because it rewrites `mGroupId` directly instead of using DSU there too.

### Example mazes

#### `RobotCheese`

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

#### `Prim`

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

#### `Kruskal`

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

## Security audit: `AES8192` vs `AES8192 -> game`

Measured locally in this workspace with a one-off harness over `32` independent trials of `8192` output bytes each.
The pipeline for the game rows was:

`password -> AESCounter -> 8192 bytes -> game -> 8192 bytes`

So this compares a raw `8192`-byte AES-backed stream against the same `8192` AES bytes post-processed by each game.

| Stream | Entropy (bits/byte) | Reduced chi^2 | Lag-1 equal rate | Lag-1 abs corr | Avalanche |
| --- | ---: | ---: | ---: | ---: | ---: |
| `AES8192` | 7.9775 | 0.9955 | 0.0040 | 0.0097 | 0.5002 |
| `AES8192 -> MatchThreeTapStreaks` | 7.9776 | 0.9945 | 0.0039 | 0.0077 | 0.5006 |
| `AES8192 -> MatchThreeTapIslands` | 7.9774 | 1.0029 | 0.0038 | 0.0078 | 0.4994 |
| `AES8192 -> MatchThreeSwapStreaks` | 7.9775 | 0.9993 | 0.0039 | 0.0084 | 0.5005 |
| `AES8192 -> MatchThreeSwapIslands` | 7.9774 | 1.0001 | 0.0039 | 0.0082 | 0.5001 |
| `AES8192 -> MatchThreeSlideStreaks` | 7.9774 | 0.9969 | 0.0038 | 0.0083 | 0.5001 |
| `AES8192 -> MatchThreeSlideIslands` | 7.9779 | 0.9786 | 0.0040 | 0.0093 | 0.5002 |

For these coarse black-box tests, every game stayed very close to the raw AES baseline.
On simple histogram, lag-1, and one-bit avalanche checks, none of the six games obviously damaged the byte-level randomness of an `8192`-byte AES stream.

That does **not** make the games cryptographically secure.
These are still ad hoc, branch-heavy, unaudited transforms built from small game state and custom mixers, not vetted cipher rounds.
At best they preserve “random-looking” output on these quick tests; they do not earn the trust level of “one more AES round.”

### Per-game security floor

| Mini-game | Security read |
| --- | --- |
| `MatchThreeTapStreaks` | Best case of the set on paper because its fast-rand expander is AES-backed, but it is still not a standard or audited cryptographic round. |
| `MatchThreeTapIslands` | ChaCha-backed expander, but still an ad hoc game transform rather than a cryptographic primitive. |
| `MatchThreeSwapStreaks` | Not a sound cryptographic strengthening layer: it is still an ad hoc game transform rather than a standard primitive. |
| `MatchThreeSwapIslands` | Not a sound cryptographic strengthening layer: its fast-rand expander depends on the repo’s placeholder `ARIA256Counter`. |
| `MatchThreeSlideStreaks` | AES-backed expander, but still only a novelty post-processing layer, not a substitute for another AES round. |
| `MatchThreeSlideIslands` | Not a sound cryptographic strengthening layer: it is still an ad hoc game transform rather than a standard primitive. |

### Bottom line

- If the goal is entertainment, novelty, or extra diffusion, the games are interesting and the quick stats do not show an obvious collapse.
- If the goal is actual cryptographic hardening, the security boundary should stay with vetted primitives like AES or ChaCha, not with the games.
- Comparing “one more AES round” against “AES output crunched through a game,” the extra AES round is the defensible security choice every time.
