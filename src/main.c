#include <SDL3/SDL.h>
#include <SDL3/SDL_init.h>

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define SDL_FLAGS SDL_INIT_VIDEO
#define SDL_WINDOWFLAGS (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_VULKAN)

#define WINDOW_TITLE "Cole's Game"
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

struct Game {
	SDL_Window *window;
	SDL_Renderer *renderer;
};

bool game_init_sdl(struct Game *g);
void game_free(struct Game *g);
void game_run(struct Game *g);

int main(void) {
	bool exit_status = EXIT_FAILURE;

	struct Game game = {0};
	
	if (game_init_sdl(&game)) {
		game_run(&game);

		exit_status = EXIT_SUCCESS;
	}
	
	game_free(&game);

	return exit_status;
}

bool game_init_sdl(struct Game *g) {
	if (!SDL_Init(SDL_FLAGS)) {
		fprintf(stderr, "Error Initializing SDL3: %s\n", SDL_GetError());
		return false;
	}

	g->window = SDL_CreateWindow(WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOWFLAGS);
	if (!g->window) {
		fprintf(stderr, "Error Creating Window: %s\n", SDL_GetError());
		return false;
	}

	g->renderer = SDL_CreateRenderer(g->window, NULL);
	if (!g->renderer) {
		fprintf(stderr, "Error Creating Renderer: %s\n", SDL_GetError());
		return false;
	}
	
	return true;
}

void game_free(struct Game *g) {
	if (g->renderer) {
		SDL_DestroyRenderer(g->renderer);
		g->renderer = NULL;
	}

	if (g->window) {
		SDL_DestroyWindow(g->window);
		g->window = NULL;
	}

	SDL_Quit();
}

void game_run(struct Game *g) {
	SDL_RenderClear(g->renderer);

	SDL_RenderPresent(g->renderer);

	SDL_Delay(5000);
}
