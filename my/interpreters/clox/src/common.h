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

// #define DEBUG_TRACE_EXECUTION // オペコードやスタックの値を出力するモード．@warning 性能には悪影響を及ぼす．

// #define DEBUG_STRESS_GC // GCをメモリの追加割り当てを行う度に実行し，メモリ管理のバグを見つけやすくする，ストレステストモード．@warning 性能には悪影響を及ぼす．
// #define DEBUG_LOG_GC // 動的メモリで何か行う度に，その情報を出力するロギングモード．@warning 性能には悪影響を及ぼす．

#define UINT8_COUNT (UINT8_MAX + 1)

#endif
