
#include "lvgl.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "candle.h"

/* -------------------------------------------------
 * Configuration
 * ------------------------------------------------- */

#define CANVAS_BG_COLOR       0x004D65

#define CANDLE_COLOR_UP       0x26A69A
#define CANDLE_COLOR_DOWN     0xEF5350

#define CANDLE_COLOR_GRID     0xB0B0B0
#define CANDLE_COLOR_AXIS     0xFFFFFF

#define CANDLE_COLOR_MARKER   0xEF5350

#define GRID_LINES            5

#define CHART_PADDING_TOP     8
#define CHART_PADDING_BOTTOM  8

#define CANDLE_BODY_MIN_WIDTH 8

#define CANDLE_FONT           LV_FONT_DEFAULT


/* -------------------------------------------------
 * Format price
 *
 * Input:  64882 = 64.882
 * Output: German number formatting
 * ------------------------------------------------- */

static void candle_format_price(
    int64_t price,
    char *buffer,
    size_t buffer_size
) {
    uint64_t value = (price < 0)
        ? (uint64_t)(-(price + 1)) + 1
        : (uint64_t)price;

    char raw[32];
    char formatted[48];

    snprintf(raw, sizeof(raw), "%llu",
             (unsigned long long)value);

    int len = strlen(raw);
    int pos = 0;

    if(price < 0)
        formatted[pos++] = '-';

    for(int i = 0; i < len; i++) {
        formatted[pos++] = raw[i];

        int remaining = len - i - 1;

        if(remaining > 0 && remaining % 3 == 0)
            formatted[pos++] = '.';
    }

    formatted[pos] = '\0';

    snprintf(buffer, buffer_size, "%s", formatted);
}


/* -------------------------------------------------
 * Calculate nice grid interval
 *
 * Example:
 * 50000 = 500.00 price units
 * ------------------------------------------------- */

static int64_t candle_nice_step(
    int64_t price_min,
    int64_t price_max
) {
    double range = (double)(price_max - price_min);

    double raw = range / (GRID_LINES - 1);

    if(raw < 1.0)
        return 1;

    double exponent = floor(log10(raw));
    double base = pow(10.0, exponent);

    double fraction = raw / base;

    double nice_fraction;

    if(fraction <= 1.0)
        nice_fraction = 1.0;
    else if(fraction <= 2.0)
        nice_fraction = 2.0;
    else if(fraction <= 2.5)
        nice_fraction = 2.5;
    else if(fraction <= 5.0)
        nice_fraction = 5.0;
    else
        nice_fraction = 10.0;

    return (int64_t)(nice_fraction * base);
}


/* -------------------------------------------------
 * Price to Y coordinate
 * ------------------------------------------------- */

static int32_t candle_price_to_y(
    int64_t price,
    int64_t price_min,
    int64_t price_max,
    int32_t chart_top,
    int32_t chart_bottom
) {
    double range =
        (double)price_max - (double)price_min;

    if(range <= 0.0)
        return chart_top;

    double normalized =
        ((double)price_max - (double)price) / range;

    if(normalized < 0.0) normalized = 0.0;
    if(normalized > 1.0) normalized = 1.0;

    return chart_top +
        (int32_t)(
            normalized * (chart_bottom - chart_top) + 0.5
        );
}


/* -------------------------------------------------
 * Draw horizontal line
 * ------------------------------------------------- */

static void candle_draw_line(
    lv_layer_t *layer,
    int32_t x1,
    int32_t y1,
    int32_t x2,
    int32_t y2,
    lv_color_t color,
    int32_t width,
    int32_t dash_width,
    int32_t dash_gap,
    lv_opa_t opa
) {
    lv_draw_line_dsc_t dsc;
    lv_draw_line_dsc_init(&dsc);

    dsc.color = color;
    dsc.width = width;
    dsc.opa = opa;

    dsc.dash_width = dash_width;
    dsc.dash_gap = dash_gap;

    dsc.p1.x = x1;
    dsc.p1.y = y1;

    dsc.p2.x = x2;
    dsc.p2.y = y2;

    lv_draw_line(layer, &dsc);
}


/* -------------------------------------------------
 * Draw text
 * ------------------------------------------------- */

static void candle_draw_text(
    lv_layer_t *layer,
    const char *text,
    int32_t x,
    int32_t y,
    int32_t width,
    int32_t height,
    lv_color_t color,
    lv_text_align_t align
) {
    lv_draw_label_dsc_t dsc;
    lv_draw_label_dsc_init(&dsc);

    dsc.color = color;
    dsc.font = CANDLE_FONT;
    dsc.text = text;
    dsc.align = align;
    dsc.opa = LV_OPA_COVER;

    lv_area_t area = {
        .x1 = x,
        .y1 = y,
        .x2 = x + width - 1,
        .y2 = y + height - 1
    };

    lv_draw_label(layer, &dsc, &area);
}


/* -------------------------------------------------
 * Draw one daily candlestick
 *
 * canvas: Existing LVGL canvas
 * data:   Candle price data
 *
 * priceClose = actual/current price
 * ------------------------------------------------- */

bool candle_stick_day_daw(
    lv_obj_t *canvas,
    const CandleChartData *data
) {
    if(canvas == NULL || data == NULL)
        return false;

    int32_t width = lv_obj_get_width(canvas);
    int32_t height = lv_obj_get_height(canvas);

    if(width < 100 || height < 60)
        return false;

    if(data->priceHight <= data->priceLow)
        return false;

    /* ---------------------------------------------
     * Chart dimensions
     * --------------------------------------------- */

    int32_t chart_top = CHART_PADDING_TOP;
    int32_t chart_bottom = height - CHART_PADDING_BOTTOM - 1;

    int32_t font_height =
        lv_font_get_line_height(CANDLE_FONT);

    /* ---------------------------------------------
     * Calculate automatic price range
     * --------------------------------------------- */

    int64_t price_step = candle_nice_step(
        data->priceLow,
        data->priceHight
    );

    if(price_step < 1)
        return false;

    int64_t price_min =
        (int64_t)floor(
            (double)data->priceLow / price_step
        ) * price_step;

    int64_t price_max =
        (int64_t)ceil(
            (double)data->priceHight / price_step
        ) * price_step;

    if(price_max <= price_min)
        return false;

    /* ---------------------------------------------
     * Layout
     * --------------------------------------------- */

    int32_t axis_width = width * 18 / 100;

    int32_t chart_left = axis_width;
    int32_t chart_right = width - 5;

    int32_t center_x =
        chart_left + (chart_right - chart_left) * 52 / 100;

    int32_t body_width = width / 7;

    if(body_width < CANDLE_BODY_MIN_WIDTH)
        body_width = CANDLE_BODY_MIN_WIDTH;

    int32_t body_left = center_x - body_width / 2;
    int32_t body_right = center_x + body_width / 2;

    /* ---------------------------------------------
     * Price coordinates
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
     * Candle direction
     * --------------------------------------------- */

    bool bullish =
        data->priceClose >= data->priceOpen;

    lv_color_t candle_color = lv_color_hex(
        bullish ? CANDLE_COLOR_UP : CANDLE_COLOR_DOWN
    );

    lv_color_t marker_color = candle_color;

    /* ---------------------------------------------
     * Clear canvas with ORIGINAL background color
     * --------------------------------------------- */

    lv_canvas_fill_bg(
        canvas,
        lv_color_hex(CANVAS_BG_COLOR),
        LV_OPA_COVER
    );

    lv_layer_t layer;

    lv_canvas_init_layer(canvas, &layer);

    /* ---------------------------------------------
     * Draw grid and price axis
     * --------------------------------------------- */

    for(int i = 0; i < GRID_LINES; i++) {

        int64_t tick_price =
            price_min + (price_max - price_min) * i
            / (GRID_LINES - 1);

        int32_t y = candle_price_to_y(
            tick_price,
            price_min,
            price_max,
            chart_top,
            chart_bottom
        );

        /* Dashed gray grid */
        candle_draw_line(
            &layer,
            chart_left,
            y,
            chart_right,
            y,
            lv_color_hex(CANDLE_COLOR_GRID),
            2,
            10,
            8,
            LV_OPA_60
        );

        /* Left price labels */
        char price_text[32];

        candle_format_price(
            tick_price,
            price_text,
            sizeof(price_text)
        );

        candle_draw_text(
            &layer,
            price_text,
            0,
            y - font_height / 2,
            axis_width - 8,
            font_height + 2,
            lv_color_hex(CANDLE_COLOR_AXIS),
            LV_TEXT_ALIGN_RIGHT
        );
    }

    /* ---------------------------------------------
     * Draw High / Low wick
     * --------------------------------------------- */

    candle_draw_line(
        &layer,
        center_x,
        y_high,
        center_x,
        y_low,
        candle_color,
        3,
        0,
        0,
        LV_OPA_COVER
    );

    /* ---------------------------------------------
     * Draw Open / Close marker lines
     * --------------------------------------------- */

    int32_t marker_start = body_right + 2;

    candle_draw_line(
        &layer,
        marker_start,
        y_open,
        chart_right,
        y_open,
        marker_color,
        2,
        10,
        8,
        LV_OPA_80
    );

    candle_draw_line(
        &layer,
        marker_start,
        y_close,
        chart_right,
        y_close,
        marker_color,
        2,
        10,
        8,
        LV_OPA_80
    );

    /* ---------------------------------------------
     * Draw candle body
     * --------------------------------------------- */

    int32_t body_top =
        y_open < y_close ? y_open : y_close;

    int32_t body_bottom =
        y_open > y_close ? y_open : y_close;

    if(body_bottom - body_top < 2)
        body_bottom = body_top + 2;

    lv_draw_rect_dsc_t body_dsc;
    lv_draw_rect_dsc_init(&body_dsc);

    body_dsc.bg_color = candle_color;
    body_dsc.bg_opa = LV_OPA_COVER;
    body_dsc.border_width = 0;
    body_dsc.radius = 0;

    lv_area_t body_area = {
        .x1 = body_left,
        .y1 = body_top,
        .x2 = body_right,
        .y2 = body_bottom
    };

    lv_draw_rect(
        &layer,
        &body_dsc,
        &body_area
    );

    /* ---------------------------------------------
     * Open / Close labels on the right
     * --------------------------------------------- */

    char open_text[48];
    char close_text[48];

    snprintf(open_text, sizeof(open_text),
             "Open %s", "");

    snprintf(close_text, sizeof(close_text),
             "Close %s", "");

    char open_price[32];
    char close_price[32];

    candle_format_price(
        data->priceOpen,
        open_price,
        sizeof(open_price)
    );

    candle_format_price(
        data->priceClose,
        close_price,
        sizeof(close_price)
    );

    snprintf(open_text, sizeof(open_text),
             "Open %s", open_price);

    snprintf(close_text, sizeof(close_text),
             "Close %s", close_price);

    int32_t label_x = marker_start + 10;
    int32_t label_width = width - label_x - 4;

    if(label_width > 0) {

        /* Open label above its marker line */
        candle_draw_text(
            &layer,
            open_text,
            label_x,
            y_open - font_height - 3,
            label_width,
            font_height + 2,
            marker_color,
            LV_TEXT_ALIGN_LEFT
        );

        /* Close label below its marker line */
        candle_draw_text(
            &layer,
            close_text,
            label_x,
            y_close + 3,
            label_width,
            font_height + 2,
            marker_color,
            LV_TEXT_ALIGN_LEFT
        );
    }

    /* ---------------------------------------------
     * Finish canvas
     * --------------------------------------------- */

    lv_canvas_finish_layer(canvas, &layer);

    return true;
}
