/**
 * NOTE: 重複 include の防止
 * clox_common_h が定義されていない場合，
 * clox_common_h を定義したうえでヘッダーファイルを include する
 */
#ifndef clox_common_h
#define clox_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// #define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION

#define UINT8_COUNT (UINT8_MAX + 1)

#endif
