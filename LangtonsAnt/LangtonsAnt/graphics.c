#include <math.h>
#include "graphics.h"

chtype fg_pair, bg_pair;
static bool do_draw = TRUE;

void init_graphics(short fg_color, short bg_color)
{
	initscr();
	resize_term(GRID_WINDOW_SIZE, GRID_WINDOW_SIZE+MENU_WINDOW_WIDTH);
	curs_set(0);
	
	cbreak();
	noecho();
	mousemask(BUTTON1_CLICKED | BUTTON3_CLICKED, NULL); // Left and right click
	keypad(stdscr, TRUE);
	nodelay(stdscr, TRUE);

	start_color();
	init_def_pairs(fg_color, bg_color);
	
	init_grid_window();
	init_menu_window();

	if (gridw == NULL || menuw == NULL) {
		printw("Couldn't initialize graphics. Please lower your terminal's font size "
			   "(raster 8x8 preferred) and try again.");
		wnoutrefresh(stdscr);
	}

	doupdate();
}

void end_graphics(void)
{
	end_grid_window();
	endwin();
}

void init_def_pairs(short fg_color, short bg_color)
{
	short i;
	for (i = 0; i < COLOR_COUNT; ++i) {
		if (i == bg_color) {
			init_pair(i+1, i, fg_color);
			bg_pair = GET_PAIR_FOR(i);
		} else {
			init_pair(i+1, i, bg_color);
			if (i == fg_color) {
				fg_pair = GET_PAIR_FOR(i);
			}
		}
	}
}

static input_t handle_input(void)
{
	input_t res = INPUT_NO_CHANGE;
	Simulation *sim = stgs.linked_sim;
	int ch = getch();
	if (sim) {
		res |= grid_key_command(sim->grid, sim->ant, ch); // TODO wgetch refreshes
	}
	res |= menu_key_command(ch);
	return res;
}

void draw_loop(void)
{
	Simulation *sim = stgs.linked_sim;
	draw_grid_full(sim->grid, sim->ant);
	draw_menu_full();

	while (do_draw) {
		input_t input = handle_input();
		input_t grid_changed = input & INPUT_GRID_CHANGED, menu_changed = input & INPUT_MENU_CHANGED;
		sim = stgs.linked_sim;

		if (is_simulation_running(sim)) {
			Vector2i oldp = sim->ant->pos;
			bool in_bounds = ant_move(sim->ant, sim->grid, sim->colors);
			grid_silent_expand(sim->grid);

			if (!in_bounds) {
				grid_expand(sim->grid, sim->ant);
				if (!grid_changed) {
					draw_grid_full(sim->grid, sim->ant);
				}
				if (!menu_changed) {
					draw_menu_full();
				}
			} else {
#if OPT_DELAY_LOOP
				unsigned d;
				for (d = 0; !IS_GRID_LARGE(sim->grid) && d < OPT_DELAY/pow(sim->steps+1, 0.9); ++d);
#endif
				if (!grid_changed) {
					draw_grid_iter(sim->grid, sim->ant, oldp);
				}
				if (!menu_changed) {
					draw_menu_iter();
				}
			}

			++(sim->steps);
		}
	
		if (grid_changed) { // TODO move before the update logic
			draw_grid_full(sim->grid, sim->ant);
		}
		if (menu_changed) {
			draw_menu_full();
		}

		doupdate();
	}
}

void stop_draw_loop(bool stop)
{
	do_draw = !stop;
}

void draw_square(WINDOW *w, Vector2i top_left, size_t size)
{
	size_t i;
	if (size == 1) {
		mvwaddch(w, top_left.y, top_left.x, ACS_BLOCK);
		return;
	}
	for (i = 0; i < size; ++i) {
		mvwhline(w, top_left.y+i, top_left.x, ACS_BLOCK, size);
	}
}

void draw_rect(WINDOW *w, Vector2i top_left, size_t width, size_t height)
{
	size_t i;
	if (width == 1 && height == 1) {
		mvwaddch(w, top_left.y, top_left.x, ACS_BLOCK);
		return;
	}
	for (i = 0; i < height; ++i) {
		mvwhline(w, top_left.y+i, top_left.x, ACS_BLOCK, width);
	}
}

void draw_border(WINDOW *w, Vector2i top_left, size_t width, size_t height)
{
	size_t y = height-1, x = width-1, i;
	if (width == 0 || height == 0) {
		return;
	}
	if (width > 1 && height > 1) {
		mvwaddch(w, top_left.y,   top_left.x,   ACS_ULCORNER);
		mvwaddch(w, top_left.y,   top_left.x+x, ACS_URCORNER);
		mvwaddch(w, top_left.y+y, top_left.x,   ACS_LLCORNER);
		mvwaddch(w, top_left.y+y, top_left.x+x, ACS_LRCORNER);
	} else {
		mvwaddch(w, top_left.y,   top_left.x+x, ACS_PLUS);
		mvwaddch(w, top_left.y+y, top_left.x,   ACS_PLUS);
	}
	for (i = 1; i < width-1; ++i) {
		mvwaddch(w, top_left.y,   top_left.x+i, ACS_HLINE);
		mvwaddch(w, top_left.y+y, top_left.x+i, ACS_HLINE);
	}
	for (i = 1; i < height-1; ++i) {
		mvwaddch(w, top_left.y+i, top_left.x,   ACS_VLINE);
		mvwaddch(w, top_left.y+i, top_left.x+x, ACS_VLINE);
	}
}

void draw_bitmap(WINDOW *w, const unsigned char *bitmap,
				 Vector2i top_left, size_t width, size_t height,
				 bool overwrite)
{
	size_t read, y, x;
	unsigned char bit;
	for (read = 0; read < width*height; ++read) {
		bit = bitmap[read/8] >> (7-read%8) & 0x1;
		y = read / width, x = read % width;
		if (overwrite) {
			mvwaddch(w, top_left.y+y, top_left.x+x, bit ? ACS_BLOCK : ' ');
		} else if (bit) {
			mvwaddch(w, top_left.y+y, top_left.x+x, ACS_BLOCK);
		}
	}
}

Vector2i rel2abs(Vector2i rel, Vector2i origin)
{
	return (Vector2i) {
		.y = origin.y + rel.y,
		.x = origin.x + rel.x
	};
}

Vector2i abs2rel(Vector2i abs, Vector2i origin)
{
	return (Vector2i) {
		.y = abs.y - origin.y,
		.x = abs.x - origin.x
	};
}

bool area_contains(Vector2i top_left, size_t width, size_t height, Vector2i v)
{
	return (v.y >= top_left.y && v.y < top_left.y+(int)height
		 && v.x >= top_left.x && v.x < top_left.x+(int)width);
}
