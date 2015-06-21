#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
//#include <vld.h>
#include "logic.h"
#include "graphics.h"

#define DRAW_EVERY 1
#define INPUT_DELAY 10000
#define SCROLL_INC 10

int main(void)
{
	int init_size;
	printf("Pocetna velicina (2, 3, 4, 5, 6): ");
	scanf("%d", &init_size);
	assert(init_size == 2 || init_size == 3 || init_size == 4 || init_size == 5 || init_size == 6);
	system("cls");

	Colors *colors = colors_new(COLOR_WHITE);
	Grid *grid = grid_new(init_size, colors);
	Ant *ant = ant_new(grid, UP);
	int i = 1;
	short c, turn;
	//grid_make_sparse(grid);

	while (i) {
		printf("1. Nova boja.\n2. Izbrisi boju.\n0 za crtanje\n");
		scanf("%d", &i);
		switch (i) {
		case 0:
			if (!has_enough_colors(colors)) {
				i = 1;
			}
			break;
		case 1:
			printf("Unesite boju (0-14) i smer skretanja (-1 ili 1)\n");
			scanf("%hi %hi", &c, &turn);
			assert(abs(turn) == 1);
			if (!color_exists(colors, c)) {
				add_color(colors, c, turn);
			}
			break;
		case 2:
			printf("Unesite boju (0-14)\n");
			scanf("%hi", &c);
			if (color_exists(colors, c)) {
				remove_color(colors, c);
			}
			break;
		}
		system("cls");
	}
	//freopen("memleaks.dmp", "w", stdout);
	
	Vector2i oldp;
	int steps = 0, cnt = DRAW_EVERY-1, input_delay = 0, sc_inc;

	init_graphics(COLOR_BLACK, COLOR_WHITE); // TODO fix flicker with bg colors other than white
	draw_grid_full(grid);

	i = 0;
	while (1) {
		oldp = ant->pos;
		ant_move(ant, grid, colors);
		grid_silent_expand(grid);
		if (is_ant_out_of_bounds(ant, grid)) {
			grid_expand(grid, ant);
			mvprintw(10, GRID_WINDOW_SIZE+10, "%d", ++i);
			mvprintw(11, GRID_WINDOW_SIZE+10, "%d", grid->size);
			if (is_grid_sparse(grid)) {
				mvaddstr(12, GRID_WINDOW_SIZE+10, "SPARSE");
			}
			draw_grid_full(grid);
		} else if (++cnt == DRAW_EVERY) {
			draw_grid_iter(grid, oldp, ant->pos);
			cnt = 0;
		}
		if (input_delay == 0) {
			switch (getch()) { // TODO apparently getch() refreshes the screen - optimize
			case KEY_UP:
				scroll_grid(grid, -SCROLL_INC, 0);
				goto delay;
			case KEY_DOWN:
				scroll_grid(grid, SCROLL_INC, 0);
				goto delay;
			case KEY_LEFT:
				scroll_grid(grid, 0, -SCROLL_INC);
				goto delay;
			case KEY_RIGHT:
				scroll_grid(grid, 0, SCROLL_INC);
			delay:
				input_delay += INPUT_DELAY;
				draw_grid_full(grid);
			}
		} else {
			--input_delay;
		}
		++steps;
	}

exit:
	colors_delete(colors);
	grid_delete(grid);
	ant_delete(ant);
	end_graphics();

	return 0;
}
