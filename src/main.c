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

struct Character {
    const char* name;
    float walkSpeed;
    float jumpStrength;
};

static struct Character character1 = {
    .name = "Character 1",
    .walkSpeed = 300.0f,
    .jumpStrength = 700.0f
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

struct SaveState {
    float x, y;
    float velocityX, velocityY;
    bool grounded;
    enum PlayerState state;
    int stateTimer;
};

// Rollback infrastructure
static struct InputState inputBuffer[INPUT_BUFFER_SIZE];
static int currentFrame = 0;

static struct Player player;
static float simFPS = 0.0f;
static float simFrameTimeMs = 0.0f;

void GameStep(struct Player* player, const struct InputState* input, const struct InputState* prevInput);
void SavePlayerState(const struct Player* player, struct SaveState* save);
void RestorePlayerState(struct Player* player, const struct SaveState* save);
struct InputState GatherInputs(void);
void RunOneSimStep(void);

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
            player->velocityX = dirX * player->character->walkSpeed;
            player->state = (dirX != 0) ? STATE_WALK : STATE_IDLE;

            if (input->up && player->grounded) {
                player->velocityY = -player->character->jumpStrength;
                player->grounded = false;
                player->state = STATE_JUMP;
            }

            if (attackPressed) {
                player->state = STATE_ATTACK;
                player->stateTimer = ATTACK_DURATION;
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

void RunOneSimStep(void) {
    struct InputState input = GatherInputs();

    int index = currentFrame % INPUT_BUFFER_SIZE;
    inputBuffer[index] = input;

    struct InputState emptyInput = {0};
    struct InputState* prevInput = &emptyInput;
    if (currentFrame > 0) {
        int prevIndex = (currentFrame - 1) % INPUT_BUFFER_SIZE;
        prevInput = &inputBuffer[prevIndex];
    }

    double stepStart = GetTime();
    GameStep(&player, &input, prevInput);
    currentFrame++;
    double stepEnd = GetTime();

    simFrameTimeMs = (float)((stepEnd - stepStart) * 1000.0);
}

int main(void) {
    const int screenWidth = 800;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "Cole's Fighting Game");

    player.x = 400.0f;
    player.y = 400.0f;
    player.state = STATE_IDLE;
    player.character = &character1;

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
            player.x = 400.0f;
            player.y = 400.0f;
            player.velocityX = 0.0f;
            player.velocityY = 0.0f;
            player.grounded = false;
            player.state = STATE_IDLE;
            player.stateTimer = 0;
        }
        if (IsKeyPressed(KEY_F3)) paused = !paused;
        if (IsKeyPressed(KEY_F4)) doStep = 1;

        if (paused) {
            if (doStep) {
                RunOneSimStep();
                doStep = 0;
            }
        } else {
            accumulator += frameTime;

            int stepsRun = 0;
            while (accumulator >= DT) {
                RunOneSimStep();
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
            DrawRectangle(player.x, player.y, 50, 100, RED);

            if (paused) {
				DrawText("PAUSED", screenWidth / 2 - 50, screenHeight / 2 - 10, 30, RED);
            }

            if (showDebugOverlay) {
                DrawText(TextFormat("Frame: %d", currentFrame), 10, 10, 20, BLACK);
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
