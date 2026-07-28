#define KONWINLIB_IMPLEMENTATION
#include "konwinlib.h"

#define KONTOML_IMPLEMENTATION
#include "kontoml.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

typedef struct random_cfg {
	bool random_cat;

	/* window */
	char title[64];

	int x, y;
	int width, height;

	/* behaviour */
	double walk_speed;
	double chance_of_rolling_a_6;

	/* audio */
	double volume;
} random_cfg_t;

static bool loadRandomCfg(const char *filePath, random_cfg_t *cfg) {
	kon_toml_t *toml = kon_parseTomlFile(filePath);
	if (!toml) {
		fprintf(stderr, "kon_parseTomlFile: error parsing file %s\n", filePath);
		return false;
	}

	cfg->random_cat  = kon_tomlGetBool(toml, "", "random_cat", false);

	/* window */
	strcpy(cfg->title, kon_tomlGetString(toml, "window", "title", ""));

	cfg->x           = kon_tomlGetInt(toml, "window", "x", 0);
	cfg->y           = kon_tomlGetInt(toml, "window", "y", 0);
	cfg->width       = kon_tomlGetInt(toml, "window", "width",  640);
	cfg->height      = kon_tomlGetInt(toml, "window", "height", 640);

	/* behaviour */
	cfg->walk_speed  = kon_tomlGetInt(toml, "behaviour", "walk_speed", 1);

	cfg->chance_of_rolling_a_6 = kon_tomlGetInt(toml, "behaviour", "chance_of_rolling_a_6", 1);

	/* audio */
	cfg->volume      = kon_tomlGetFloat(toml, "audio", "volume", 1.0);

	kon_freeToml(toml);

	return true;
}

int main(void) {

	/*** toml file loading ***/

	random_cfg_t cfg;

	const char *filePath = "example.toml";

	if (!loadRandomCfg(filePath, &cfg)) return EXIT_FAILURE;
	puts("toml file has been loaded!");
	putc('\n', stdout);

	if (cfg.random_cat) puts("OH NO!!! THERES SOME RANDOM CAT!!!");

	printf("character walk speed: %lf\n", cfg.walk_speed);
	printf("chance of rolling a 6 on a 6 sided dice: %lf\n", cfg.chance_of_rolling_a_6);
	printf("volume: %lf\n", cfg.volume);

	/*** window initialization ***/

	if (!kon_init()) return EXIT_FAILURE;

	kon_window_t *window = kon_createWindow(cfg.title, cfg.x, cfg.y, cfg.width, cfg.height, KON_WINDOW_RESIZABLE);
	if (!window) {
		kon_deinit();
		fprintf(stderr, "kon_createWindow: error creating window\n");
		return EXIT_FAILURE;
	}

	kon_setExitKey(window, 'q');

	kon_event_t event;
	while (!kon_windowShouldClose(window)) {
		kon_pollEvent(window, &event);
	}

	/*** cleanup ***/

	kon_destroyWindow(window);

	kon_deinit();

	return EXIT_SUCCESS;
}
