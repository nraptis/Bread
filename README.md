# Bread

## Counters

The four counters in this repo are `MersenneCounter`, `ChaCha20Counter`, `AESCounter`, and `ARIA256Counter`.
Measured locally on this workspace with a one-off `-O3` 8 MiB generation harness, the speed order was:

| Counter | Approx. speed | Relative note |
| --- | ---: | --- |
| `MersenneCounter` | ~379 MB/s | Fastest by a wide margin |
| `ChaCha20Counter` | ~210 MB/s | Roughly half of Mersenne |
| `AESCounter` | ~98 MB/s | Slower software AES-CTR |
| `ARIA256Counter` | ~49 MB/s | Slowest in this repo |

This ranking is about this codebase, not the algorithms in the abstract.
The benchmark target named `benchmark_counter_speed_comparison` is currently a placeholder, so the numbers above come from a direct local timing pass over the real counter implementations.
If you swap compilers, machines, or optimization flags, the exact numbers will move.

### Limitations vs real library versions

- `MersenneCounter` is not a cryptographic counter at all; it is a deterministic MT-style PRNG with a large state and easy predictability once enough output is seen.
- `ChaCha20Counter` uses the normal 20-round ChaCha core, but it is still an in-repo implementation with no third-party audit, no SIMD/vector fast path, and no hardening claims.
- `AESCounter` is a straightforward software AES-256-CTR implementation, but it does not use AES-NI, does not claim side-channel resistance, and should not be treated as a drop-in replacement for a vetted crypto library.
- `ARIA256Counter` is explicitly not a real ARIA implementation here; its block core is a SHA-256-based placeholder, so its behavior and security are not representative of true ARIA-256 CTR mode.

### Cryptographic security read

- `MersenneCounter`: not cryptographically secure. It is fine for deterministic testing and cheap pseudorandomness, but it should be treated as broken for secrecy, forward secrecy, or adversarial use.
- `ChaCha20Counter`: the strongest design in this repo on paper, because the core primitive is standard ChaCha20 and the seed derivation uses HKDF-SHA256. Even so, it still lacks the implementation assurance, review depth, misuse resistance, and constant-time confidence you would expect from a production crypto library.
- `AESCounter`: conceptually sound as AES-256 in CTR mode with HKDF-based key/IV derivation, but the implementation is homemade and not hardened. The biggest risk is not the AES design itself, but using an unaudited software implementation as if it had the guarantees of OpenSSL, libsodium, BoringSSL, or a platform crypto API.
- `ARIA256Counter`: do not treat it as cryptographically secure ARIA. In this repo it is best described as a deterministic keyed stream generator with an ARIA-shaped API, not as a real ARIA construction.

## PasswordExpander Family

- `PasswordExpanderA`: Seeds an AES-backed workspace, then runs two full passes of byte-wise mixing over the expanded buffer. It leans on multiply, add, rotate, and nearby wraparound reads to spread local changes. It is the “AES + two-pass mixer” member of the family and is used by `MatchThreeTapStreaks`.
- `PasswordExpanderB`: Seeds a ChaCha20-backed workspace, then uses two forward passes with XOR-heavy and multiply-heavy byte updates. Its indexing jumps are more irregular than `A`, so it diffuses bytes through a less local access pattern. It is the ChaCha-flavored companion used by `MatchThreeTapIslands`.
- `PasswordExpanderC`: Seeds from `MersenneCounter`, then writes through the buffer with a hopping index and a running carry-like state before doing a cleanup diffusion pass. That gives it a more sequential state-machine feel than the purely local mixers. It is the Mersenne-based expander used by `MatchThreeSwapStreaks`.
- `PasswordExpanderD`: Seeds from the repo’s `ARIA256Counter`, mixes backward through the buffer, then does an even/odd pair recombination pass. It uses nibble swapping more visibly than the other expanders, so its byte structure is a little more “shuffle then blend.” It is the ARIA-family member used by `MatchThreeSwapIslands`.
- `PasswordExpanderE`: Seeds an AES-backed workspace and performs a single dense pass with subtract, multiply, rotate, and XOR interactions. It is simpler than `A` because it skips the second cleanup pass and just commits the one main transform. It is the AES-side expander used by `MatchThreeSlideStreaks`.
- `PasswordExpanderF`: Seeds from ChaCha20 and is the busiest mixer in the family. It does a main pass, then three extra diffusion rounds, then a final even/odd recombination pass. It is the most layered expander in the set, even though it is not currently wired into one of the six mini-games.
- `PasswordExpanderG`: Seeds from `MersenneCounter` and walks the buffer with a single hopping-index loop driven by a carry byte. That makes it compact and cheap compared with the heavier multi-pass expanders. It is the Mersenne-side expander used by `MatchThreeSlideIslands`.
- `PasswordExpanderH`: Seeds from the repo’s `ARIA256Counter`, mixes backward, and uses nibble swapping plus rotated neighbor blending. It is structurally simpler than `D` because it keeps only one main pass. It is the lighter ARIA-family variant and is currently unused by the six mini-games.

## Games

- `MatchThreeTapStreaks`: The move is “tap one cell.” A tap removes a classic streak match, meaning a horizontal or vertical run of 3 or more equal tiles. It uses collapse-style gravity and `PasswordExpanderA`.
- `MatchThreeTapIslands`: The move is still “tap one cell.” A tap removes a connected island of 3 or more equal tiles, so blobs matter instead of only straight lines. It also uses collapse-style gravity and `PasswordExpanderB`.
- `MatchThreeSwapStreaks`: The move is “swap two adjacent tiles.” A move is legal only if the swap creates a classic streak match. It uses Bejeweled-style play flow and `PasswordExpanderC`.
- `MatchThreeSwapIslands`: The move is “swap two adjacent tiles.” A move is legal only if the swap creates an island-style connected match. It uses Bejeweled-style flow and `PasswordExpanderD`.
- `MatchThreeSlideStreaks`: The move is “slide a whole row or column” by 1 through 7 cells. A move is legal only if the resulting board contains a streak match. It uses `PasswordExpanderE`.
- `MatchThreeSlideIslands`: The move is “slide a whole row or column” by 1 through 7 cells. A move is legal only if the resulting board contains an island match. It uses `PasswordExpanderG`.

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
