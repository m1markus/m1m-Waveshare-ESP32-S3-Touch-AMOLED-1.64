#ifndef MY_CANDLE_H
#define MY_CANDLE_H

#include "lvgl.h"
#include "my_app.h"

#ifdef __cplusplus
extern "C" {
#endif

bool candle_stick_day_daw(
    lv_obj_t *canvas,
    const CandleChartData *data
);

#ifdef __cplusplus
}
#endif

#endif
