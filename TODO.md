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
- [x] Collision Boxes: players push each other. cannot cross
- [x] Start of Animation Strips: per anim frame collision and hurt boxes
- [ ] Health System: health/maxhealth, on hit/hurtbox collision, subtract dmg, simple healthbar, KO when hp=0
- [x] Hitstun: hit player cant act for x frames
- [x] Debug State Timeline: visual display of each players current state on a timeline
- [x] Auto face opponent
- [x] STATE_CROUCH: crouch on holding 2. framework for crouch attacks.
- [ ] STATE_BLOCK: block attacks. added bools overhead and low on attacks. 
- [x] Input Buffer: buffer up to 6 frames in advance. buffer during blockstun, attack, etc.
- [x] Throw: Chord M+H to throw. Throw based on collison box.
- [x] Knockdown and Wakeup options: certain attacks cause knockdown (rn, heavy and throw). options, roll/tech (direction or neutral), and universal getup attack.

## Next Steps

### Working Combat

#### Health System
- KO when health reaches 0

#### Blocking
- Chip damage on blocked specials 
- Tune Airblock Behavior

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

