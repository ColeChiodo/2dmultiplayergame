# 2D Fighting Game TODO List

## Done

- [x] Raylib window + game loop with fixed timestep (60fps sim)
- [x] Single player movement (A/D left/right, Space jump, S crouch)
- [x] Gravity and ground collision (`GROUND_HEIGHT`)
- [x] Player state machine: IDLE, WALK, JUMP, ATTACK
- [x] Basic attack state with duration timer (no attack type distinction)
- [x] Rising edge input detection (button press vs hold)
- [x] Circular input buffer (size 120, for rollback prep)
- [x] Save/Restore player state functions
- [x] Debug overlay: sim FPS, frame counter, timings
- [x] Dev controls: F1 toggle overlay, F2 reset, F3 pause, F4 step
- [x] Player 2: distinct key binds (arrows), different color
- [x] GameState struct: players[MAX_PLAYERS] + inputBuffer[MAX_PLAYERS]
- [x] Attack Data System: attacktype enum, startup, active, recovery counts. diff damage, hitboxes, knockback, etc.
- [x] Hit/Hurtboxes: debug drawing, aabb overlap detection
- [x] On-screen input display (live + history)
- [x] Camera2D: zooms out when players are further. clamped to stage limits

## Next Steps

### Working Combat

#### Health System
- `int health`, `int maxHealth` per player
- On hitbox/hurtbox collision, subtract damage
- Flag attacks as "already connected" to prevent multi-hits per swing
    - But plan for multi-hit attacks
- Display health bars on screen
- KO when health reaches 0

#### Hitstun
- Add `STATE_HITSTUN` to the player state machine
- On hit: victim enters hitstun (can't act for X frames)
- *Without this, landing a hit has no observable effect*

#### Blocking
- Hold back (← or → depending on facing) to block mids/highs
- Hold down-back to block lows
- Add `STATE_BLOCKSTUN` to state machine
- On block: reduced or zero damage, shorter stun than hitstun
- Chip damage on blocked specials (optional)

#### Knockdown + Wake-up
- On certain attacks (e.g. heavy), force a hard knockdown
- Character falls to ground, briefly invincible while grounded
- Standing back up uses a fixed recovery period
- Getup options: neutral, attack, roll forward/back
- Opens okizeme (oki) — pressure on wake-up

### Neutral

#### Pushback on Hit / Block
- On hit: both players slide apart by a knockback-driven amount
- On block: defender slides back
- Keeps spacing dynamic; enables whiff punishing

#### Stage Boundaries
- Left/right walls that stop player movement
- Corner carry: push opponent into the corner
- Optional: wall splat / wall bounce for combos

#### Dash / Backdash
- Double-tap forward/backward
- Forward dash: quick burst of speed in the direction faced
- Backdash: quick backward hop with optional brief invincibility
- Can cancel dash into attacks (dash momentum carries)

#### Throw
- Universal command (light+medium, or forward+heavy close up)
- Beats blocking — strike/throw/block triangle
- Fixed damage, no scaling
- Tech window: opponent breaks the throw with same input within ~7 frames
- Whiff animation if no one is in range

### Foundation Complete

#### Air Options
- Air blocking: hold back while airborne to block
- Air-to-air: normal attacks work in the air
- Juggles: opponent in hitstun after knockup can be hit again
- Landing recovery: brief landing lag after falling from any height

#### Motion Inputs (Special Moves)
- Add `enum Motion` field to `AttackData` (e.g. `MOTION_NONE`, `MOTION_236`, `MOTION_214`, `MOTION_623`)
- Write a `DetectMotion()` function reading backwards through the input buffer within a time window (~15 frames)
- Normalize directions relative to `facingRight` so quarter-circle works on both sides
- On button press, check each attack using that button for motion requirement
- ~100-150 lines of new code, no refactoring needed

#### Round / Victory System
- Timer countdown (99s standard)
- Best-of-X rounds
- Round start: reset positions, brief pause
- Win screen / results display

### Polish & Netcode

#### Refactor to `struct GameState`
- Replace scattered globals with a single `struct GameState`
- Trivially savable/restorable — one `memcpy` for rollback snapshots
- Makes rollback straightforward

#### Data-Driven Config (JSON)
- Port attack data, character stats, hitbox sizes into a separate JSON file
- Parse with cJSON at startup
- In-game editor: visual overlays that modify in-memory structs, then save back to file

#### Rollback Netcode
- Save state ring buffer (snapshot per frame)
- On receiving remote input for an already-simulated frame, roll back to that frame and re-simulate with correct input
- Local prediction + resync

