#ifndef S21_DECIMAL_H
#define S21_DECIMAL_H

#include <stdint.h>
#include <stdlib.h>

typedef struct s21_decimal {
  int bits[4];
} s21_decimal;

typedef struct s21_int256 {
  s21_decimal decimals[2];
} s21_int256;

#define MAX_BITS 128
#define MAX_BLOCK_BITS 32
#define MAX_BLOCK_NUMBER 96

typedef union decimal_bit3 {
  int i;
  struct {
    uint32_t empty2 : 16;
    uint32_t power : 8;
    uint32_t empty1 : 7;
    uint32_t sign : 1;
  } parts;
} decimal_bit3;

typedef enum s21_decimal_sign {
  S21_POSITIVE = 0,
  S21_NEGATIVE = 1
} s21_decimal_sign;

int s21_add(s21_decimal value_1, s21_decimal value_2, s21_decimal *result);
int s21_sub(s21_decimal value_1, s21_decimal value_2, s21_decimal *result);
int s21_mul(s21_decimal value_1, s21_decimal value_2, s21_decimal *result);
int s21_div(s21_decimal value_1, s21_decimal value_2, s21_decimal *result);
int s21_mod(s21_decimal value_1, s21_decimal value_2, s21_decimal *result);

int s21_add_handle(s21_decimal value_1, s21_decimal value_2,
                   s21_decimal *result);
int s21_sub_handle(s21_decimal value_1, s21_decimal value_2,
                   s21_decimal *result);
int s21_mul_handle(s21_decimal value_1, s21_decimal value_2,
                   s21_decimal *result);
int s21_div_handle(s21_int256 value_2l, s21_int256 res, s21_int256 remainder,
                   s21_decimal *result);
int s21_div_calc_fractional(s21_int256 *res, s21_int256 value_2l,
                            s21_int256 *remainder);

void s21_decimal_leveling(s21_decimal value_1, s21_decimal value_2,
                          s21_int256 *value_1l, s21_int256 *value_2l);
s21_decimal s21_abs(s21_decimal value_1);

int s21_int256_get_shift_to_decimal(s21_int256 value);
int s21_max(int value_1, int value_2);

typedef enum s21_arithmetic_result {
  S21_ARITHMETIC_OK = 0,
  S21_ARITHMETIC_BIG = 1,
  S21_ARITHMETIC_SMALL = 2,
  S21_ARITHMETIC_ZERO_DIV = 3,
  S21_ARITHMETIC_ERROR = 4
} s21_arithmetic_result;

int s21_is_set_bit(int number, int index);
int s21_set_bit(int number, int index);
int s21_reset_bit(int number, int index);

int s21_decimal_is_set_bit(s21_decimal decimal, int index);
s21_decimal s21_decimal_set_bit(s21_decimal decimal, int index);
int s21_decimal_get_not_zero_bit(s21_decimal decimal);

int s21_int128_binary_equal_zero(s21_decimal decimal);
int s21_int128_binary_compare(s21_decimal d1, s21_decimal d2);
int s21_int256_binary_compare(s21_int256 d1, s21_int256 d2);

s21_decimal s21_int128_binary_and(s21_decimal decimal1, s21_decimal decimal2);
s21_decimal s21_int128_binary_xor(s21_decimal decimal1, s21_decimal decimal2);
s21_decimal s21_int128_binary_not(s21_decimal decimal);

s21_decimal s21_int128_binary_addition(s21_decimal decimal1,
                                       s21_decimal decimal2);
s21_decimal s21_int128_binary_subtraction(s21_decimal decimal1,
                                          s21_decimal decimal2);
s21_decimal s21_int128_binary_division(s21_decimal decimal1,
                                       s21_decimal decimal2,
                                       s21_decimal *remainder);
s21_int256 s21_int128_binary_multiplication(s21_decimal decimal1,
                                            s21_decimal decimal2);

s21_int256 s21_int256_binary_addition(s21_int256 decimal1, s21_int256 decimal2);
s21_int256 s21_int256_binary_subtraction(s21_int256 decimal1,
                                         s21_int256 decimal2);
s21_int256 s21_int256_binary_division(s21_int256 decimal1, s21_int256 decimal2,
                                      s21_int256 *remainder);
s21_int256 s21_int256_binary_multiplication(s21_int256 decimal1,
                                            s21_decimal decimal2);

s21_decimal s21_int128_binary_shift_left(s21_decimal decimal, int shift);
s21_decimal s21_int128_binary_shift_right(s21_decimal decimal, int shift);
s21_decimal s21_int128_binary_shift_left_one(s21_decimal decimal);
s21_decimal s21_int128_binary_shift_right_one(s21_decimal decimal);
s21_int256 s21_int256_binary_shift_left(s21_int256 decimal, int shift);
s21_int256 s21_int256_binary_shift_right(s21_int256 decimal, int shift);

int s21_is_less(s21_decimal value_1, s21_decimal value_2);
int s21_is_less_or_equal(s21_decimal value_1, s21_decimal value_2);
int s21_is_greater(s21_decimal value_1, s21_decimal value_2);
int s21_is_greater_or_equal(s21_decimal value_1, s21_decimal value_2);
int s21_is_equal(s21_decimal decimal1, s21_decimal decimal2);
int s21_is_not_equal(s21_decimal value_1, s21_decimal value_2);

int s21_is_less_handle(s21_decimal value_1, s21_decimal value_2);

typedef enum s21_comparison_result {
  S21_COMPARISON_FALSE = 0,
  S21_COMPARISON_TRUE = 1,
} s21_comparison_result;

int s21_from_int_to_decimal(int src, s21_decimal *dst);
int s21_from_float_to_decimal(float src, s21_decimal *dst);
int s21_from_decimal_to_int(s21_decimal src, int *dst);
int s21_from_decimal_to_float(s21_decimal src, float *dst);

typedef enum s21_conversion_result {
  S21_CONVERSION_OK = 0,
  S21_CONVERSION_ERROR = 1
} s21_conversion_result;

#define MAX_FLOAT_TO_CONVERT 79228157791897854723898736640.0f
#define MIN_FLOAT_TO_CONVERT \
  0.00000000000000000000000000010000000031710768509710513471352647538147514756461109f

s21_decimal s21_decimal_get_from_char(char c);
int s21_get_float_exp_from_string(char *str);
s21_decimal s21_float_string_to_decimal(char *str);

int s21_is_correct_decimal(s21_decimal decimal);
int s21_decimal_get_sign(s21_decimal decimal);
int s21_decimal_get_power(s21_decimal decimal);
int s21_decimal_get_empty1(s21_decimal decimal);
int s21_decimal_get_empty2(s21_decimal decimal);

void s21_decimal_set_sign(s21_decimal *decimal, int sign);
void s21_decimal_set_power(s21_decimal *decimal, int power);
int s21_decimal_even(s21_decimal value);

s21_decimal s21_create_decimal_from_array(int data1, int data2, int data3,
                                          int data4);
s21_decimal s21_create_decimal_from_data(int sign, int power, int data1,
                                         int data2, int data3);
s21_decimal s21_create_decimal_from_strings(char *str1, char *str2, char *str3,
                                            char *str4);
int s21_decimal_set_bits_from_string(int *bits, char *str);

void s21_clear_decimal(s21_decimal *decimal);
void s21_decimal_null_service_bits(s21_decimal *value);
s21_decimal s21_decimal_get_zero(void);
s21_decimal s21_decimal_get_one(void);
s21_decimal s21_decimal_get_ten(void);
s21_decimal s21_int128_get_ten_pow(int pow);
s21_decimal s21_decimal_get_zerofive(void);
s21_decimal s21_decimal_get_min(void);
s21_decimal s21_decimal_get_max(void);
s21_decimal s21_decimal_get_int_max(void);
s21_decimal s21_decimal_get_int_min(void);
s21_decimal s21_decimal_get_inf(void);

s21_int256 s21_create_int256_from_decimal(s21_decimal value_1);

static const s21_decimal all_ten_pows[39] = {
    [0] = {{0x1, 0x0, 0x0, 0x0}},
    [1] = {{0xA, 0x0, 0x0, 0x0}},
    [2] = {{0x64, 0x0, 0x0, 0x0}},
    [3] = {{0x3E8, 0x0, 0x0, 0x0}},
    [4] = {{0x2710, 0x0, 0x0, 0x0}},
    [5] = {{0x186A0, 0x0, 0x0, 0x0}},
    [6] = {{0xF4240, 0x0, 0x0, 0x0}},
    [7] = {{0x989680, 0x0, 0x0, 0x0}},
    [8] = {{0x5F5E100, 0x0, 0x0, 0x0}},
    [9] = {{0x3B9ACA00, 0x0, 0x0, 0x0}},
    [10] = {{0x540BE400, 0x2, 0x0, 0x0}},
    [11] = {{0x4876E800, 0x17, 0x0, 0x0}},
    [12] = {{0xD4A51000, 0xE8, 0x0, 0x0}},
    [13] = {{0x4E72A000, 0x918, 0x0, 0x0}},
    [14] = {{0x107A4000, 0x5AF3, 0x0, 0x0}},
    [15] = {{0xA4C68000, 0x38D7E, 0x0, 0x0}},
    [16] = {{0x6FC10000, 0x2386F2, 0x0, 0x0}},
    [17] = {{0x5D8A0000, 0x1634578, 0x0, 0x0}},
    [18] = {{0xA7640000, 0xDE0B6B3, 0x0, 0x0}},
    [19] = {{0x89E80000, 0x8AC72304, 0x0, 0x0}},
    [20] = {{0x63100000, 0x6BC75E2D, 0x5, 0x0}},
    [21] = {{0xDEA00000, 0x35C9ADC5, 0x36, 0x0}},
    [22] = {{0xB2400000, 0x19E0C9BA, 0x21E, 0x0}},
    [23] = {{0xF6800000, 0x2C7E14A, 0x152D, 0x0}},
    [24] = {{0xA1000000, 0x1BCECCED, 0xD3C2, 0x0}},
    [25] = {{0x4A000000, 0x16140148, 0x84595, 0x0}},
    [26] = {{0xE4000000, 0xDCC80CD2, 0x52B7D2, 0x0}},
    [27] = {{0xE8000000, 0x9FD0803C, 0x33B2E3C, 0x0}},
    [28] = {{0x10000000, 0x3E250261, 0x204FCE5E, 0x0}},
    [29] = {{0xA0000000, 0x6D7217CA, 0x431E0FAE, 0x1}},
    [30] = {{0x40000000, 0x4674EDEA, 0x9F2C9CD0, 0xC}},
    [31] = {{0x80000000, 0xC0914B26, 0x37BE2022, 0x7E}},
    [32] = {{0x0, 0x85ACEF81, 0x2D6D415B, 0x4EE}},
    [33] = {{0x0, 0x38C15B0A, 0xC6448D93, 0x314D}},
    [34] = {{0x0, 0x378D8E64, 0xBEAD87C0, 0x1ED09}},
    [35] = {{0x0, 0x2B878FE8, 0x72C74D82, 0x134261}},
    [36] = {{0x0, 0xB34B9F10, 0x7BC90715, 0xC097CE}},
    [37] = {{0x0, 0xF436A0, 0xD5DA46D9, 0x785EE10}},
    [38] = {{0x0, 0x98A2240, 0x5A86C47A, 0x4B3B4CA8}}};

int s21_floor(s21_decimal value, s21_decimal *result);
int s21_round(s21_decimal value, s21_decimal *result);
int s21_truncate(s21_decimal value, s21_decimal *result);
int s21_negate(s21_decimal value, s21_decimal *result);

s21_decimal s21_remove_trailing_zeros(s21_decimal value);
s21_decimal s21_round_banking(s21_decimal integral, s21_decimal fractional);

typedef enum s21_other_result {
  S21_OTHER_OK = 0,
  S21_OTHER_ERROR = 1
} s21_other_result;

#define TEST_ARITHMETIC_OK 0
#define TEST_ARITHMETIC_BIG 1
#define TEST_ARITHMETIC_SMALL 2
#define TEST_ARITHMETIC_ZERO_DIV 3
#define TEST_COMPARISON_FALSE 0
#define TEST_COMPARISON_TRUE 1
#define TEST_CONVERSION_OK 0
#define TEST_CONVERSION_ERROR 1
#define TEST_OTHER_OK 0
#define TEST_OTHER_ERROR 1

typedef union float_cast_test {
  float f;
  struct {
    uint32_t mantisa : 23;
    uint32_t exponent : 8;
    uint32_t sign : 1;
  } parts;
  uint32_t bytes;
  int int32_bytes;
} float_cast_test;

typedef union test_decimal_bytes {
  struct s21_decimal d;
  struct {
    uint32_t part0;
    uint32_t part1;
    uint32_t part2;
    uint32_t empty2 : 16;
    uint32_t power : 8;
    uint32_t empty1 : 7;
    uint32_t sign : 1;
  } parts;
} test_decimal_bytes;

#endif