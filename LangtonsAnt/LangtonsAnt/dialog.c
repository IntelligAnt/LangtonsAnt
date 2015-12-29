#include "graphics.h"

WINDOW *dialogw;
Vector2i dialog_pos;

int cidx;
short picked_color = COLOR_NONE, picked_turn = TURN_NONE;

const Vector2i colors_pos  = { 1, 1 };
const Vector2i left_pos    = { DIALOG_TILE_ROWS*DIALOG_TILE_SIZE+2, 1 };
const Vector2i right_pos   = { DIALOG_TILE_ROWS*DIALOG_TILE_SIZE+2, DIALOG_BUTTON_WIDTH+2 };
const Vector2i delete_pos  = { DIALOG_TILE_ROWS*DIALOG_TILE_SIZE+DIALOG_BUTTON_HEIGHT+3,
                               (DIALOG_WINDOW_WIDTH-DIALOG_DELETE_WIDTH-2)/2+1 };

void open_dialog(Vector2i pos, int color_index)
{
	size_t height = (color_index == CIDX_DEFAULT)  ? left_pos.x
		          : (color_index == CIDX_NEWCOLOR) ? delete_pos.y
		          : DIALOG_WINDOW_HEIGHT;
	cidx = color_index;

	if (pos.x + DIALOG_WINDOW_WIDTH >= MENU_WINDOW_WIDTH) {
		pos.x += DIALOG_WINDOW_WIDTH - MENU_WINDOW_WIDTH - 2;
	}
	dialog_pos = rel2abs(pos, menu_pos);

	dialogw = newwin(height, DIALOG_WINDOW_WIDTH, dialog_pos.y, dialog_pos.x);
	keypad(dialogw, TRUE);
	nodelay(dialogw, TRUE);
}

void close_dialog(void)
{
	delwin(dialogw);
	dialogw = NULL;
	picked_color = COLOR_NONE;
	picked_turn = TURN_NONE;
}

static void draw_tiles(void)
{
	short i, fg = GET_COLOR_FOR(fg_pair);
	Vector2i outer = colors_pos, inner;

	for (i = 0; i < COLOR_COUNT; i++) {
		if (i == fg || cidx != CIDX_DEFAULT && i == stgs.colors->def) {
			continue;
		}
		if (color_exists(stgs.colors, i)) {
			inner.y = outer.y + 1, inner.x = outer.x + 1;
			wattrset(dialogw, GET_PAIR_FOR(stgs.colors->def));
			draw_square(dialogw, outer, DIALOG_TILE_SIZE);
			wattrset(dialogw, GET_PAIR_FOR(i));
			draw_square(dialogw, inner, DIALOG_TILE_SIZE - 2);
		} else {
			wattrset(dialogw, GET_PAIR_FOR(i));
			draw_square(dialogw, outer, DIALOG_TILE_SIZE);
			if (picked_color == i) {
				wattron(dialogw, A_REVERSE);
				draw_box_border(dialogw, outer, DIALOG_TILE_SIZE, DIALOG_TILE_SIZE);
			}
		}
		if (outer.x+DIALOG_TILE_SIZE+1 >= DIALOG_WINDOW_WIDTH) {
			outer.y += DIALOG_TILE_SIZE, outer.x = colors_pos.x;
		} else {
			outer.x += DIALOG_TILE_SIZE;
		}
	}
}

static void draw_buttons()
{
	if (cidx == CIDX_DEFAULT) {
		return;
	}

	int ymid = DIALOG_BUTTON_HEIGHT/2, xmid = DIALOG_BUTTON_WIDTH/2;
	wattrset(dialogw, GET_PAIR_FOR((picked_color != COLOR_NONE) ? picked_color : DIALOG_BUTTON_COLOR));
	draw_rect(dialogw, left_pos,  DIALOG_BUTTON_WIDTH, DIALOG_BUTTON_HEIGHT);
	draw_rect(dialogw, right_pos, DIALOG_BUTTON_WIDTH, DIALOG_BUTTON_HEIGHT);
	wattron(dialogw, A_REVERSE);
	mvwaddstr(dialogw, left_pos.y+ymid,  left_pos.x+xmid,  "<");
	mvwaddstr(dialogw, right_pos.y+ymid, right_pos.x+xmid, ">");
	if (picked_turn != TURN_NONE) {
		draw_box_border(dialogw, (picked_turn == TURN_LEFT) ? left_pos : right_pos,
						DIALOG_BUTTON_WIDTH, DIALOG_BUTTON_HEIGHT);
	}

	if (cidx >= 0) {
		ymid = DIALOG_DELETE_HEIGHT/2, xmid = DIALOG_DELETE_WIDTH/2;
		wattrset(dialogw, GET_PAIR_FOR(DIALOG_DELETE_COLOR));
		draw_rect(dialogw, delete_pos, DIALOG_DELETE_WIDTH, DIALOG_DELETE_HEIGHT);
		wattron(dialogw, A_REVERSE);
		mvwaddstr(dialogw, delete_pos.y+ymid, delete_pos.x+xmid, "X");
	}
}

void draw_dialog(void)
{
	wattrset(dialogw, GET_PAIR_FOR(stgs.colors->def));
	draw_rect(dialogw, (Vector2i) { 0, 0 }, DIALOG_WINDOW_WIDTH, DIALOG_WINDOW_HEIGHT);

	draw_tiles();
	draw_buttons();

	wnoutrefresh(dialogw);
}

Vector2i get_dialog_tile_pos(int index)
{
	int i, fg = GET_COLOR_FOR(fg_pair);
	Vector2i pos = { 1, 1 };

	if (index == fg || cidx != CIDX_DEFAULT && index == stgs.colors->def) {
		return (Vector2i) { -1, -1 };
	}

	for (i = 0; i < index; ++i) {
		if (i == fg || cidx != CIDX_DEFAULT && i == stgs.colors->def) {
			continue;
		}
		if (pos.x + DIALOG_TILE_SIZE + 1 < DIALOG_WINDOW_WIDTH) {
			pos.x += DIALOG_TILE_SIZE;
		} else {
			pos.x = 1;
			pos.y += DIALOG_TILE_SIZE;
		}
	}

	return pos;
}

input_t dialog_mouse_command(MEVENT event)
{
	int i;
	bool del = FALSE;
	Vector2i pos = abs2rel((Vector2i) { event.y, event.x }, dialog_pos), tl;

	if (!(event.bstate & BUTTON1_CLICKED)) {
		return INPUT_NO_CHANGE;
	}

	if (area_contains(left_pos, DIALOG_BUTTON_WIDTH, DIALOG_BUTTON_HEIGHT, pos)) {
		picked_turn = TURN_LEFT;
		goto exit;
	}
	if (area_contains(right_pos, DIALOG_BUTTON_WIDTH, DIALOG_BUTTON_HEIGHT, pos)) {
		picked_turn = TURN_RIGHT;
		goto exit;
	}
	if (cidx >= 0 && area_contains(delete_pos, DIALOG_DELETE_WIDTH, DIALOG_DELETE_HEIGHT, pos)) {
		del = TRUE;
		goto exit;
	}
	for (i = 0; i < COLOR_COUNT; i++) {
		tl = get_dialog_tile_pos(i);
		if (!color_exists(stgs.colors, i) &&
			area_contains(tl, DIALOG_TILE_SIZE, DIALOG_TILE_SIZE, pos)) {
			picked_color = i;
			goto exit;
		}
	}

	return INPUT_NO_CHANGE;

exit:
	switch (cidx) {
	case CIDX_NEWCOLOR:
		if (picked_color != COLOR_NONE && picked_turn != TURN_NONE) {
			add_color(stgs.colors, picked_color, picked_turn);
			close_dialog();
		}
		return INPUT_MENU_CHANGED;
	case CIDX_DEFAULT:
		colors_delete(stgs.colors);
		stgs.colors = colors_new(picked_color);
		close_dialog();
		return INPUT_MENU_CHANGED;
	default:
		if (cidx < 0 || cidx >= COLOR_COUNT) {
			return INPUT_NO_CHANGE;
		}
		if (picked_turn != TURN_NONE) {
			if (picked_color != COLOR_NONE) {
				set_color(stgs.colors, cidx, picked_color, picked_turn);
			} else {
				set_turn(stgs.colors, cidx, picked_turn);
			}
			close_dialog();
			return INPUT_MENU_CHANGED;
		} else if (del) {
			remove_color(stgs.colors, get_color_at(stgs.colors, cidx));
			if (!has_enough_colors(stgs.colors)) {
				halt_simulation(stgs.linked_sim);
			}
			close_dialog();
		}
		return INPUT_MENU_CHANGED;
	}
}
