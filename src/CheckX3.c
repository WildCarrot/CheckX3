/*
  App to check things off a list.
  (Name is a line from Poker Night 2 at the Inventory:  "Check, check, check, on my sandwich!")

  Eventually, I'd like to hook this up to Remember the Milk on the phone
  so I can check off my grocery list while I shop.

  Right now, just working with a hard-coded list of things.
 */

#include <pebble.h>
//#include "ScrollList.h"

static Window* window;
static ScrollLayer* scroll_layer;

#define LIST_ITEM_LEN 40
#define LIST_ITEM_HEIGHT 20 // XXX FIXME for font!

struct ListItem {
	TextLayer* text_layer;
	char text_buffer[LIST_ITEM_LEN];
	bool highlighted;
};

static struct ListItem* list_items;
static int num_items = 15;
static int highlighted_item = 0;

static void highlight_item(struct ListItem* item, bool highlight)
{
	static const int BACKGROUND_COLOR = 0;
	static const int TEXT_COLOR = 1;
	static GColor nor_colors[] = {GColorWhite, GColorBlack};
	static GColor hi_colors[] = {GColorBlack, GColorWhite};
	static GColor* colors[] = { nor_colors, hi_colors };

	text_layer_set_background_color(item->text_layer, colors[highlight][BACKGROUND_COLOR]);
	text_layer_set_text_color(item->text_layer, colors[highlight][TEXT_COLOR]);
	item->highlighted = highlight;
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
	for (int i=0; i < num_items; ++i) {
		highlight_item(&(list_items[i]), !list_items[i].highlighted);
	}
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
	if (highlighted_item > 0) {
		highlight_item(&(list_items[highlighted_item]), false);
		--highlighted_item;
		highlight_item(&(list_items[highlighted_item]), true);
	}
	// If the newly highlighted item is clipped, scroll up.
	// The offset will be negative when scrolling up.
	GRect item_frame = layer_get_frame((const Layer*) list_items[highlighted_item].text_layer);
	if (item_frame.origin.y + scroll_layer_get_content_offset(scroll_layer).y < LIST_ITEM_HEIGHT) {
		scroll_layer_scroll_up_click_handler(recognizer, scroll_layer);
	}
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
	if (highlighted_item < num_items - 1) {
		highlight_item(&(list_items[highlighted_item]), false);
		++highlighted_item;
		highlight_item(&(list_items[highlighted_item]), true);
	}
	// If the newly highlighted item is clipped, scroll down.
	GRect item_frame = layer_get_frame((const Layer*) list_items[highlighted_item].text_layer);
	if (item_frame.origin.y + item_frame.size.h > 130) { // XXX near SCREEN_HEIGHT
		scroll_layer_scroll_down_click_handler(recognizer, scroll_layer);
	}
}

static void click_config_provider(void *context) {
	window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
	window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
	window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);

	scroll_layer = scroll_layer_create(bounds);

	int x = 0;
	int y = 0;

	list_items = (struct ListItem*) malloc(num_items * sizeof(struct ListItem));

	for (int i=0; i < num_items; ++i) {
		list_items[i].text_layer = text_layer_create((GRect) { .origin = { x, y },
					.size = { bounds.size.w, 20 } });
		list_items[i].highlighted = false;

		// XXX Fix for text size.
		y += 20;

		snprintf(list_items[i].text_buffer, LIST_ITEM_LEN, "Item %d\n", i);
		text_layer_set_text(list_items[i].text_layer, list_items[i].text_buffer);
		text_layer_set_text_alignment(list_items[i].text_layer, GTextAlignmentLeft);
		scroll_layer_add_child(scroll_layer, text_layer_get_layer(list_items[i].text_layer));
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Added a text layer with text %s", list_items[i].text_buffer);
	}
	scroll_layer_set_content_size(scroll_layer, (GSize) { bounds.size.w, y });

	layer_add_child(window_layer, (Layer*) scroll_layer);

//	scroll_layer_set_click_config_onto_window(scroll_layer, window);
}

static void window_appear(Window* window) {
	if (highlighted_item < num_items) {
		highlight_item(&(list_items[highlighted_item]), true);
	}
}

static void window_unload(Window *window) {
	for (int i=0; i < num_items; ++i) {
		text_layer_destroy(list_items[i].text_layer);
	}
	scroll_layer_destroy(scroll_layer);
}

static void init(void) {
	window = window_create();
	window_set_click_config_provider(window, click_config_provider);
	window_set_window_handlers(window, (WindowHandlers) {
			.load = window_load,
			.appear = window_appear,
			.unload = window_unload,
		     });
	const bool animated = true;
	window_stack_push(window, animated);
}

static void deinit(void) {
	window_destroy(window);
}

int main(void) {
	init();

	APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

	app_event_loop();
	deinit();
}
