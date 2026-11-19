
#include "lvgl.h"
#include <stdint.h>

#include "candle.h"

#define CANVAS_BG_COLOR       0x004D65

/* -------------------------------------------------
 * Configuration
 * ------------------------------------------------- */

#define CANDLE_COLOR_UP       0x26A69A
#define CANDLE_COLOR_DOWN     0xEF5350
#define CANDLE_COLOR_GRID     0x454545

#define CANDLE_BODY_MAX_WIDTH 24
#define CANDLE_BODY_MIN_WIDTH 6

#define CHART_PADDING         5
#define GRID_LINES            5


/* -------------------------------------------------
 * Price to Y coordinate
 *
 * Higher price = smaller Y
 * Lower price  = larger Y
 * ------------------------------------------------- */

static int32_t candle_price_to_y(
    int64_t price,
    int64_t price_min,
    int64_t price_max,
    int32_t chart_top,
    int32_t chart_bottom
) {
    double range = (double)price_max - (double)price_min;

    if(range <= 0.0) return chart_top;

    double normalized =
        ((double)price_max - (double)price) / range;

    /* Clamp to chart boundaries */
    if(normalized < 0.0) normalized = 0.0;
    if(normalized > 1.0) normalized = 1.0;

    return chart_top +
        (int32_t)(normalized * (chart_bottom - chart_top) + 0.5);
}


/* -------------------------------------------------
 * Draw horizontal grid lines
 * ------------------------------------------------- */

static void candle_draw_grid(
    lv_layer_t *layer,
    int32_t width,
    int32_t top,
    int32_t bottom
) {
    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);

    line_dsc.color = lv_color_hex(CANDLE_COLOR_GRID);
    line_dsc.width = 1;
    line_dsc.opa = LV_OPA_COVER;

    for(int i = 0; i < GRID_LINES; i++) {

        int32_t y = top +
            (bottom - top) * i / (GRID_LINES - 1);

        line_dsc.p1.x = 0;
        line_dsc.p1.y = y;

        line_dsc.p2.x = width - 1;
        line_dsc.p2.y = y;

        lv_draw_line(layer, &line_dsc);
    }
}


/* -------------------------------------------------
 * Draw one daily candle
 *
 * Parameters:
 * 1. Existing LVGL canvas object
 * 2. Candle OHLC price data
 *
 * Returns:
 * true  = successfully drawn
 * false = invalid input or canvas
 * ------------------------------------------------- */

bool candle_stick_day_daw(
    lv_obj_t *canvas,
    const CandleChartData *data
) {
    if(canvas == NULL || data == NULL)
        return false;

    int32_t width = lv_obj_get_width(canvas);
    int32_t height = lv_obj_get_height(canvas);

    if(width < 10 || height < 10)
        return false;

    if(data->priceHight <= data->priceLow)
        return false;

    /* ---------------------------------------------
     * Chart dimensions
     * --------------------------------------------- */

    int32_t chart_top = CHART_PADDING;
    int32_t chart_bottom = height - CHART_PADDING - 1;

    int64_t price_min = data->priceLow;
    int64_t price_max = data->priceHight;

    /* ---------------------------------------------
     * Map prices to pixel coordinates
     * --------------------------------------------- */

    int32_t y_high = candle_price_to_y(
        data->priceHight,
        price_min,
        price_max,
        chart_top,
        chart_bottom
    );

    int32_t y_low = candle_price_to_y(
        data->priceLow,
        price_min,
        price_max,
        chart_top,
        chart_bottom
    );

    int32_t y_open = candle_price_to_y(
        data->priceOpen,
        price_min,
        price_max,
        chart_top,
        chart_bottom
    );

    int32_t y_close = candle_price_to_y(
        data->priceClose,
        price_min,
        price_max,
        chart_top,
        chart_bottom
    );

    /* ---------------------------------------------
     * Determine candle direction
     *
     * Close = actual/current price
     * --------------------------------------------- */

    bool bullish = data->priceClose >= data->priceOpen;

    lv_color_t candle_color = lv_color_hex(
        bullish ? CANDLE_COLOR_UP : CANDLE_COLOR_DOWN
    );

    /* ---------------------------------------------
     * Initialize canvas drawing layer
     * --------------------------------------------- */

    lv_layer_t layer;

    lv_canvas_fill_bg(canvas, lv_color_hex(CANVAS_BG_COLOR), LV_OPA_COVER);
    lv_canvas_init_layer(canvas, &layer);

    /* ---------------------------------------------
     * Draw supporting grid
     * --------------------------------------------- */

    candle_draw_grid(
        &layer,
        width,
        chart_top,
        chart_bottom
    );

    /* ---------------------------------------------
     * Candle position and width
     * --------------------------------------------- */

    int32_t center_x = width / 2;

    int32_t body_width = width / 8;

    if(body_width > CANDLE_BODY_MAX_WIDTH)
        body_width = CANDLE_BODY_MAX_WIDTH;

    if(body_width < CANDLE_BODY_MIN_WIDTH)
        body_width = CANDLE_BODY_MIN_WIDTH;

    if(body_width > width - 2)
        body_width = width - 2;

    /* ---------------------------------------------
     * Draw wick (High -> Low)
     * --------------------------------------------- */

    lv_draw_line_dsc_t wick_dsc;
    lv_draw_line_dsc_init(&wick_dsc);

    wick_dsc.color = candle_color;
    wick_dsc.width = 2;
    wick_dsc.opa = LV_OPA_COVER;

    wick_dsc.p1.x = center_x;
    wick_dsc.p1.y = y_high;

    wick_dsc.p2.x = center_x;
    wick_dsc.p2.y = y_low;

    lv_draw_line(&layer, &wick_dsc);

    /* ---------------------------------------------
     * Draw candle body (Open -> Actual price)
     * --------------------------------------------- */

    int32_t body_top =
        y_open < y_close ? y_open : y_close;

    int32_t body_bottom =
        y_open > y_close ? y_open : y_close;

    int32_t body_height = body_bottom - body_top + 1;

    /* Ensure visible body for very small price changes */
    if(body_height < 2)
        body_height = 2;

    lv_draw_rect_dsc_t body_dsc;
    lv_draw_rect_dsc_init(&body_dsc);

    body_dsc.bg_color = candle_color;
    body_dsc.bg_opa = LV_OPA_COVER;
    body_dsc.border_width = 0;
    body_dsc.radius = 0;

    lv_area_t body_area = {
        .x1 = center_x - body_width / 2,
        .y1 = body_top,
        .x2 = center_x + (body_width - 1) / 2,
        .y2 = body_top + body_height - 1
    };

    lv_draw_rect(&layer, &body_dsc, &body_area);

    /* ---------------------------------------------
     * Finish canvas drawing
     * --------------------------------------------- */

    lv_canvas_finish_layer(canvas, &layer);

    return true;
}
