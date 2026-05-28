#include "raylib.h"
#include <stdio.h>

#define GRAVITY 1800.0f
#define DT (1.0f / 60.0f)
#define GROUND_HEIGHT 400

#define MAX_PLAYERS 2
#define INPUT_BUFFER_SIZE 120

#define MAX_HURTBOXES 4
#define MAX_HITBOXES 4
#define MAX_HITBOX_STRIPS 8
#define MAX_HITBOXES_PER_STRIP 4

struct InputState {
    bool left;
    bool right;
    bool up;
    bool down;

    bool light;
    bool medium;
    bool heavy;
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

struct HitboxStrip {
    int startFrame;
    int endFrame;
    int hitboxCount;
    struct Hitbox hitboxes[MAX_HITBOXES_PER_STRIP];
    int damage;
    float knockbackX, knockbackY;
    int hitstun;
};

enum AttackType { ATTACK_LIGHT, ATTACK_MEDIUM, ATTACK_HEAVY, ATTACK_COUNT };

struct AttackData {
	int startup, active, recovery;
	struct HitboxStrip strips[MAX_HITBOX_STRIPS];
    int stripCount;
	float moveX, moveY;
};

struct Character {
    const char* name;
    float walkSpeed;
    float jumpStrength;
    float width;
    float height;
	struct Rect hurtboxes[MAX_HURTBOXES];
    int hurtboxCount;
    struct AttackData attacks[ATTACK_COUNT];
};

static struct Character character1 = {
    .name = "Character 1",
    .walkSpeed = 300.0f,
    .jumpStrength = 700.0f,
    .width = 50.0f,
    .height = 100.0f,
    .hurtboxes = {
        [0] = {
            .offsetX = -25.0f, .offsetY = -100.0f, .width = 50.0f, .height = 100.0f,
        },
    },
    .hurtboxCount = 1,
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
                        [0] = { .rect = { .offsetX = 25.0f, .offsetY = -60.0f, .width = 30.0f, .height = 20.0f } },
                    },
                    .damage = 100,
                    .knockbackX = 100.0f, .knockbackY = 0.0f,
                    .hitstun = 10,
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
                        [0] = { .rect = { .offsetX = 45.0f, .offsetY = -40.0f, .width = 35.0f, .height = 25.0f } },
                    },
                    .damage = 200,
                    .knockbackX = 150.0f, .knockbackY = 0.0f,
                    .hitstun = 15,
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
                        [0] = { .rect = { .offsetX = 50.0f, .offsetY = -30.0f, .width = 40.0f, .height = 30.0f } },
                    },
                    .damage = 400,
                    .knockbackX = 300.0f, .knockbackY = -100.0f,
                    .hitstun = 25,
                },
            },
        },
    },
};

enum PlayerState {
    STATE_IDLE,
    STATE_WALK,
    STATE_JUMP,
    STATE_ATTACK
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

	bool hitConnected[MAX_HITBOX_STRIPS];
};

static void WorldRect(struct Player* p, struct Rect* r, float* outX, float* outY) {
    *outX = p->x + (p->facingRight ? r->offsetX : -r->offsetX - r->width);
    *outY = p->y + r->offsetY;
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
    bool hitConnected[MAX_HITBOX_STRIPS];
};

static struct GameState gameState;
static float simFPS = 0.0f;
static float simFrameTimeMs = 0.0f;

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
    }
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

void RunOneSimStep(struct GameState* gs) {
	struct GameInput frameInput;

    frameInput.players[0] = GatherInputs();
    frameInput.players[1] = GatherInputs2();
	
    int index = gs->currentFrame % INPUT_BUFFER_SIZE;
    gs->inputBuffer[index] = frameInput;

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
	
	for (int i = 0; i < MAX_PLAYERS; i++) {
		int opponent = (i == 0) ? 1 : 0;
		struct Player* attacker = &gs->players[i];
		struct Player* defender = &gs->players[opponent];

		if (attacker->state != STATE_ATTACK) continue;

		struct AttackData* ad = &attacker->character->attacks[attacker->attackType];
	    int total = ad->startup + ad->active + ad->recovery;
	    int elapsed = total - attacker->stateTimer;

	    for (int s = 0; s < ad->stripCount; s++) {
	        if (attacker->hitConnected[s]) continue;
	        struct HitboxStrip* strip = &ad->strips[s];
	        if (elapsed < strip->startFrame || elapsed > strip->endFrame) continue;

	        for (int h = 0; h < strip->hitboxCount; h++) {
	            float ax, ay;
	            WorldRect(attacker, &strip->hitboxes[h].rect, &ax, &ay);
	            float aw = strip->hitboxes[h].rect.width;
	            float ah = strip->hitboxes[h].rect.height;

	            for (int hb = 0; hb < defender->character->hurtboxCount; hb++) {
	                float dx, dy;
	                WorldRect(defender, &defender->character->hurtboxes[hb], &dx, &dy);
	                float dw = defender->character->hurtboxes[hb].width;
	                float dh = defender->character->hurtboxes[hb].height;

	                if (AABB(ax, ay, aw, ah, dx, dy, dw, dh)) {
	                    attacker->hitConnected[s] = true;

						printf("TODO: apply damage, knockback, hitstun here\n");
						fflush(stdout);

	                    goto next_strip;	
					}
	            }
	        }
	        next_strip:;
	    }
	}
    gs->currentFrame++;
    double stepEnd = GetTime();
    simFrameTimeMs = (float)((stepEnd - stepStart) * 1000.0);
}

int main(void) {
    const int screenWidth = 800;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "Cole's Fighting Game");
	
	InitPlayer(&gameState.players[0], 200.0f, GROUND_HEIGHT, &character1);
    InitPlayer(&gameState.players[1], 600.0f, GROUND_HEIGHT, &character1);

    double accumulator = 0.0;
    double fpsTimer = 0.0;
    int stepsThisSecond = 0;

    int paused = 0;
    int doStep = 0;
    int showDebugOverlay = 1;

    while (!WindowShouldClose()) {
        double frameTime = GetFrameTime();
        if (frameTime > 0.1) frameTime = 0.1;
        float renderFrameTimeMs = (float)(frameTime * 1000.0);

        // Dev input handling
        if (IsKeyPressed(KEY_F1)) showDebugOverlay = !showDebugOverlay;
        if (IsKeyPressed(KEY_F2)) {
			InitPlayer(&gameState.players[0], 200.0f, GROUND_HEIGHT, &character1);
			InitPlayer(&gameState.players[1], 600.0f, GROUND_HEIGHT, &character1);
        }
        if (IsKeyPressed(KEY_F3)) paused = !paused;
        if (IsKeyPressed(KEY_F4)) doStep = 1;

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

        BeginDrawing();
            ClearBackground(RAYWHITE);

			DrawRectangle(0, GROUND_HEIGHT, screenWidth, screenHeight, GRAY);

            float pw0 = gameState.players[0].character->width;
            float ph0 = gameState.players[0].character->height;
            DrawRectangle(gameState.players[0].x - pw0 * 0.5f, gameState.players[0].y - ph0, pw0, ph0, RED);
            float pw1 = gameState.players[1].character->width;
            float ph1 = gameState.players[1].character->height;
			DrawRectangle(gameState.players[1].x - pw1 * 0.5f, gameState.players[1].y - ph1, pw1, ph1, BLUE);

            if (paused) {
				DrawText("PAUSED", screenWidth / 2 - 50, screenHeight / 2 - 10, 30, RED);
            }

            if (showDebugOverlay) {
                for (int pi = 0; pi < MAX_PLAYERS; pi++) {
                    struct Player* pp = &gameState.players[pi];
                    struct Character* cc = pp->character;

                    for (int h = 0; h < cc->hurtboxCount; h++) {
                        struct Rect* r = &cc->hurtboxes[h];
                        float hx = pp->x + (pp->facingRight ? r->offsetX : -r->offsetX - r->width);
                        float hy = pp->y + r->offsetY;
                        DrawRectangle((int)hx, (int)hy, (int)r->width, (int)r->height, (Color){ 0, 0, 255, 80 });
                    }

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
                                    DrawRectangle((int)hx, (int)hy, (int)r->width, (int)r->height, (Color){ 255, 0, 0, 80 });
                                }
                            }
                        }
                    }
                }

                DrawText(TextFormat("Frame: %d", gameState.currentFrame), 10, 10, 20, BLACK);
                DrawText(TextFormat("Sim FPS: %.0f", simFPS), 10, 30, 20, BLACK);
                DrawText(TextFormat("Sim Frame Time: %.3f ms", simFrameTimeMs), 10, 50, 20, BLACK);
                DrawText(TextFormat("Render Frame Time: %.3f ms", renderFrameTimeMs), 10, 70, 20, BLACK);

                DrawText("[F1] Debug  [F2] Reset  [F3] Pause  [F4] Step", 10, screenHeight - 30, 15, BLACK);
            }
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
