#include <pebble.h>
#include "shadow_analog.h"
#include "effect_layer.h"  

static Window *window;
static Layer  *s_hands_layer;

static EffectLayer *s_effect_layer;
static  EffectOffset s_effect_offset;
#define SHADOW_LENGTH 120

GColor hand_color, shadow_color;


static void hands_update_proc(Layer *layer, GContext *ctx) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  
  graphics_context_set_stroke_color(ctx, hand_color);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_context_set_antialiased(ctx, false);
    
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);
  
  //minute hand
  int16_t hand_length = bounds.size.w / 2;
  int32_t angle = TRIG_MAX_ANGLE * t->tm_min / 60;
  GPoint hand = {
    .x = (int16_t)(sin_lookup(angle) * (int32_t)hand_length / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(angle) * (int32_t)hand_length / TRIG_MAX_RATIO) + center.y,
  };
  graphics_draw_line(ctx, hand, center);
  
  //hour hand
  hand_length = hand_length - 20;
  angle = (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6);
  hand = (GPoint){
    .x = (int16_t)(sin_lookup(angle) * (int32_t)hand_length / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(angle) * (int32_t)hand_length / TRIG_MAX_RATIO) + center.y,
  };
  graphics_draw_line(ctx, hand, center);
  
  
}



static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  //adjusting shadow direction according to minute hand location
  if (tick_time->tm_min > 0 && tick_time->tm_min < 15) {
    s_effect_offset.offset_x = SHADOW_LENGTH;
    s_effect_offset.offset_y = SHADOW_LENGTH;
  } else if (tick_time->tm_min > 15 && tick_time->tm_min < 30) {
    s_effect_offset.offset_x = -SHADOW_LENGTH;
    s_effect_offset.offset_y = SHADOW_LENGTH;
  } else if (tick_time->tm_min > 30 && tick_time->tm_min < 45) {  
    s_effect_offset.offset_x = -SHADOW_LENGTH;
    s_effect_offset.offset_y = -SHADOW_LENGTH;  
  } else {
    s_effect_offset.offset_x = SHADOW_LENGTH;
    s_effect_offset.offset_y = -SHADOW_LENGTH;  
  }  
  layer_mark_dirty(window_get_root_layer(window));
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  layer_add_child(window_layer, s_hands_layer);
  
  //creating shadow layer
  s_effect_offset = (EffectOffset){
    .orig_color = hand_color,
    .offset_color = shadow_color,
    .option = 1
  };
  
  s_effect_layer = effect_layer_create(bounds);
  effect_layer_add_effect(s_effect_layer, effect_shadow, &s_effect_offset);
  effect_layer_add_effect(s_effect_layer, effect_blur, (void*)1);
  layer_add_child(window_layer, effect_layer_get_layer(s_effect_layer));
  
}

static void window_unload(Window *window) {
  layer_destroy(s_hands_layer);
}

static void init() {
  
  hand_color = GColorRed;
  shadow_color = GColorWhite;
  
  
  window = window_create();
  window_set_background_color(window, GColorBlack);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, handle_second_tick);
}

static void deinit() {
  tick_timer_service_unsubscribe();
  window_destroy(window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}