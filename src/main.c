#include "raylib.h"
#include <stdio.h>
#include <math.h>

#define GRAVITY 1800.0f
#define DT (1.0f / 60.0f)
#define GROUND_HEIGHT 450

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480
#define STAGE_LEFT    0.0f
#define STAGE_RIGHT   (SCREEN_WIDTH * 2.5f)
#define MAX_DISTANCE  (SCREEN_WIDTH * 1.5f)

#define MAX_PLAYERS 2
#define INPUT_BUFFER_SIZE 120
#define INPUT_HISTORY_SIZE 10
#define STATE_HISTORY_SIZE 60

#define MAX_HURTBOXES 4
#define MAX_HITBOXES 4
#define MAX_HITBOX_STRIPS 8
#define MAX_HITBOXES_PER_STRIP 4
#define MAX_COLLISION_BOXES 4
#define MAX_COLLISION_BOXES_PER_STRIP 4
#define MAX_COLLISION_STRIPS 8
#define MAX_HURTBOX_STRIPS 8

struct InputState {
    bool left;
    bool right;
    bool up;
    bool down;

    bool light;
    bool medium;
    bool heavy;
};

struct InputHistoryEntry {
    struct InputState state;
    int duration;
};

struct Rect {
	float offsetX, offsetY;
	float width, height;
};

static bool AABB(float ax, float ay, float aw, float ah,
                 float bx, float by, float bw, float bh) {
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

struct Hitbox {
    struct Rect rect;
};

struct CollisionBox {
    struct Rect rect;
};

struct AnimationStrip {
    int startFrame;
    int endFrame;
    int collisionBoxCount;
    struct CollisionBox collisionBoxes[MAX_COLLISION_BOXES_PER_STRIP];
    int hurtboxCount;
    struct Rect hurtboxes[MAX_HURTBOXES];
};

#define MAX_ANIMATION_STRIPS 8

struct AnimationStrips {
    int stripCount;
    struct AnimationStrip strips[MAX_ANIMATION_STRIPS];
};

struct HitboxStrip {
    int startFrame;
    int endFrame;
    int hitboxCount;
    struct Hitbox hitboxes[MAX_HITBOXES_PER_STRIP];
    float damage;
    float knockbackX, knockbackY;
    int hitstun;
	int blockstun;
};

enum AttackType { ATTACK_LIGHT, ATTACK_MEDIUM, ATTACK_HEAVY, ATTACK_COUNT };

struct AttackData {
    int startup, active, recovery;
    struct HitboxStrip strips[MAX_HITBOX_STRIPS];
    int stripCount;
    struct AnimationStrips animStrips;
    float moveX, moveY;
};

struct Character {
    const char* name;
    float walkSpeed;
    float jumpStrength;
    float width;
    float height;
	float maxHealth;
    struct AnimationStrips idleStrips;
    struct AnimationStrips walkStrips;
    struct AnimationStrips jumpStrips;
    struct AttackData attacks[ATTACK_COUNT];
};

static struct Character character1 = {
    .name = "Character 1",
    .walkSpeed = 300.0f,
    .jumpStrength = 800.0f,
    .width = 75.0f,
    .height = 150.0f,
	.maxHealth = 10000.0f,
    .idleStrips = {
        .stripCount = 1,
        .strips = {
            [0] = {
                .startFrame = 0, .endFrame = 0,
                .collisionBoxCount = 1,
                .collisionBoxes = {
                    [0] = { .rect = { .offsetX = -22.0f, .offsetY = -90.0f, .width = 44.0f, .height = 85.0f } },
                },
                .hurtboxCount = 1,
                .hurtboxes = {
                    [0] = { .offsetX = -37.5f, .offsetY = -150.0f, .width = 75.0f, .height = 150.0f },
                },
            },
        },
    },
    .walkStrips = {
        .stripCount = 1,
        .strips = {
            [0] = {
                .startFrame = 0, .endFrame = 0,
                .collisionBoxCount = 1,
                .collisionBoxes = {
                    [0] = { .rect = { .offsetX = -22.0f, .offsetY = -90.0f, .width = 44.0f, .height = 85.0f } },
                },
                .hurtboxCount = 1,
                .hurtboxes = {
                    [0] = { .offsetX = -37.5f, .offsetY = -150.0f, .width = 75.0f, .height = 150.0f },
                },
            },
        },
    },
    .jumpStrips = {
        .stripCount = 1,
        .strips = {
            [0] = {
                .startFrame = 0, .endFrame = 0,
                .collisionBoxCount = 1,
                .collisionBoxes = {
                    [0] = { .rect = { .offsetX = -22.0f, .offsetY = -90.0f, .width = 44.0f, .height = 85.0f } },
                },
                .hurtboxCount = 1,
                .hurtboxes = {
                    [0] = { .offsetX = -37.5f, .offsetY = -150.0f, .width = 75.0f, .height = 150.0f },
                },
            },
        },
    },
    .attacks = {
        [ATTACK_LIGHT] = {
            .startup = 2, .active = 5, .recovery = 3,
            .moveX = 0.0f, .moveY = 0.0f,
            .stripCount = 1,
            .strips = {
                [0] = {
                    .startFrame = 2, .endFrame = 6,
                    .hitboxCount = 1,
                    .hitboxes = {
                        [0] = { .rect = { .offsetX = 37.5f, .offsetY = -90.0f, .width = 45.0f, .height = 30.0f } },
                    },
                    .damage = 100,
                    .knockbackX = 100.0f, .knockbackY = 0.0f,
                    .hitstun = 10,
					.blockstun = 9,
                },
            },
            .animStrips = {
                .stripCount = 1,
                .strips = {
                    [0] = {
                        .startFrame = 0, .endFrame = 9,
                        .collisionBoxCount = 1,
                        .collisionBoxes = {
                            [0] = { .rect = { .offsetX = -22.0f, .offsetY = -90.0f, .width = 44.0f, .height = 85.0f } },
                        },
                        .hurtboxCount = 1,
                        .hurtboxes = {
                            [0] = { .offsetX = -37.5f, .offsetY = -150.0f, .width = 75.0f, .height = 150.0f },
                        },
                    },
                },
            },
        },
        [ATTACK_MEDIUM] = {
            .startup = 5, .active = 5, .recovery = 8,
            .moveX = 0.0f, .moveY = 0.0f,
            .stripCount = 1,
            .strips = {
                [0] = {
                    .startFrame = 5, .endFrame = 9,
                    .hitboxCount = 1,
                    .hitboxes = {
                        [0] = { .rect = { .offsetX = 67.5f, .offsetY = -60.0f, .width = 52.5f, .height = 37.5f } },
                    },
                    .damage = 200,
                    .knockbackX = 150.0f, .knockbackY = 0.0f,
                    .hitstun = 15,
					.blockstun = 12,
                },
            },
            .animStrips = {
                .stripCount = 1,
                .strips = {
                    [0] = {
                        .startFrame = 0, .endFrame = 17,
                        .collisionBoxCount = 1,
                        .collisionBoxes = {
                            [0] = { .rect = { .offsetX = -22.0f, .offsetY = -90.0f, .width = 44.0f, .height = 85.0f } },
                        },
                        .hurtboxCount = 1,
                        .hurtboxes = {
                            [0] = { .offsetX = -37.5f, .offsetY = -150.0f, .width = 75.0f, .height = 150.0f },
                        },
                    },
                },
            },
        },
        [ATTACK_HEAVY] = {
            .startup = 8, .active = 10, .recovery = 10,
            .moveX = 300.0f, .moveY = 0.0f,
            .stripCount = 1,
            .strips = {
                [0] = {
                    .startFrame = 8, .endFrame = 17,
                    .hitboxCount = 1,
                    .hitboxes = {
                        [0] = { .rect = { .offsetX = 75.0f, .offsetY = -45.0f, .width = 60.0f, .height = 45.0f } },
                    },
                    .damage = 400,
                    .knockbackX = 300.0f, .knockbackY = -300.0f,
                    .hitstun = 25,
					.blockstun = 12,
                },
            },
            .animStrips = {
                .stripCount = 1,
                .strips = {
                    [0] = {
                        .startFrame = 0, .endFrame = 27,
                        .collisionBoxCount = 1,
                        .collisionBoxes = {
                            [0] = { .rect = { .offsetX = -22.0f, .offsetY = -90.0f, .width = 44.0f, .height = 85.0f } },
                        },
                        .hurtboxCount = 1,
                        .hurtboxes = {
                            [0] = { .offsetX = -37.5f, .offsetY = -150.0f, .width = 75.0f, .height = 150.0f },
                        },
                    },
                },
            },
        },
    },
};

enum PlayerState {
    STATE_IDLE,
    STATE_WALK,
    STATE_JUMP,
    STATE_ATTACK,
	STATE_HITSTUN,
    STATE_BLOCKSTUN
};

enum DispState {
    DISP_IDLE,
    DISP_WALK,
    DISP_JUMP,
    DISP_ATTACK_STARTUP,
    DISP_ATTACK_ACTIVE,
    DISP_ATTACK_RECOVERY,
    DISP_HITSTUN,
    DISP_BLOCKSTUN,
};

struct Player {
    struct Character* character;

    float x, y;
    float velocityX, velocityY;
    bool grounded;

    enum PlayerState state;
    int stateTimer;

	bool facingRight;
	int attackType;
    int attackFrame;

	float health;

	bool hitConnected[MAX_HITBOX_STRIPS];
};

static void WorldRect(struct Player* p, struct Rect* r, float* outX, float* outY) {
    *outX = p->x + (p->facingRight ? r->offsetX : -r->offsetX - r->width);
    *outY = p->y + r->offsetY;
}

static struct AnimationStrips* GetCurrentAnimStrips(struct Player* p) {
    switch (p->state) {
        case STATE_IDLE:   return &p->character->idleStrips;
        case STATE_WALK:   return &p->character->walkStrips;
        case STATE_JUMP:   return &p->character->jumpStrips;
        case STATE_ATTACK: return &p->character->attacks[p->attackType].animStrips;
		case STATE_HITSTUN:
		case STATE_BLOCKSTUN: return &p->character->idleStrips;
    }
    return NULL;
}

static int GetCurrentAnimFrame(struct Player* p) {
    if (p->state == STATE_ATTACK) {
        struct AttackData* ad = &p->character->attacks[p->attackType];
        int total = ad->startup + ad->active + ad->recovery;
        return total - p->stateTimer;
    }
    return 0;
}

static int GetActiveCollisionBoxes(struct Player* p, struct CollisionBox* outBoxes, int maxOut) {
    struct AnimationStrips* anim = GetCurrentAnimStrips(p);
    if (!anim) return 0;

    int frame = GetCurrentAnimFrame(p);
    int count = 0;
    for (int s = 0; s < anim->stripCount && count < maxOut; s++) {
        struct AnimationStrip* strip = &anim->strips[s];
        if (frame >= strip->startFrame && frame <= strip->endFrame) {
            for (int b = 0; b < strip->collisionBoxCount && count < maxOut; b++) {
                outBoxes[count++] = strip->collisionBoxes[b];
            }
        }
    }
    return count;
}

static int GetActiveHurtboxes(struct Player* p, struct Rect* outBoxes, int maxOut) {
    struct AnimationStrips* anim = GetCurrentAnimStrips(p);
    if (!anim) return 0;

    int frame = GetCurrentAnimFrame(p);
    int count = 0;
    for (int s = 0; s < anim->stripCount && count < maxOut; s++) {
        struct AnimationStrip* strip = &anim->strips[s];
        if (frame >= strip->startFrame && frame <= strip->endFrame) {
            for (int h = 0; h < strip->hurtboxCount && count < maxOut; h++) {
                outBoxes[count++] = strip->hurtboxes[h];
            }
        }
    }
    return count;
}

static void GetCollisionHull(struct Player* p, float* outMinX, float* outMinY, float* outMaxX, float* outMaxY) {
    struct CollisionBox boxes[MAX_COLLISION_BOXES];
    int count = GetActiveCollisionBoxes(p, boxes, MAX_COLLISION_BOXES);

    if (count == 0) {
        *outMinX = p->x - p->character->width * 0.5f;
        *outMinY = p->y - p->character->height;
        *outMaxX = p->x + p->character->width * 0.5f;
        *outMaxY = p->y;
        return;
    }

    *outMinX = INFINITY;
    *outMinY = INFINITY;
    *outMaxX = -INFINITY;
    *outMaxY = -INFINITY;

    for (int i = 0; i < count; i++) {
        float wx, wy;
        WorldRect(p, &boxes[i].rect, &wx, &wy);
        float wr = wx + boxes[i].rect.width;
        float wb = wy + boxes[i].rect.height;
        if (wx < *outMinX) *outMinX = wx;
        if (wy < *outMinY) *outMinY = wy;
        if (wr > *outMaxX) *outMaxX = wr;
        if (wb > *outMaxY) *outMaxY = wb;
    }
}

static void ResolvePlayerCollision(struct Player* p0, struct Player* p1) {
    float min0x, min0y, max0x, max0y;
    float min1x, min1y, max1x, max1y;

    GetCollisionHull(p0, &min0x, &min0y, &max0x, &max0y);
    GetCollisionHull(p1, &min1x, &min1y, &max1x, &max1y);

    if (min0x < max1x && max0x > min1x && min0y < max1y && max0y > min1y) {
        float overlapX = fminf(max0x - min1x, max1x - min0x);
        float push = overlapX * 0.5f;

        float dir;
        if (min0x < min1x) {
            dir = -1.0f;
        } else if (min0x > min1x) {
            dir = 1.0f;
        } else {
            float stageCenter = (STAGE_LEFT + STAGE_RIGHT) * 0.5f;
            if (p0->y < p1->y) {
                dir = (p0->x < stageCenter) ? 1.0f : -1.0f;
            } else {
                dir = (p1->x < stageCenter) ? -1.0f : 1.0f;
            }
        }

        p0->x += dir * push;
        p1->x -= dir * push;
    }
}

struct GameInput {
	struct InputState players[MAX_PLAYERS];
};

struct GameState {
	struct Player players[MAX_PLAYERS];
	struct GameInput inputBuffer[INPUT_BUFFER_SIZE];
	int currentFrame;
};

struct SaveState {
    float x, y;
    float velocityX, velocityY;
    bool grounded;
    bool facingRight;
    enum PlayerState state;
    int stateTimer;
    int attackType;
    float health;
    bool hitConnected[MAX_HITBOX_STRIPS];
};

static struct GameState gameState;
static float simFPS = 0.0f;
static float simFrameTimeMs = 0.0f;

static struct InputHistoryEntry historyP1[INPUT_HISTORY_SIZE] = {0};
static struct InputHistoryEntry historyP2[INPUT_HISTORY_SIZE] = {0};
static int historyCountP1 = 0;
static int historyCountP2 = 0;
static bool showInputP1 = true;
static bool showInputP2 = false;
static bool showStateTimeline = true;

static enum DispState stateHistoryP1[STATE_HISTORY_SIZE];
static enum DispState stateHistoryP2[STATE_HISTORY_SIZE];

void InitGameState();
void InitPlayer(struct Player* p, float x, float y, struct Character* c);
void GameStep(struct Player* p, const struct InputState* input, const struct InputState* prevInput);
void SavePlayerState(const struct Player* p, struct SaveState* save);
void RestorePlayerState(struct Player* p, const struct SaveState* save);
struct InputState GatherInputs(void);
struct InputState GatherInputs2(void);
void RunOneSimStep(struct GameState* gs);

void SavePlayerState(const struct Player* player, struct SaveState* save) {
    save->x = player->x;
    save->y = player->y;
    save->velocityX = player->velocityX;
    save->velocityY = player->velocityY;
    save->grounded = player->grounded;
    save->facingRight = player->facingRight;
    save->state = player->state;
    save->stateTimer = player->stateTimer;
    save->attackType = player->attackType;
    save->health = player->health;
    for (int i = 0; i < MAX_HITBOX_STRIPS; i++) {
        save->hitConnected[i] = player->hitConnected[i];
    }
}

void RestorePlayerState(struct Player* player, const struct SaveState* save) {
    player->x = save->x;
    player->y = save->y;
    player->velocityX = save->velocityX;
    player->velocityY = save->velocityY;
    player->grounded = save->grounded;
    player->facingRight = save->facingRight;
    player->state = save->state;
    player->stateTimer = save->stateTimer;
    player->attackType = save->attackType;
    player->health = save->health;
    for (int i = 0; i < MAX_HITBOX_STRIPS; i++) {
        player->hitConnected[i] = save->hitConnected[i];
    }
}

void InitPlayer(struct Player* player, float x, float y, struct Character* character) {
    player->x = x;
    player->y = y;
    player->velocityX = 0.0f;
    player->velocityY = 0.0f;
    player->grounded = false;
    player->facingRight = true;
    player->state = STATE_IDLE;
    player->character = character;
    player->stateTimer = 0;
    player->attackType = ATTACK_LIGHT;
    player->attackFrame = 0;
	player->health = character->maxHealth;
    for (int i = 0; i < MAX_HITBOX_STRIPS; i++) {
        player->hitConnected[i] = false;
    }
}

void GameStep(struct Player* player, const struct InputState* input, const struct InputState* prevInput) {
    int dirX = 0;
    if (input->left && !input->right) {
        dirX = -1;
    } else if (input->right && !input->left) {
        dirX = 1;
    }

    if (dirX != 0) {
        player->facingRight = (dirX > 0);
    }

    bool lightPressed = input->light && !prevInput->light;
    bool mediumPressed = input->medium && !prevInput->medium;
    bool heavyPressed = input->heavy && !prevInput->heavy;

    switch (player->state) {
        case STATE_IDLE:
        case STATE_WALK:
            if (lightPressed || mediumPressed || heavyPressed) {
                player->attackType = lightPressed ? ATTACK_LIGHT
                                  : mediumPressed ? ATTACK_MEDIUM
                                  : ATTACK_HEAVY;
                struct AttackData* ad = &player->character->attacks[player->attackType];
                player->stateTimer = ad->startup + ad->active + ad->recovery;
                player->attackFrame = 0;
                for (int i = 0; i < MAX_HITBOX_STRIPS; i++) {
                    player->hitConnected[i] = false;
                }
                if (player->grounded) {
                    player->velocityX = 0.0f;
                }
                player->state = STATE_ATTACK;
                break;
            }

            player->velocityX = dirX * player->character->walkSpeed;
            player->state = (dirX != 0) ? STATE_WALK : STATE_IDLE;

            if (input->up && player->grounded) {
                player->velocityY = -player->character->jumpStrength;
                player->grounded = false;
                player->state = STATE_JUMP;
            }

            break;
        case STATE_JUMP:
            if (lightPressed || mediumPressed || heavyPressed) {
                player->attackType = lightPressed ? ATTACK_LIGHT
                                  : mediumPressed ? ATTACK_MEDIUM
                                  : ATTACK_HEAVY;
                struct AttackData* ad = &player->character->attacks[player->attackType];
                player->stateTimer = ad->startup + ad->active + ad->recovery;
                player->attackFrame = 0;
                for (int i = 0; i < MAX_HITBOX_STRIPS; i++) {
                    player->hitConnected[i] = false;
                }
                player->state = STATE_ATTACK;
                break;
            }

            if (player->grounded) {
                player->state = (dirX != 0) ? STATE_WALK : STATE_IDLE;
            }

            break;
        case STATE_ATTACK: {
			struct AttackData* ad = &player->character->attacks[player->attackType];
            int totalFrames = ad->startup + ad->active + ad->recovery;
            int elapsed = totalFrames - player->stateTimer;

            if (elapsed >= ad->startup && elapsed < ad->startup + ad->active) {
                if (ad->moveX != 0.0f) {
                    player->velocityX = player->facingRight ? ad->moveX : -ad->moveX;
                }
            } else if (player->grounded) {
                player->velocityX = 0.0f;
            }

            player->stateTimer--;
            player->attackFrame++;
            if (player->stateTimer <= 0) {
                player->state = player->grounded ? STATE_IDLE : STATE_JUMP;
            }

            break;
		}
		case STATE_HITSTUN:
		case STATE_BLOCKSTUN:
			if (player->grounded) {
                player->velocityX *= 0.85f;
                if (fabsf(player->velocityX) < 1.0f) player->velocityX = 0.0f;
            }


			player->stateTimer--;

			if (player->stateTimer <= 0) {
                player->state = player->grounded ? STATE_IDLE : STATE_JUMP;
            }

			break;
    }

    if (!player->grounded) {
        player->velocityY += GRAVITY * DT;
    }

    player->x += player->velocityX * DT;
    player->y += player->velocityY * DT;

    if (player->y >= GROUND_HEIGHT) {
        player->y = GROUND_HEIGHT;
        player->velocityY = 0.0f;
        player->grounded = true;
    } else {
        player->grounded = false;
    }

    float halfW = player->character->width * 0.5f;
    if (player->x < STAGE_LEFT + halfW) player->x = STAGE_LEFT + halfW;
    if (player->x > STAGE_RIGHT - halfW) player->x = STAGE_RIGHT - halfW;
}

struct InputState GatherInputs(void) {
    struct InputState input = {0};

    input.left   = IsKeyDown(KEY_A);
    input.right  = IsKeyDown(KEY_D);
    input.up     = IsKeyDown(KEY_SPACE);
    input.down   = IsKeyDown(KEY_S);

    input.light  = IsKeyDown(KEY_J);
    input.medium = IsKeyDown(KEY_K);
    input.heavy  = IsKeyDown(KEY_L);

    return input;
}

struct InputState GatherInputs2(void) {
    struct InputState input = {0};

    input.left   = IsKeyDown(KEY_LEFT);
    input.right  = IsKeyDown(KEY_RIGHT);
    input.up     = IsKeyDown(KEY_UP);
    input.down   = IsKeyDown(KEY_DOWN);

    input.light  = IsKeyDown(KEY_COMMA);
    input.medium = IsKeyDown(KEY_PERIOD);
    input.heavy  = IsKeyDown(KEY_SLASH);

    return input;
}

static bool InputStateEq(const struct InputState* a, const struct InputState* b) {
    return a->left == b->left && a->right == b->right && a->up == b->up && a->down == b->down
        && a->light == b->light && a->medium == b->medium && a->heavy == b->heavy;
}

void RunOneSimStep(struct GameState* gs) {
	struct GameInput frameInput;

    frameInput.players[0] = GatherInputs();
    frameInput.players[1] = GatherInputs2();
	
    int index = gs->currentFrame % INPUT_BUFFER_SIZE;
    gs->inputBuffer[index] = frameInput;

    for (int pl = 0; pl < MAX_PLAYERS; pl++) {
        struct InputState* cur = &frameInput.players[pl];
        struct InputHistoryEntry* hist = pl == 0 ? historyP1 : historyP2;
        int* count = pl == 0 ? &historyCountP1 : &historyCountP2;
        if (*count > 0 && InputStateEq(&hist[0].state, cur)) {
            hist[0].duration++;
        } else {
            if (*count < INPUT_HISTORY_SIZE) (*count)++;
            for (int h = *count - 1; h > 0; h--) {
                hist[h] = hist[h - 1];
            }
            hist[0].state = *cur;
            hist[0].duration = 1;
        }
    }

    struct GameInput emptyFrame = {0};
    struct GameInput* prevFrame = &emptyFrame;

    if (gs->currentFrame > 0) {
        int prevIndex = (gs->currentFrame - 1) % INPUT_BUFFER_SIZE;
        prevFrame = &gs->inputBuffer[prevIndex];
    }

    double stepStart = GetTime();

    for (int i = 0; i < MAX_PLAYERS; i++) {
        GameStep(&gs->players[i],
                 &gs->inputBuffer[index].players[i],
                 &prevFrame->players[i]);
    }

    int slot = gs->currentFrame % STATE_HISTORY_SIZE;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        struct Player* p = &gs->players[i];
        enum DispState ds;
        switch (p->state) {
            case STATE_IDLE:   ds = DISP_IDLE; break;
            case STATE_WALK:   ds = DISP_WALK; break;
            case STATE_JUMP:   ds = DISP_JUMP; break;
            case STATE_HITSTUN:   ds = DISP_HITSTUN; break;
            case STATE_BLOCKSTUN: ds = DISP_BLOCKSTUN; break;
            case STATE_ATTACK: {
                struct AttackData* ad = &p->character->attacks[p->attackType];
                int elapsed = p->attackFrame - 1;
                if (elapsed < ad->startup) ds = DISP_ATTACK_STARTUP;
                else if (elapsed < ad->startup + ad->active) ds = DISP_ATTACK_ACTIVE;
                else ds = DISP_ATTACK_RECOVERY;
                break;
            }
            default: ds = DISP_IDLE; break;
        }
        if (i == 0) stateHistoryP1[slot] = ds;
        else stateHistoryP2[slot] = ds;
    }

    bool playerWasHit[MAX_PLAYERS] = {false, false};
    float playerHitDamage[MAX_PLAYERS] = {0, 0};
    int playerHitstun[MAX_PLAYERS] = {0, 0};
    int playerBlockstun[MAX_PLAYERS] = {0, 0};
    float playerKbX[MAX_PLAYERS] = {0, 0};
    float playerKbY[MAX_PLAYERS] = {0, 0};

    for (int i = 0; i < MAX_PLAYERS; i++) {
        int opponent = (i == 0) ? 1 : 0;
        struct Player* attacker = &gs->players[i];

        if (attacker->state != STATE_ATTACK) continue;

        struct AttackData* ad = &attacker->character->attacks[attacker->attackType];
        int total = ad->startup + ad->active + ad->recovery;
        int elapsed = total - attacker->stateTimer;

        for (int s = 0; s < ad->stripCount; s++) {
            if (attacker->hitConnected[s]) continue;
            struct HitboxStrip* strip = &ad->strips[s];
            if (elapsed < strip->startFrame || elapsed > strip->endFrame) continue;

            struct Player* defender = &gs->players[opponent];
            for (int h = 0; h < strip->hitboxCount; h++) {
                float ax, ay;
                WorldRect(attacker, &strip->hitboxes[h].rect, &ax, &ay);
                float aw = strip->hitboxes[h].rect.width;
                float ah = strip->hitboxes[h].rect.height;

                struct Rect defenderHurtboxes[MAX_HURTBOXES];
                int defenderHurtboxCount = GetActiveHurtboxes(defender, defenderHurtboxes, MAX_HURTBOXES);
                for (int hb = 0; hb < defenderHurtboxCount; hb++) {
                    float dx, dy;
                    WorldRect(defender, &defenderHurtboxes[hb], &dx, &dy);
                    float dw = defenderHurtboxes[hb].width;
                    float dh = defenderHurtboxes[hb].height;

                    if (AABB(ax, ay, aw, ah, dx, dy, dw, dh)) {
                        attacker->hitConnected[s] = true;
                        playerWasHit[opponent] = true;
                        playerHitDamage[opponent] = strip->damage;
                        playerHitstun[opponent] = strip->hitstun;
                        playerBlockstun[opponent] = strip->blockstun;
                        playerKbX[opponent] = attacker->facingRight ? strip->knockbackX : -strip->knockbackX;
                        playerKbY[opponent] = strip->knockbackY;
                        goto next_strip;
                    }
                }
            }
            next_strip:;
        }
    }

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!playerWasHit[i]) continue;
        struct Player* p = &gs->players[i];
        bool blocked = false;
        if (!blocked) {
            int other = (i == 0) ? 1 : 0;
            printf("Player %d hit Player %d:\n\t%.0f - %.0f = %.0f\n", other + 1, i + 1, p->health, playerHitDamage[i], p->health - playerHitDamage[i]);
            fflush(stdout);
            p->health -= playerHitDamage[i];
            p->state = STATE_HITSTUN;
            p->stateTimer = playerHitstun[i];
            p->velocityX = playerKbX[i];
            p->velocityY = playerKbY[i];
        } else {
            p->state = STATE_BLOCKSTUN;
            p->stateTimer = playerBlockstun[i];
        }
    }


    ResolvePlayerCollision(&gs->players[0], &gs->players[1]);

    for (int i = 0; i < MAX_PLAYERS; i++) {
        struct Player* p = &gs->players[i];
        float halfW = p->character->width * 0.5f;
        if (p->x < STAGE_LEFT + halfW) p->x = STAGE_LEFT + halfW;
        if (p->x > STAGE_RIGHT - halfW) p->x = STAGE_RIGHT - halfW;
        if (p->y > GROUND_HEIGHT) p->y = GROUND_HEIGHT;
    }

    float dx = gs->players[0].x - gs->players[1].x;
    float dist = fabsf(dx);
    if (dist > MAX_DISTANCE) {
        float correction = (dist - MAX_DISTANCE) / 2.0f;
        if (dx > 0) {
            gs->players[0].x -= correction;
            gs->players[1].x += correction;
        } else {
            gs->players[0].x += correction;
            gs->players[1].x -= correction;
        }
    }

    gs->currentFrame++;
    double stepEnd = GetTime();
    simFrameTimeMs = (float)((stepEnd - stepStart) * 1000.0);
}

static void DrawHealthBars(struct GameState* gs, bool showNumbers) {
    int barWidth = 200;
    int barHeight = 20;
    int barY = 10;

    int p1BarX = 20;
    int p2BarX = SCREEN_WIDTH - 20 - barWidth;

    for (int i = 0; i < MAX_PLAYERS; i++) {
        struct Player* p = &gs->players[i];
        float ratio = p->health / p->character->maxHealth;
        if (ratio < 0.0f) ratio = 0.0f;

        Color fg = i == 0 ? RED : BLUE;
        int bx = i == 0 ? p1BarX : p2BarX;

        DrawRectangle(bx, barY, barWidth, barHeight, DARKGRAY);
        if (i == 0) {
            DrawRectangle(bx + barWidth - (int)(barWidth * ratio), barY, (int)(barWidth * ratio), barHeight, fg);
        } else {
            DrawRectangle(bx, barY, (int)(barWidth * ratio), barHeight, fg);
        }
        DrawRectangleLines(bx, barY, barWidth, barHeight, WHITE);

        if (showNumbers) {
            DrawText(TextFormat("%.0f / %.0f", p->health, p->character->maxHealth),
                     bx + 4, barY + 2, 14, WHITE);
        }
    }
}

static void DrawCurrentInput(struct InputState state, int x, int y, bool isP1) {
    int ds = 10, g = 3, cw = ds + g, ch = ds + g;
    Color on = isP1 ? (Color){200, 50, 50, 255} : (Color){50, 50, 200, 255};
    Color off = (Color){160, 160, 160, 200};

    DrawRectangle(x + cw, y, ds, ds, state.up ? on : off);
    DrawRectangle(x, y + ch, ds, ds, state.left ? on : off);
    DrawRectangle(x + cw, y + ch, ds, ds, state.down ? on : off);
    DrawRectangle(x + 2 * cw, y + ch, ds, ds, state.right ? on : off);

    int bx = x + 3 * cw + 6, by = y + ch - 6, bs = 12, bg = 4;
    Color dim = (Color){80, 80, 80, 200};
    DrawRectangleLines(bx, by, bs, bs, state.light ? PINK : dim);
    DrawText("L", bx + 2, by + 1, 9, state.light ? PINK : dim);
    DrawRectangleLines(bx + bs + bg, by, bs, bs, state.medium ? GREEN : dim);
    DrawText("M", bx + bs + bg + 2, by + 1, 9, state.medium ? GREEN : dim);
    DrawRectangleLines(bx + 2 * (bs + bg), by, bs, bs, state.heavy ? RED : dim);
    DrawText("H", bx + 2 * (bs + bg) + 2, by + 1, 9, state.heavy ? RED : dim);
}

static void DrawInputHistory(struct InputHistoryEntry* hist, int count, int x, int y) {
    for (int i = 0; i < count; i++) {
        struct InputState* s = &hist[i].state;
        int ry = y + i * 18;
        int ds = 6, g = 2, cw = ds + g, ch = ds + g;
        Color on = (Color){180, 180, 50, 255};
        Color off = (Color){120, 120, 120, 200};

        DrawRectangle(x + cw, ry, ds, ds, s->up ? on : off);
        DrawRectangle(x, ry + ch, ds, ds, s->left ? on : off);
        DrawRectangle(x + cw, ry + ch, ds, ds, s->down ? on : off);
        DrawRectangle(x + 2 * cw, ry + ch, ds, ds, s->right ? on : off);

        int tx = x + 3 * cw + 4 + 2;
        Color dim = (Color){80, 80, 80, 200};
        DrawText("L", tx, ry, 9, s->light ? PINK : dim); tx += 11;
        DrawText("M", tx, ry, 9, s->medium ? GREEN : dim); tx += 11;
        DrawText("H", tx, ry, 9, s->heavy ? RED : dim); tx += 11;

        DrawText(TextFormat("%df", hist[i].duration), tx, ry, 9, BLACK);
    }
}

static Color DispColor(enum DispState s) {
    switch (s) {
        case DISP_IDLE:             return (Color){180, 180, 180, 255};
        case DISP_WALK:             return (Color){ 80, 130, 255, 255};
        case DISP_JUMP:             return (Color){ 80, 220,  80, 255};
        case DISP_ATTACK_STARTUP:   return (Color){255, 255,  80, 255};
        case DISP_ATTACK_ACTIVE:    return (Color){255,  50,  50, 255};
        case DISP_ATTACK_RECOVERY:  return (Color){180,  50,  50, 255};
        case DISP_HITSTUN:          return (Color){255, 180,  40, 255};
        case DISP_BLOCKSTUN:        return (Color){200,  80, 255, 255};
        default:                    return (Color){100, 100, 100, 255};
    }
}

static void DrawStateTimeline(void) {
    int fw = 6;
    int barH = 18;
    int numFrames = STATE_HISTORY_SIZE;
    int barWidth = numFrames * fw;
    int barX = (SCREEN_WIDTH - barWidth) / 2;
    int barY = 130;

    for (int pl = 0; pl < MAX_PLAYERS; pl++) {
        int y = barY + pl * (barH + 16);
        enum DispState* hist = pl == 0 ? stateHistoryP1 : stateHistoryP2;

        DrawText(TextFormat("P%d", pl + 1), barX - 30, y + 2, 14, BLACK);

        for (int f = 0; f < numFrames; f++) {
            int x = barX + f * fw;
            Color c = DispColor(hist[f]);
            DrawRectangle(x, y, fw, barH, c);
        }

        DrawRectangle(barX + (gameState.currentFrame % STATE_HISTORY_SIZE) * fw, y, fw, barH, BLACK);
    }
}

int main(void) {
    // SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Cole's Fighting Game");

    RenderTexture2D target = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
	
	float stageCenter = (STAGE_LEFT + STAGE_RIGHT) / 2.0f;
	InitPlayer(&gameState.players[0], stageCenter - SCREEN_WIDTH * 0.25f, GROUND_HEIGHT, &character1);
	InitPlayer(&gameState.players[1], stageCenter + SCREEN_WIDTH * 0.25f, GROUND_HEIGHT, &character1);

    double accumulator = 0.0;
    double fpsTimer = 0.0;
    int stepsThisSecond = 0;

    int paused = 0;
    int doStep = 0;
    int showDebugOverlay = 1;
	
	Camera2D camera = { 0 };
	camera.offset = (Vector2){ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
	camera.target = (Vector2){ (gameState.players[0].x + gameState.players[1].x) / 2, SCREEN_HEIGHT / 2.0f };
	camera.zoom = 1.0f;

    while (!WindowShouldClose()) {
        double frameTime = GetFrameTime();
        if (frameTime > 0.1) frameTime = 0.1;
        float renderFrameTimeMs = (float)(frameTime * 1000.0);

        // Dev input handling
        if (IsKeyPressed(KEY_F1)) showDebugOverlay = !showDebugOverlay;
		if (IsKeyPressed(KEY_F2)) {
			float stageCenter = (STAGE_LEFT + STAGE_RIGHT) / 2.0f;
			InitPlayer(&gameState.players[0], stageCenter - SCREEN_WIDTH * 0.25f, GROUND_HEIGHT, &character1);
			InitPlayer(&gameState.players[1], stageCenter + SCREEN_WIDTH * 0.25f, GROUND_HEIGHT, &character1);
        }
        if (IsKeyPressed(KEY_F3)) paused = !paused;
        if (IsKeyPressed(KEY_F4)) doStep = 1;
        if (IsKeyPressed(KEY_F5)) showInputP1 = !showInputP1;
        if (IsKeyPressed(KEY_F6)) showInputP2 = !showInputP2;
        if (IsKeyPressed(KEY_F7)) showStateTimeline = !showStateTimeline;

        if (paused) {
            if (doStep) {
                RunOneSimStep(&gameState);
                doStep = 0;
            }
        } else {
            accumulator += frameTime;

            int stepsRun = 0;
            while (accumulator >= DT) {
                RunOneSimStep(&gameState);
                accumulator -= DT;
                stepsRun++;
            }

            stepsThisSecond += stepsRun;
            fpsTimer += frameTime;
            if (fpsTimer >= 1.0) {
                simFPS = (float)stepsThisSecond;
                stepsThisSecond = 0;
                fpsTimer -= 1.0;
            }
        }

		// Update Camera
		float midpointX = (gameState.players[0].x + gameState.players[1].x) / 2.0f;

		float baseDistance = SCREEN_WIDTH * 0.5f; // 320
		float distance = fabsf(gameState.players[0].x - gameState.players[1].x);
		float desiredZoom = fminf(1.0f, baseDistance / distance);
		desiredZoom = fmaxf(desiredZoom, 0.625f);
		
		// Lerp
		Vector2 desiredTarget = { midpointX, 450.0f - 210.0f / desiredZoom };

		float smoothSpeed = 2.0f;
		float t = 1.0f - expf(-smoothSpeed * GetFrameTime());
		camera.target.x += (desiredTarget.x - camera.target.x) * t;
		camera.target.y += (desiredTarget.y - camera.target.y) * t;
		camera.zoom += (desiredZoom - camera.zoom) * t;

		// Clamp Camera X
		float viewHalf = (SCREEN_WIDTH / 2.0f) / camera.zoom;
		float minTarget = STAGE_LEFT + viewHalf;
		float maxTarget = STAGE_RIGHT - viewHalf;
		
		if (minTarget < maxTarget) {
		    camera.target.x = fminf(fmaxf(camera.target.x, minTarget), maxTarget);
		} else {
		    camera.target.x = (STAGE_LEFT + STAGE_RIGHT) / 2.0f;
		}

		BeginDrawing();

            ClearBackground(BLACK);


            float scale = fminf((float)GetScreenWidth() / SCREEN_WIDTH,
                                (float)GetScreenHeight() / SCREEN_HEIGHT);
            int vx = (GetScreenWidth() - (int)(SCREEN_WIDTH * scale)) / 2;
            int vy = (GetScreenHeight() - (int)(SCREEN_HEIGHT * scale)) / 2;
            int vw = (int)(SCREEN_WIDTH * scale);
            int vh = (int)(SCREEN_HEIGHT * scale);

            BeginTextureMode(target);

                ClearBackground(RAYWHITE);
				BeginMode2D(camera);

					DrawRectangle(STAGE_LEFT, GROUND_HEIGHT, STAGE_RIGHT - STAGE_LEFT, SCREEN_HEIGHT, GRAY);

                	float pw0 = gameState.players[0].character->width;
                	float ph0 = gameState.players[0].character->height;
                	DrawRectangle(gameState.players[0].x - pw0 * 0.5f, gameState.players[0].y - ph0, pw0, ph0, RED);
                	float pw1 = gameState.players[1].character->width;
                	float ph1 = gameState.players[1].character->height;
			    	DrawRectangle(gameState.players[1].x - pw1 * 0.5f, gameState.players[1].y - ph1, pw1, ph1, BLUE);

				EndMode2D();

                DrawHealthBars(&gameState, showDebugOverlay);

                if (paused) {
				    DrawText("PAUSED", SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 - 10, 30, RED);
                }

                if (showDebugOverlay) {
					BeginMode2D(camera);

						for (int pi = 0; pi < MAX_PLAYERS; pi++) {
                    	    struct Player* pp = &gameState.players[pi];
                    	    struct Character* cc = pp->character;

                    	    // Hurtboxes (green)
                    	    {
                    	        struct Rect hurtboxes[MAX_HURTBOXES];
                    	        int hurtboxCount = GetActiveHurtboxes(pp, hurtboxes, MAX_HURTBOXES);
                    	        for (int h = 0; h < hurtboxCount; h++) {
                    	            struct Rect* r = &hurtboxes[h];
                    	            float hx = pp->x + (pp->facingRight ? r->offsetX : -r->offsetX - r->width);
                    	            float hy = pp->y + r->offsetY;
                    	            DrawRectangleLines((int)hx, (int)hy, (int)r->width, (int)r->height, (Color){ 0, 255, 0, 255 });
                    	            DrawRectangle((int)hx, (int)hy, (int)r->width, (int)r->height, (Color){ 0, 255, 0, 80 });
                    	        }
                    	    }

                    	    // Collision boxes (yellow)
                    	    {
                    	        struct CollisionBox boxes[MAX_COLLISION_BOXES];
                    	        int boxCount = GetActiveCollisionBoxes(pp, boxes, MAX_COLLISION_BOXES);
                    	        for (int b = 0; b < boxCount; b++) {
                    	            struct Rect* r = &boxes[b].rect;
                    	            float hx, hy;
                    	            WorldRect(pp, r, &hx, &hy);
                    	            DrawRectangleLines((int)hx, (int)hy, (int)r->width, (int)r->height, (Color){ 255, 255, 0, 255 });
                    	            DrawRectangle((int)hx, (int)hy, (int)r->width, (int)r->height, (Color){ 255, 255, 0, 60 });
                    	        }
                    	    }

                    	    // Hitboxes (red) - only during attack
                    	    if (pp->state == STATE_ATTACK) {
                    	        struct AttackData* ad = &cc->attacks[pp->attackType];
                    	        int totalFrames = ad->startup + ad->active + ad->recovery;
                    	        int elapsed = totalFrames - pp->stateTimer;
                    	        for (int s = 0; s < ad->stripCount; s++) {
                    	            struct HitboxStrip* strip = &ad->strips[s];
                    	            if (elapsed >= strip->startFrame && elapsed <= strip->endFrame) {
                    	                for (int b = 0; b < strip->hitboxCount; b++) {
                    	                    struct Rect* r = &strip->hitboxes[b].rect;
                    	                    float hx = pp->x + (pp->facingRight ? r->offsetX : -r->offsetX - r->width);
                    	                    float hy = pp->y + r->offsetY;
                    	                    DrawRectangleLines((int)hx, (int)hy, (int)r->width, (int)r->height, (Color){ 255, 0, 0, 255 });
											DrawRectangle((int)hx, (int)hy, (int)r->width, (int)r->height, (Color){ 255, 0, 0, 80 });
                    	                }
                    	            }
                    	        }
                    	    }
                    	}

					EndMode2D();

                    DrawText(TextFormat("Frame: %d", gameState.currentFrame), 10, 45, 20, BLACK);
                    DrawText(TextFormat("Sim FPS: %.0f", simFPS), 10, 65, 20, BLACK);
                    DrawText(TextFormat("Sim Frame Time: %.3f ms", simFrameTimeMs), 10, 85, 20, BLACK);
                    DrawText(TextFormat("Render Frame Time: %.3f ms", renderFrameTimeMs), 10, 105, 20, BLACK);
					// DrawText(TextFormat("Player[0]: (%.0f,%.0f)", gameState.players[0].x, gameState.players[0].y), 10, 125, 20, BLACK);
					// DrawText(TextFormat("Player[1]: (%.0f,%.0f)", gameState.players[1].x, gameState.players[1].y), 10, 145, 20, BLACK);

                    if (showStateTimeline) {
                        DrawStateTimeline();
                    }

                    int inputY = SCREEN_HEIGHT - 10;
                    if (showInputP1) {
                        int idx = (gameState.currentFrame + INPUT_BUFFER_SIZE - 1) % INPUT_BUFFER_SIZE;
                        DrawCurrentInput(gameState.inputBuffer[idx].players[0], 10, inputY - 40, true);
                        DrawInputHistory(historyP1, historyCountP1, 10, inputY - 40 - 18 * historyCountP1 - 5);
                    }
                    if (showInputP2) {
                        int idx = (gameState.currentFrame + INPUT_BUFFER_SIZE - 1) % INPUT_BUFFER_SIZE;
                        int p2x = SCREEN_WIDTH - 10 - 90;
                        DrawCurrentInput(gameState.inputBuffer[idx].players[1], p2x, inputY - 40, false);
                        DrawInputHistory(historyP2, historyCountP2, p2x, inputY - 40 - 18 * historyCountP2 - 5);
                    }

                    DrawText("[F1] Debug  [F2] Reset  [F3] Pause  [F4] Step  [F5] P1 In  [F6] P2 In  [F7] Timeline", 10, SCREEN_HEIGHT - 20, 12, BLACK);
                }

            EndTextureMode();

            DrawTexturePro(target.texture,
                           (Rectangle){ 0, 0, SCREEN_WIDTH, -SCREEN_HEIGHT },
                           (Rectangle){ vx, vy, vw, vh },
                           (Vector2){ 0, 0 }, 0.0f, WHITE);

        EndDrawing();
    }

    UnloadRenderTexture(target);
    CloseWindow();

    return 0;
}
