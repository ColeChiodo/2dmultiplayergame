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

## Next Steps

### Health System
- `int health`, `int maxHealth` per player
- On hitbox/hurtbox collision, subtract damage
- Flag attacks as "already connected" to prevent multi-hits per swing
    - But Plan for multi-hit attacks
- Display health bars on screen

### Hitstun / Blockstun States
- Add `STATE_HITSTUN`, `STATE_BLOCK` to the player state machine
- On hit: victim enters hitstun (can't act for X frames)
- On block (hold back while getting hit): reduced or zero damage, shorter stun
- *Without this, landing a hit has no observable effect*

### Refactor to `struct GameState`
- Replace scattered globals (`player`, `currentFrame`, `inputBuffer`) with a single `struct GameState`
- Trivially savable/restorable — one `memcpy` for rollback snapshots
- Makes step 7 (rollback) straightforward

### Rollback Netcode
- Save state ring buffer (snapshot per frame)
- On receiving remote input for an already-simulated frame, roll back to that frame and re-simulate with correct input
- Local prediction + resync

### Data-Driven Config (JSON)
- Port attack data, character stats, hitbox sizes into a separate JSON file
- Parse with cJSON at startup
- In-game editor: visual overlays that modify the in-memory structs, then save back to file
