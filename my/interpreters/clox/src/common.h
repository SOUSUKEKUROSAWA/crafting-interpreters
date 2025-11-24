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

/**
 * @note NaN ボックス化（タグ化）
 *  double 型にはNaNという特別な形式があり，
 *  NaNにはシグナリングNaNとクワイエットNaNの2種類がある．
 *  シグナリングNaNはゼロによる除算のような誤った計算の結果に使われるが，
 *  クワイエットNaNは特に有効な意味を持たず，触っても危険がない状態で残されている．
 *  ここで，使われていないクワイエットNaNの 51 bit （2 の 51 乗パターン）に意味を持たせ，
 *  メモリの節約をすることで，キャッシュラインに載る Value の量が増えるため，
 *  キャッシュミスが減り，スピードが向上させるという最適化手法．
 */
#define NAN_BOXING // NaN ボックス化（タグ化）による値表現の最適化を有効化するモード．@warning 全てのCPUアーキテクチャで動作するとは限らない．

// #define DEBUG_PRINT_CODE

// #define DEBUG_TRACE_EXECUTION // オペコードやスタックの値を出力するモード．@warning 性能には悪影響を及ぼす．

// #define DEBUG_STRESS_GC // GCをメモリの追加割り当てを行う度に実行し，メモリ管理のバグを見つけやすくする，ストレステストモード．@warning 性能には悪影響を及ぼす．
// #define DEBUG_LOG_GC // 動的メモリで何か行う度に，その情報を出力するロギングモード．@warning 性能には悪影響を及ぼす．

#define UINT8_COUNT (UINT8_MAX + 1)

#endif
