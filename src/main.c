#include "raylib.h"

#define GRAVITY 1800.0f
#define DT (1.0f / 60.0f)
#define GROUND_HEIGHT 400
#define ATTACK_DURATION 15
#define MAX_PLAYERS 2
#define INPUT_BUFFER_SIZE 120

struct InputState {
    bool left;
    bool right;
    bool up;
    bool down;

    bool light;
    bool medium;
    bool heavy;
};

enum AttackType { ATTACK_LIGHT, ATTACK_MEDIUM, ATTACK_HEAVY, ATTACK_COUNT };

struct AttackData {
	int startup, active, recovery;
	float moveX, moveY;
	int damage;
};

struct Character {
    const char* name;
    float walkSpeed;
    float jumpStrength;
    float width;
    float height;
	struct AttackData attacks[ATTACK_COUNT];
};

static struct Character character1 = {
    .name = "Character 1",
    .walkSpeed = 300.0f,
    .jumpStrength = 700.0f,
    .width = 50.0f,
    .height = 100.0f,
	.attacks = {
        [ATTACK_LIGHT] = (struct AttackData) {
		    .startup = 3, .active = 5, .recovery = 7, .moveX = 0.0f, .moveY = 0.0f, .damage = 100
		},
        [ATTACK_MEDIUM] = (struct AttackData) {
		    .startup = 5, .active = 5, .recovery = 8, .moveX = 0.0f, .moveY = 0.0f, .damage = 200
		},
		[ATTACK_HEAVY] = (struct AttackData) {
		    .startup = 8, .active = 10, .recovery = 10, .moveX = 300.0f, .moveY = 0.0f, .damage = 400
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
};

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
    enum PlayerState state;
    int stateTimer;
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
    save->state = player->state;
    save->stateTimer = player->stateTimer;
}

void RestorePlayerState(struct Player* player, const struct SaveState* save) {
    player->x = save->x;
    player->y = save->y;
    player->velocityX = save->velocityX;
    player->velocityY = save->velocityY;
    player->grounded = save->grounded;
    player->state = save->state;
    player->stateTimer = save->stateTimer;
}

void InitPlayer(struct Player * player, float x, float y, struct Character* character) {
	player->x = x;
    player->y = y;
    player->velocityX = 0.0f;
    player->velocityY = 0.0f;
    player->grounded = false;
    player->state = STATE_IDLE;
	player->character = character;
    player->stateTimer = 0;
}

void GameStep(struct Player* player, const struct InputState* input, const struct InputState* prevInput) {
    int dirX = 0;
    if (input->left && !input->right) {
        dirX = -1;
    } else if (input->right && !input->left) {
        dirX = 1;
    }

    bool lightPressed = input->light && !prevInput->light;
    bool mediumPressed = input->medium && !prevInput->medium;
    bool heavyPressed = input->heavy && !prevInput->heavy;
    bool attackPressed = lightPressed || mediumPressed || heavyPressed;

    switch (player->state) {
        case STATE_IDLE:
        case STATE_WALK:
            if (attackPressed) {
		        player->velocityX = 0.0f;
		        player->state = STATE_ATTACK;
		        player->stateTimer = ATTACK_DURATION;
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
            if (attackPressed) {
                player->state = STATE_ATTACK;
                player->stateTimer = ATTACK_DURATION;
                break;
            }

            if (player->grounded) {
                player->state = (dirX != 0) ? STATE_WALK : STATE_IDLE;
            }

            break;
        case STATE_ATTACK:
            player->stateTimer--;
            if (player->grounded) {
                player->velocityX = 0.0f;
            }
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
                DrawText(TextFormat("Frame: %d", gameState.currentFrame), 10, 10, 20, BLACK);
                DrawText(TextFormat("Sim FPS: %.0f", simFPS), 10, 30, 20, BLACK);
                DrawText(TextFormat("Sim Frame Time: %.3f ms", simFrameTimeMs), 10, 50, 20, BLACK);
                DrawText(TextFormat("Render Frame Time: %.3f ms", renderFrameTimeMs), 10, 70, 20, BLACK);

                DrawText("[F1] Debug  [F2] Reset  [F3] Pause  [F4] Step", 10, screenHeight - 30, 15, GRAY);
            }
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
