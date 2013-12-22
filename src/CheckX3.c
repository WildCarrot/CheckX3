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

typedef struct _ListItem {
	TextLayer* text_layer;
	char text_buffer[LIST_ITEM_LEN];
	bool highlighted;
	bool completed;
	struct _ListItem* prev;
	struct _ListItem* next;
} ListItem;

static ListItem* list_items = NULL;
static ListItem* highlighted_item = NULL;

static void highlight_item(ListItem* item, bool highlight)
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

// XXX This is icky: modifies the globals, not parameters.
/* static void complete_highlighted_item() */
/* { */
/* 	// For now, change the text */

/* 	// "Remove" an item by moving all the items up. */
/* 	// This is icky, but we have to recompute the frame for the text layers after */
/* 	// the one we're removing, so we'll have to loop sometime. */
/* 	for (int i=highlighted_item; i < num_visible_items; ++i) { */
/* 		// Remove the highlighted item. */
/* 		scroll_layer_remove_child(scroll_layer, */
/* 					  text_layer_get_layer(list_items[highlighted_item].text_layer)); */
/* 		--num_visible_items; */
/* 	} */
/* 	else { */
/* 		memmove(&(item_list[to_remove]), & */
/* } */

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Clicked on item %p", highlighted_item);

	if (highlighted_item == NULL) {
		return;
	}

	// Use the next item as the new highlight, if there is one.
	ListItem* new_highlight = highlighted_item->next;
	if (new_highlight == NULL) {
		new_highlight = highlighted_item->prev;
	}

	APP_LOG(APP_LOG_LEVEL_DEBUG, "New highlight item %p", new_highlight);

	// Remove the highlighted item.
	GRect item_bounds = layer_get_frame((Layer*) highlighted_item->text_layer);
	layer_remove_from_parent((Layer*) highlighted_item->text_layer);
	text_layer_destroy(highlighted_item->text_layer);

	// Update list pointers.
	if (highlighted_item->prev != NULL) {
		highlighted_item->prev->next = highlighted_item->next;
	}
	if (highlighted_item->next != NULL) {
		highlighted_item->next->prev = highlighted_item->prev;
	}
	free(highlighted_item);
	highlighted_item = new_highlight;
	if (new_highlight == NULL) {
		return;
	}

	// Highlight the new highlighted item.
	highlight_item(highlighted_item, true);

	// Update all the items at and after the newly highlighted item
	// with their new graphics position.
	ListItem* item = highlighted_item;
	while (item != NULL) {
		GRect curr_bounds = layer_get_frame((Layer*) item->text_layer);
		if (curr_bounds.origin.y > item_bounds.origin.y) {
			layer_set_frame((Layer*) item->text_layer, item_bounds);
			item_bounds.origin.y += LIST_ITEM_HEIGHT;
		}
		item = item->next;
	}

	// We removed only one item, so adjust the content size by that much.
	GSize scroll_size = scroll_layer_get_content_size(scroll_layer);
	scroll_size.h -= LIST_ITEM_HEIGHT;
	// Don't try to size negatively.
	if (scroll_size.h > 0) {
		scroll_layer_set_content_size(scroll_layer, scroll_size);
	}
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
	if ((highlighted_item != NULL) && (highlighted_item->prev != NULL)) {
		highlight_item(highlighted_item, false);
		highlighted_item = highlighted_item->prev;
		highlight_item(highlighted_item, true);

		// If the newly highlighted item is clipped, scroll up.
		// The offset will be negative when scrolling up.
		GRect item_frame = layer_get_frame((const Layer*) highlighted_item->text_layer);
		if (item_frame.origin.y + scroll_layer_get_content_offset(scroll_layer).y < LIST_ITEM_HEIGHT) {
			scroll_layer_scroll_up_click_handler(recognizer, scroll_layer);
		}
	}
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
	if ((highlighted_item != NULL) && (highlighted_item->next != NULL)) {
		highlight_item(highlighted_item, false);
		highlighted_item = highlighted_item->next;
		highlight_item(highlighted_item, true);

		// If the newly highlighted item is clipped, scroll down.
		GRect item_frame = layer_get_frame((const Layer*) highlighted_item->text_layer);
		if (item_frame.origin.y + item_frame.size.h > 130) { // XXX near SCREEN_HEIGHT
			scroll_layer_scroll_down_click_handler(recognizer, scroll_layer);
		}
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

	GRect item_bounds = bounds;
	item_bounds.size.h = 20;

	// XXX for testing only: eventually need to get the list from RTM
	int num_items = 15;

	ListItem* last_item = NULL;
	ListItem* item = NULL;
	for (int i=0; i < num_items; ++i) {
		item = (ListItem*) malloc(sizeof(ListItem));
		memset(item, '\0', sizeof(ListItem));

		snprintf(item->text_buffer, LIST_ITEM_LEN, "Item %d", i);

		item->text_layer = text_layer_create(item_bounds);
		text_layer_set_text(item->text_layer, item->text_buffer);
		text_layer_set_text_alignment(item->text_layer, GTextAlignmentLeft);
		scroll_layer_add_child(scroll_layer, text_layer_get_layer(item->text_layer));

		item->prev = last_item;
		if (item->prev != NULL) {
			item->prev->next = item;
		}
		APP_LOG(APP_LOG_LEVEL_DEBUG, "last_item=%p item=%p text=%s",
			last_item, item, item->text_buffer);

		if (list_items == NULL) {
			list_items = item;
			highlighted_item = item;
		}

		last_item = item;
		item = NULL;

		// XXX Fix for text size.
		item_bounds.origin.y += 20;
	}

	scroll_layer_set_content_size(scroll_layer, (GSize) { bounds.size.w, item_bounds.origin.y });

	layer_add_child(window_layer, (Layer*) scroll_layer);
}

static void window_appear(Window* window) {
	if (highlighted_item != NULL) {
		highlight_item(highlighted_item, true);
	}
}

static void window_unload(Window *window) {
	ListItem* item = list_items;
	while (item != NULL) {
		text_layer_destroy(item->text_layer);
		ListItem* next = item->next;
		free(item);
		item = next;
	}
	highlighted_item = NULL;
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
