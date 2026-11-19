#ifndef MY_APP_H
#define MY_APP_H

#ifdef __cplusplus
extern "C" {
#endif

struct CandleChartData {
    int64_t priceOpen;
    int64_t priceClose;  // actual price
    int64_t priceHight;
    int64_t priceLow;
};

struct UiCommandItem {
    int uiCommand;
    char price[32];
    char timeAt[32];
    CandleChartData candleChartData;
};

#ifdef __cplusplus
}
#endif

#endif
