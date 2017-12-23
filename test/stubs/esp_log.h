#pragma once

#define ESP_LOGE(tag, format, ...) (printf(format "\n", ##__VA_ARGS__))
#define ESP_LOGW(tag, format, ...) (printf(format "\n", ##__VA_ARGS__))
#define ESP_LOGI(tag, format, ...) (printf(format "\n", ##__VA_ARGS__))
#define ESP_LOGD(tag, format, ...) (printf(format "\n", ##__VA_ARGS__))
#define ESP_LOGV(tag, format, ...) (printf(format "\n", ##__VA_ARGS__))
