#include <pebble.h>
#include "shadow_analog.h"
#include <pebble-effect-layer/pebble-effect-layer.h>

static Window *window;
static Layer  *s_hands_layer;

static EffectLayer *s_effect_layer;
static  EffectOffset s_effect_offset;

#ifdef PBL_RECT
  int SHADOW_LENGTH = 120;
#else
  int SHADOW_LENGTH = 30;
#endif

GColor hand_color, shadow_color;

#ifndef PBL_COLOR //for Aplite - defining array that would hold set pixels (for Shadow effect)
  uint8_t *aplite_visited;
#endif

#ifndef PBL_SDK_2
static void app_focus_changed(bool focused) {
  if (focused) { // on resuming focus - restore background
    layer_mark_dirty(effect_layer_get_layer(s_effect_layer));
  }
}
#endif

static void hands_update_proc(Layer *layer, GContext *ctx) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  
  graphics_context_set_stroke_color(ctx, hand_color);
  
  #ifdef PBL_COLOR
    graphics_context_set_antialiased(ctx, false);
  #endif
  
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);
  
  //minute hand
  int16_t hand_length = bounds.size.w / 2 ;
  int32_t angle = TRIG_MAX_ANGLE * t->tm_min / 60;
  GPoint hand = {
    .x = (int16_t)(sin_lookup(angle) * (int32_t)hand_length / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(angle) * (int32_t)hand_length / TRIG_MAX_RATIO) + center.y,
  };
  #ifdef PBL_COLOR
    graphics_context_set_stroke_width(ctx, 3);
  #endif
  graphics_draw_line(ctx, hand, center);
  
  //hour hand
  hand_length = hand_length - 25;
  angle = (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6);
  hand = (GPoint){
    .x = (int16_t)(sin_lookup(angle) * (int32_t)hand_length / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(angle) * (int32_t)hand_length / TRIG_MAX_RATIO) + center.y,
  };
  #ifdef PBL_COLOR
   graphics_context_set_stroke_width(ctx, 5);
  #endif
  graphics_draw_line(ctx, hand, center);
  
  #ifndef PBL_RECT
    graphics_context_set_fill_color(ctx, hand_color);
    graphics_fill_circle(ctx, center, 7);
  #endif
  
}



static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  
  #ifndef PBL_COLOR
     memset(aplite_visited, 0, 168*20);
  #endif
  
 
  //adjusting shadow direction according to minute hand location
  if (tick_time->tm_min >= 0 && tick_time->tm_min < 15) {
    s_effect_offset.offset_x = SHADOW_LENGTH;
    s_effect_offset.offset_y = SHADOW_LENGTH;
  } else if (tick_time->tm_min >= 15 && tick_time->tm_min < 30) {
    s_effect_offset.offset_x = -SHADOW_LENGTH;
    s_effect_offset.offset_y = SHADOW_LENGTH;
  } else if (tick_time->tm_min >= 30 && tick_time->tm_min < 45) {  
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
      
    // creating array for "visited" pixels and assigning it to shadow effect parameter
    #ifndef PBL_COLOR  
      ,
      .aplite_visited = aplite_visited
    #endif 
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
  
  #ifndef PBL_SDK_2
  // need to catch when app resumes focus after notification, otherwise background won't restore
  app_focus_service_subscribe_handlers((AppFocusHandlers){
    .did_focus = app_focus_changed
  });
  #endif

  
  
  #ifdef PBL_COLOR
    hand_color = GColorRed;
  #else
    hand_color = GColorWhite;
    aplite_visited = malloc(168*20);
  #endif   
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
  
  

  #ifndef PBL_SDK_2
    app_focus_service_unsubscribe();
  #endif
  
  #ifndef PBL_COLOR
    free(aplite_visited);
  #endif
  tick_timer_service_unsubscribe();
  window_destroy(window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}