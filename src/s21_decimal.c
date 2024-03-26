#include "s21_decimal.h"

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int s21_int256_get_shift_to_decimal(s21_int256 value) {
  int cnt = 0;
  if (!(s21_int128_binary_equal_zero(value.decimals[0]) &&
        s21_int128_binary_equal_zero(value.decimals[1]))) {
    s21_int256 max = s21_create_int256_from_decimal(s21_decimal_get_max());
    s21_int256 quotient = s21_int256_binary_division(value, max, NULL);
    while (1) {
      int compare = s21_int128_binary_compare(quotient.decimals[0],
                                              s21_int128_get_ten_pow(cnt));
      if (compare == -1 || compare == 0) {
        break;
      }
      ++cnt;
    }
    s21_int256 tmp = s21_int256_binary_division(
        value, s21_create_int256_from_decimal(s21_int128_get_ten_pow(cnt)),
        NULL);
    if (!s21_int128_binary_equal_zero(tmp.decimals[1]) ||
        tmp.decimals[0].bits[3] != 0) {
      ++cnt;
    }
  }
  return cnt;
}

int s21_max(int value_1, int value_2) {
  int result = value_2;
  if (value_1 > value_2) {
    result = value_1;
  }

  return result;
}

void s21_decimal_leveling(s21_decimal value_1, s21_decimal value_2,
                          s21_int256 *value_1l, s21_int256 *value_2l) {
  int power1 = s21_decimal_get_power(value_1);
  int power2 = s21_decimal_get_power(value_2);
  s21_decimal tmp1 = value_1;
  s21_decimal tmp2 = value_2;

  s21_decimal_null_service_bits(&tmp1);
  s21_decimal_null_service_bits(&tmp2);

  if (power1 > power2) {
    *value_1l = s21_create_int256_from_decimal(tmp1);
    *value_2l = s21_int128_binary_multiplication(
        tmp2, s21_int128_get_ten_pow(power1 - power2));
  } else if (power1 < power2) {
    *value_1l = s21_int128_binary_multiplication(
        tmp1, s21_int128_get_ten_pow(power2 - power1));
    *value_2l = s21_create_int256_from_decimal(tmp2);
  } else {
    *value_1l = s21_create_int256_from_decimal(tmp1);
    *value_2l = s21_create_int256_from_decimal(tmp2);
  }
}

s21_decimal s21_abs(s21_decimal value_1) {
  s21_decimal result = value_1;
  s21_decimal_set_sign(&result, S21_POSITIVE);
  return result;
}

int s21_add(s21_decimal value_1, s21_decimal value_2, s21_decimal *result) {
  s21_arithmetic_result code = S21_ARITHMETIC_OK;
  if (!result) {
    code = S21_ARITHMETIC_ERROR;
  } else if (!s21_is_correct_decimal(value_1) ||
             !s21_is_correct_decimal(value_2)) {
    code = S21_ARITHMETIC_ERROR;
    *result = s21_decimal_get_inf();
  } else {
    *result = s21_decimal_get_zero();
    s21_decimal res = s21_decimal_get_zero();

    int sign1 = s21_decimal_get_sign(value_1);
    int sign2 = s21_decimal_get_sign(value_2);

    if (sign1 == S21_POSITIVE && sign2 == S21_POSITIVE) {
      code = s21_add_handle(value_1, value_2, &res);
    } else if (sign1 == S21_POSITIVE && sign2 == S21_NEGATIVE) {
      code = s21_sub(value_1, s21_abs(value_2), &res);
    } else if (sign1 == S21_NEGATIVE && sign2 == S21_POSITIVE) {
      code = s21_sub(s21_abs(value_1), value_2, &res);
      s21_negate(res, &res);
    } else if (sign1 == S21_NEGATIVE && sign2 == S21_NEGATIVE) {
      code = s21_add_handle(s21_abs(value_1), s21_abs(value_2), &res);
      s21_negate(res, &res);
    }

    if (s21_decimal_get_sign(res) == S21_NEGATIVE &&
        code == S21_ARITHMETIC_BIG) {
      code = S21_ARITHMETIC_SMALL;
    }

    *result = res;
  }

  return code;
}

int s21_add_handle(s21_decimal value_1, s21_decimal value_2,
                   s21_decimal *result) {
  s21_arithmetic_result code = S21_ARITHMETIC_OK;
  s21_int256 value_1l;
  s21_int256 value_2l;
  int power1 = s21_decimal_get_power(value_1);
  int power2 = s21_decimal_get_power(value_2);
  int max_power = s21_max(power1, power2);

  s21_decimal_leveling(value_1, value_2, &value_1l, &value_2l);
  s21_int256 res = s21_int256_binary_addition(value_1l, value_2l);
  int shift = s21_int256_get_shift_to_decimal(res);
  int res_power = max_power - shift;

  if (res_power < 0) {
    code = S21_ARITHMETIC_BIG;
    *result = s21_decimal_get_inf();
  } else {
    while (shift > 28) {
      res = s21_int256_binary_division(
          res, s21_create_int256_from_decimal(s21_decimal_get_ten()), NULL);
      --shift;
    }

    s21_int256 remainder =
        s21_create_int256_from_decimal(s21_decimal_get_zero());
    s21_int256 powerten =
        s21_create_int256_from_decimal(s21_int128_get_ten_pow(shift));

    res = s21_int256_binary_division(res, powerten, &remainder);
    s21_decimal_set_power(&remainder.decimals[0], shift);
    res.decimals[0] = s21_round_banking(res.decimals[0], remainder.decimals[0]);
    s21_decimal_set_power(&res.decimals[0], res_power);

    if (!s21_int128_binary_equal_zero(res.decimals[1]) ||
        !s21_is_correct_decimal(res.decimals[0])) {
      code = S21_ARITHMETIC_BIG;
      *result = s21_decimal_get_inf();
    } else {
      *result = res.decimals[0];
    }
  }

  return code;
}

/**
 * @brief Делит value_1 на value_2 и записывает результат в result
 *
 * @author Hubert Furr (hubertfu@student.21-school.ru)
 * @param value_1 делимое
 * @param value_2 делитель
 * @param result частное
 * @return int код ошибки:
 * 0 - OK
 * 1 - число слишком велико или равно бесконечности
 * 2 - число слишком мало или равно отрицательной бесконечности
 * 3 - деление на 0
 * 4 - некорректные входные данные
 */
int s21_div(s21_decimal value_1, s21_decimal value_2, s21_decimal *result) {
  s21_arithmetic_result code = S21_ARITHMETIC_OK;

  if (!result) {
    // Если указатель на decimal является NULL
    code = S21_ARITHMETIC_ERROR;
  } else if (!s21_is_correct_decimal(value_1) ||
             !s21_is_correct_decimal(value_2)) {
    // Проверяем, что value_1 и value_2 являются корректными decimal
    code = S21_ARITHMETIC_ERROR;
    *result = s21_decimal_get_inf();
  } else if (s21_is_equal(value_2, s21_decimal_get_zero())) {
    // Проверяем случай, когда value_2 является нулем
    code = S21_ARITHMETIC_ZERO_DIV;
    *result = s21_decimal_get_inf();
  } else {
    // В остальных случаях считаем частное
    *result = s21_decimal_get_zero();
    int sign1 = s21_decimal_get_sign(value_1);
    int sign2 = s21_decimal_get_sign(value_2);
    s21_int256 value_1l;
    s21_int256 value_2l;
    // Выравниваем степени делимого и делителя (знак и степень при этом будут
    // обнулены)
    s21_decimal_leveling(value_1, value_2, &value_1l, &value_2l);

    s21_int256 remainder =
        s21_create_int256_from_decimal(s21_decimal_get_zero());
    s21_int256 res;

    // Делим выравненное делимое на выравненный делитель, остаток от деления
    // будет записан в remainder
    res = s21_int256_binary_division(value_1l, value_2l, &remainder);

    if (res.decimals[0].bits[3] != 0 ||
        !s21_int128_binary_equal_zero(res.decimals[1])) {
      // Если целый результат деления уже не помещается в decimal
      // (переполнение), то возвращаем соответствующие ошибки
      if (sign1 != sign2) {
        code = S21_ARITHMETIC_SMALL;
      } else {
        code = S21_ARITHMETIC_BIG;
      }
      *result = s21_decimal_get_inf();
    } else {
      // В остальных случаях продолжаем вычисления и вызываем вспомогательную
      // функцию для расчетов
      code = s21_div_handle(value_2l, res, remainder, result);
      // Если знаки делимого и делителя отличаются, то необходимо сделать
      // результат отрицательным
      if (sign1 != sign2) {
        s21_decimal_set_sign(result, S21_NEGATIVE);
      }
      // Корректируем код ответа от вспомогательной функции в случае ошибки и
      // отрицательного результата
      if (s21_decimal_get_sign(*result) == S21_NEGATIVE &&
          code == S21_ARITHMETIC_BIG) {
        code = S21_ARITHMETIC_SMALL;
      }
      // Обрабатываем ситуацию, что результат получился слишком маленький (0 <
      // |x| < 1e-28)
      if (code == S21_ARITHMETIC_OK &&
          s21_is_not_equal(value_1, s21_decimal_get_zero()) &&
          s21_is_equal(*result, s21_decimal_get_zero())) {
        code = S21_ARITHMETIC_SMALL;
      }
    }
  }

  return code;
}

int s21_div_handle(s21_int256 value_2l, s21_int256 res, s21_int256 remainder,
                   s21_decimal *result) {
  s21_arithmetic_result code = S21_ARITHMETIC_OK;

  int power1 = s21_div_calc_fractional(&res, value_2l, &remainder);

  s21_int256 tmp_res = s21_create_int256_from_decimal(s21_decimal_get_zero());
  int power2 = s21_div_calc_fractional(&tmp_res, value_2l, &remainder);

  s21_decimal_set_power(&tmp_res.decimals[0], power2);

  if (s21_is_equal(tmp_res.decimals[0], s21_decimal_get_zerofive())) {
    if (!s21_int128_binary_equal_zero(remainder.decimals[0]) ||
        !s21_int128_binary_equal_zero(remainder.decimals[1])) {
      s21_add(tmp_res.decimals[0], s21_decimal_get_min(), &tmp_res.decimals[0]);
    }
  }
  res.decimals[0] = s21_round_banking(res.decimals[0], tmp_res.decimals[0]);
  s21_decimal_set_power(&res.decimals[0], power1);
  if (!s21_int128_binary_equal_zero(res.decimals[1]) ||
      !s21_is_correct_decimal(res.decimals[0])) {
    code = S21_ARITHMETIC_BIG;
    *result = s21_decimal_get_inf();
  } else {
    *result = res.decimals[0];
  }

  return code;
}

int s21_div_calc_fractional(s21_int256 *res, s21_int256 value_2l,
                            s21_int256 *remainder) {
  int power = 0;
  s21_int256 number = *res;

  s21_int256 tmp;
  s21_int256 tmp_remainder = *remainder;

  while ((!s21_int128_binary_equal_zero((*remainder).decimals[0]) ||
          !s21_int128_binary_equal_zero((*remainder).decimals[1])) &&
         power < 28) {
    s21_int256 number_stored = number;
    s21_int256 remainder_stored = tmp_remainder;

    number = s21_int256_binary_multiplication(number, s21_decimal_get_ten());
    tmp_remainder =
        s21_int256_binary_multiplication(tmp_remainder, s21_decimal_get_ten());
    tmp = s21_int256_binary_division(tmp_remainder, value_2l, &tmp_remainder);
    number = s21_int256_binary_addition(number, tmp);

    if (!s21_is_correct_decimal(number.decimals[0])) {
      number = number_stored;
      tmp_remainder = remainder_stored;
      break;
    }

    ++power;
  }

  *res = number;
  *remainder = tmp_remainder;

  return power;
}

int s21_mod(s21_decimal value_1, s21_decimal value_2, s21_decimal *result) {
  s21_arithmetic_result code = S21_ARITHMETIC_OK;

  if (!result) {
    code = S21_ARITHMETIC_ERROR;
  } else if (!s21_is_correct_decimal(value_1) ||
             !s21_is_correct_decimal(value_2)) {
    code = S21_ARITHMETIC_ERROR;
    *result = s21_decimal_get_inf();
  } else if (s21_is_equal(value_2, s21_decimal_get_zero())) {
    code = S21_ARITHMETIC_ZERO_DIV;
    *result = s21_decimal_get_inf();
  } else if (s21_is_less(s21_abs(value_1), s21_abs(value_2))) {
    code = S21_ARITHMETIC_OK;
    *result = value_1;
  } else {
    *result = s21_decimal_get_zero();

    int sign1 = s21_decimal_get_sign(value_1);
    int power1 = s21_decimal_get_power(value_1);
    int power2 = s21_decimal_get_power(value_2);
    int max_power = s21_max(power1, power2);
    s21_int256 value_1l;
    s21_int256 value_2l;
    s21_int256 remainder =
        s21_create_int256_from_decimal(s21_decimal_get_zero());

    s21_decimal_leveling(value_1, value_2, &value_1l, &value_2l);
    s21_int256_binary_division(value_1l, value_2l, &remainder);
    s21_decimal_set_power(&remainder.decimals[0], max_power);

    *result = remainder.decimals[0];
    s21_decimal_set_sign(result, sign1);
  }

  return code;
}

int s21_mul(s21_decimal value_1, s21_decimal value_2, s21_decimal *result) {
  s21_arithmetic_result code = S21_ARITHMETIC_OK;

  if (!result) {
    code = S21_ARITHMETIC_ERROR;
  } else if (!s21_is_correct_decimal(value_1) ||
             !s21_is_correct_decimal(value_2)) {
    code = S21_ARITHMETIC_ERROR;
    *result = s21_decimal_get_inf();
  } else {
    *result = s21_decimal_get_zero();
    s21_decimal res = s21_decimal_get_zero();

    int sign1 = s21_decimal_get_sign(value_1);
    int sign2 = s21_decimal_get_sign(value_2);

    if (sign1 == S21_POSITIVE && sign2 == S21_POSITIVE) {
      code = s21_mul_handle(value_1, value_2, &res);
    } else if (sign1 == S21_POSITIVE && sign2 == S21_NEGATIVE) {
      code = s21_mul_handle(value_1, s21_abs(value_2), &res);
      s21_negate(res, &res);
    } else if (sign1 == S21_NEGATIVE && sign2 == S21_POSITIVE) {
      code = s21_mul_handle(s21_abs(value_1), value_2, &res);
      s21_negate(res, &res);
    } else if (sign1 == S21_NEGATIVE && sign2 == S21_NEGATIVE) {
      code = s21_mul_handle(s21_abs(value_1), s21_abs(value_2), &res);
    }

    if (code == S21_ARITHMETIC_BIG) {
      if (s21_decimal_get_sign(res) == S21_NEGATIVE) {
        code = S21_ARITHMETIC_SMALL;
      }
    }

    if (code == S21_ARITHMETIC_OK &&
        s21_is_not_equal(value_1, s21_decimal_get_zero()) &&
        s21_is_not_equal(value_2, s21_decimal_get_zero()) &&
        s21_is_equal(res, s21_decimal_get_zero())) {
      code = S21_ARITHMETIC_SMALL;
    }

    *result = res;
  }

  return code;
}

int s21_mul_handle(s21_decimal value_1, s21_decimal value_2,
                   s21_decimal *result) {
  s21_arithmetic_result code = S21_ARITHMETIC_OK;
  int power1 = s21_decimal_get_power(value_1);
  int power2 = s21_decimal_get_power(value_2);
  s21_decimal_null_service_bits(&value_1);
  s21_decimal_null_service_bits(&value_2);
  s21_int256 res = s21_int128_binary_multiplication(value_1, value_2);

  int shift = s21_int256_get_shift_to_decimal(res);
  int res_power = power1 + power2 - shift;

  if (res_power < 0) {
    code = S21_ARITHMETIC_BIG;
    *result = s21_decimal_get_inf();
  } else {
    while (shift > 28) {
      res = s21_int256_binary_division(
          res, s21_create_int256_from_decimal(s21_decimal_get_ten()), NULL);
      --shift;
    }
    if (res_power > 28) {
      s21_int256 tmp = res;
      int tmp_power = res_power;
      while (tmp_power > 28) {
        tmp = s21_int256_binary_division(
            tmp, s21_create_int256_from_decimal(s21_decimal_get_ten()), NULL);
        --tmp_power;
      }
      shift = res_power - tmp_power + shift;
      res_power = tmp_power;
    }
    s21_int256 remainder =
        s21_create_int256_from_decimal(s21_decimal_get_zero());
    s21_int256 powerten =
        s21_create_int256_from_decimal(s21_int128_get_ten_pow(shift));
    res = s21_int256_binary_division(res, powerten, &remainder);
    s21_decimal_set_power(&remainder.decimals[0], shift);
    res.decimals[0] = s21_round_banking(res.decimals[0], remainder.decimals[0]);
    s21_decimal_set_power(&res.decimals[0], res_power);

    if (!s21_int128_binary_equal_zero(res.decimals[1]) ||
        !s21_is_correct_decimal(res.decimals[0])) {
      code = S21_ARITHMETIC_BIG;
      *result = s21_decimal_get_inf();
    } else {
      *result = res.decimals[0];
    }
  }

  return code;
}

int s21_sub(s21_decimal value_1, s21_decimal value_2, s21_decimal *result) {
  s21_arithmetic_result code = S21_ARITHMETIC_OK;
  if (!result) {
    code = S21_ARITHMETIC_ERROR;
  } else if (!s21_is_correct_decimal(value_1) ||
             !s21_is_correct_decimal(value_2)) {
    code = S21_ARITHMETIC_ERROR;
    *result = s21_decimal_get_inf();
  } else {
    *result = s21_decimal_get_zero();
    s21_decimal res = s21_decimal_get_zero();
    int sign1 = s21_decimal_get_sign(value_1);
    int sign2 = s21_decimal_get_sign(value_2);

    if (sign1 == S21_POSITIVE && sign2 == S21_POSITIVE) {
      if (s21_is_greater_or_equal(value_1, value_2)) {
        code = s21_sub_handle(value_1, value_2, &res);
      } else {
        code = s21_sub_handle(value_2, value_1, &res);
        s21_negate(res, &res);
      }
    } else if (sign1 == S21_POSITIVE && sign2 == S21_NEGATIVE) {
      code = s21_add(value_1, s21_abs(value_2), &res);
    } else if (sign1 == S21_NEGATIVE && sign2 == S21_POSITIVE) {
      code = s21_add(s21_abs(value_1), value_2, &res);
      s21_negate(res, &res);
    } else if (sign1 == S21_NEGATIVE && sign2 == S21_NEGATIVE) {
      if (s21_is_greater_or_equal(value_1, value_2)) {
        code = s21_sub_handle(s21_abs(value_2), s21_abs(value_1), &res);
      } else {
        code = s21_sub_handle(s21_abs(value_1), s21_abs(value_2), &res);
        s21_negate(res, &res);
      }
    }

    if (s21_decimal_get_sign(res) == S21_NEGATIVE &&
        code == S21_ARITHMETIC_BIG) {
      code = S21_ARITHMETIC_SMALL;
    }

    *result = res;
  }

  return code;
}

int s21_sub_handle(s21_decimal value_1, s21_decimal value_2,
                   s21_decimal *result) {
  s21_arithmetic_result code = S21_ARITHMETIC_OK;
  s21_int256 value_1l;
  s21_int256 value_2l;
  int power1 = s21_decimal_get_power(value_1);
  int power2 = s21_decimal_get_power(value_2);
  int max_power = s21_max(power1, power2);
  s21_decimal_leveling(value_1, value_2, &value_1l, &value_2l);
  s21_int256 res = s21_int256_binary_subtraction(value_1l, value_2l);
  int shift = s21_int256_get_shift_to_decimal(res);
  int res_power = max_power - shift;

  if (res_power < 0) {
    code = S21_ARITHMETIC_BIG;
    *result = s21_decimal_get_inf();
  } else {
    s21_int256 remainder =
        s21_create_int256_from_decimal(s21_decimal_get_zero());
    s21_int256 powerten =
        s21_create_int256_from_decimal(s21_int128_get_ten_pow(shift));

    res = s21_int256_binary_division(res, powerten, &remainder);
    s21_decimal_set_power(&remainder.decimals[0], shift);
    res.decimals[0] = s21_round_banking(res.decimals[0], remainder.decimals[0]);
    s21_decimal_set_power(&res.decimals[0], res_power);

    *result = res.decimals[0];
  }

  return code;
}

int s21_is_set_bit(int number, int index) { return !!(number & (1U << index)); }

int s21_set_bit(int number, int index) { return number | (1U << index); }

int s21_reset_bit(int number, int index) { return number & ~(1U << index); }

int s21_decimal_is_set_bit(s21_decimal decimal, int index) {
  return s21_is_set_bit(decimal.bits[index / MAX_BLOCK_BITS],
                        index % MAX_BLOCK_BITS);
}

s21_decimal s21_decimal_set_bit(s21_decimal decimal, int index) {
  decimal.bits[index / MAX_BLOCK_BITS] =
      s21_set_bit(decimal.bits[index / MAX_BLOCK_BITS], index % MAX_BLOCK_BITS);
  return decimal;
}

int s21_decimal_get_not_zero_bit(s21_decimal decimal) {
  int result = -1;
  for (int i = MAX_BITS - 1; i >= 0; i--) {
    if (s21_decimal_is_set_bit(decimal, i)) {
      result = i;
      break;
    }
  }

  return result;
}

s21_decimal s21_int128_binary_addition(s21_decimal decimal1,
                                       s21_decimal decimal2) {
  s21_decimal result = decimal1;
  s21_decimal tmp = decimal2;

  while (!s21_int128_binary_equal_zero(tmp)) {
    s21_decimal overflow_bits = s21_int128_binary_and(result, tmp);
    overflow_bits = s21_int128_binary_shift_left(overflow_bits, 1);
    result = s21_int128_binary_xor(result, tmp);
    tmp = overflow_bits;
  }

  return result;
}

s21_int256 s21_int256_binary_addition(s21_int256 decimal1,
                                      s21_int256 decimal2) {
  s21_int256 result = decimal1;
  s21_int256 tmp = decimal2;

  while (!s21_int128_binary_equal_zero(tmp.decimals[0]) ||
         !s21_int128_binary_equal_zero(tmp.decimals[1])) {
    s21_int256 overflow_bits;
    overflow_bits.decimals[0] =
        s21_int128_binary_and(result.decimals[0], tmp.decimals[0]);
    overflow_bits.decimals[1] =
        s21_int128_binary_and(result.decimals[1], tmp.decimals[1]);

    overflow_bits = s21_int256_binary_shift_left(overflow_bits, 1);
    result.decimals[0] =
        s21_int128_binary_xor(result.decimals[0], tmp.decimals[0]);
    result.decimals[1] =
        s21_int128_binary_xor(result.decimals[1], tmp.decimals[1]);

    tmp = overflow_bits;
  }

  return result;
}

int s21_int128_binary_equal_zero(s21_decimal decimal) {
  return decimal.bits[0] == 0 && decimal.bits[1] == 0 && decimal.bits[2] == 0 &&
         decimal.bits[3] == 0;
}

int s21_int128_binary_compare(s21_decimal d1, s21_decimal d2) {
  int result = 0;

  for (int i = MAX_BITS - 1; i >= 0; i--) {
    int b1 = s21_decimal_is_set_bit(d1, i);
    int b2 = s21_decimal_is_set_bit(d2, i);

    if (b1 == 0 && b2 != 0) {
      result = -1;
    }

    if (b1 != 0 && b2 == 0) {
      result = 1;
    }

    if (result != 0) {
      break;
    }
  }

  return result;
}

int s21_int256_binary_compare(s21_int256 d1, s21_int256 d2) {
  int compare = s21_int128_binary_compare(d1.decimals[1], d2.decimals[1]);

  if (compare == 0) {
    compare = s21_int128_binary_compare(d1.decimals[0], d2.decimals[0]);
  }

  return compare;
}

s21_decimal s21_int128_binary_division(s21_decimal decimal1,
                                       s21_decimal decimal2,
                                       s21_decimal *remainder) {
  s21_decimal result;

  s21_decimal partial_remainder = s21_decimal_get_zero();
  s21_decimal quotient = s21_decimal_get_zero();

  if (s21_int128_binary_equal_zero(decimal1)) {
    quotient = s21_decimal_get_zero();
    partial_remainder = s21_decimal_get_zero();
  } else if (s21_int128_binary_compare(decimal1, decimal2) == -1) {
    quotient = s21_decimal_get_zero();
    partial_remainder = decimal1;
  } else {
    int left1 = s21_decimal_get_not_zero_bit(decimal1);
    int left2 = s21_decimal_get_not_zero_bit(decimal2);
    int shift = left1 - left2;

    s21_decimal shifted_divisor = s21_int128_binary_shift_left(decimal2, shift);
    s21_decimal dividend = decimal1;

    int need_subtraction = 1;

    while (shift >= 0) {
      if (need_subtraction == 1) {
        partial_remainder =
            s21_int128_binary_subtraction(dividend, shifted_divisor);
      } else {
        partial_remainder =
            s21_int128_binary_addition(dividend, shifted_divisor);
      }

      quotient = s21_int128_binary_shift_left(quotient, 1);
      if (s21_decimal_is_set_bit(partial_remainder, MAX_BITS - 1) == 0) {
        quotient = s21_decimal_set_bit(quotient, 0);
      }

      dividend = s21_int128_binary_shift_left(partial_remainder, 1);

      if (s21_decimal_is_set_bit(partial_remainder, MAX_BITS - 1) == 0) {
        need_subtraction = 1;
      } else {
        need_subtraction = 0;
      }
      --shift;
    }
    if (s21_decimal_is_set_bit(partial_remainder, MAX_BITS - 1)) {
      partial_remainder =
          s21_int128_binary_addition(partial_remainder, shifted_divisor);
    }
    partial_remainder =
        s21_int128_binary_shift_right(partial_remainder, left1 - left2);
  }

  result = quotient;
  if (remainder != NULL) {
    *remainder = partial_remainder;
  }

  return result;
}

s21_int256 s21_int256_binary_division(s21_int256 decimal1, s21_int256 decimal2,
                                      s21_int256 *remainder) {
  s21_int256 result;

  s21_int256 partial_remainder =
      s21_create_int256_from_decimal(s21_decimal_get_zero());
  s21_int256 quotient = s21_create_int256_from_decimal(s21_decimal_get_zero());

  if (s21_int128_binary_equal_zero(decimal1.decimals[0]) &&
      s21_int128_binary_equal_zero(decimal1.decimals[1])) {
    quotient = s21_create_int256_from_decimal(s21_decimal_get_zero());
    partial_remainder = s21_create_int256_from_decimal(s21_decimal_get_zero());
  } else if (s21_int256_binary_compare(decimal1, decimal2) == -1) {
    quotient = s21_create_int256_from_decimal(s21_decimal_get_zero());
    partial_remainder = decimal1;
  } else {
    int left1 = s21_decimal_get_not_zero_bit(decimal1.decimals[1]);
    if (left1 == -1) {
      left1 = s21_decimal_get_not_zero_bit(decimal1.decimals[0]);
    } else {
      left1 = MAX_BITS + left1;
    }

    int left2 = s21_decimal_get_not_zero_bit(decimal2.decimals[1]);
    if (left2 == -1) {
      left2 = s21_decimal_get_not_zero_bit(decimal2.decimals[0]);
    } else {
      left2 = MAX_BITS + left2;
    }

    int shift = left1 - left2;

    s21_int256 shifted_divisor = s21_int256_binary_shift_left(decimal2, shift);
    s21_int256 dividend = decimal1;

    int need_subtraction = 1;

    while (shift >= 0) {
      if (need_subtraction == 1) {
        partial_remainder =
            s21_int256_binary_subtraction(dividend, shifted_divisor);
      } else {
        partial_remainder =
            s21_int256_binary_addition(dividend, shifted_divisor);
      }

      quotient = s21_int256_binary_shift_left(quotient, 1);
      if (s21_decimal_is_set_bit(partial_remainder.decimals[1], MAX_BITS - 1) ==
          0) {
        quotient.decimals[0] = s21_decimal_set_bit(quotient.decimals[0], 0);
      }

      dividend = s21_int256_binary_shift_left(partial_remainder, 1);

      if (s21_decimal_is_set_bit(partial_remainder.decimals[1], MAX_BITS - 1) ==
          0) {
        need_subtraction = 1;
      } else {
        need_subtraction = 0;
      }
      --shift;
    }
    if (s21_decimal_is_set_bit(partial_remainder.decimals[1], MAX_BITS - 1)) {
      partial_remainder =
          s21_int256_binary_addition(partial_remainder, shifted_divisor);
    }
    partial_remainder =
        s21_int256_binary_shift_right(partial_remainder, left1 - left2);
  }

  result = quotient;
  if (remainder != NULL) {
    *remainder = partial_remainder;
  }

  return result;
}

s21_decimal s21_int128_binary_and(s21_decimal decimal1, s21_decimal decimal2) {
  s21_decimal result = s21_decimal_get_zero();
  result.bits[0] = decimal1.bits[0] & decimal2.bits[0];
  result.bits[1] = decimal1.bits[1] & decimal2.bits[1];
  result.bits[2] = decimal1.bits[2] & decimal2.bits[2];
  result.bits[3] = decimal1.bits[3] & decimal2.bits[3];

  return result;
}

s21_decimal s21_int128_binary_xor(s21_decimal decimal1, s21_decimal decimal2) {
  s21_decimal result = s21_decimal_get_zero();
  result.bits[0] = decimal1.bits[0] ^ decimal2.bits[0];
  result.bits[1] = decimal1.bits[1] ^ decimal2.bits[1];
  result.bits[2] = decimal1.bits[2] ^ decimal2.bits[2];
  result.bits[3] = decimal1.bits[3] ^ decimal2.bits[3];

  return result;
}

s21_decimal s21_int128_binary_not(s21_decimal decimal) {
  s21_decimal result = s21_decimal_get_zero();
  result.bits[0] = ~decimal.bits[0];
  result.bits[1] = ~decimal.bits[1];
  result.bits[2] = ~decimal.bits[2];
  result.bits[3] = ~decimal.bits[3];

  return result;
}

s21_int256 s21_int128_binary_multiplication(s21_decimal decimal1,
                                            s21_decimal decimal2) {
  s21_int256 int256_result =
      s21_create_int256_from_decimal(s21_decimal_get_zero());
  s21_int256 int256_tmp1 = s21_create_int256_from_decimal(decimal1);

  int max_bit = s21_decimal_get_not_zero_bit(decimal2);

  for (int i = 0; i <= max_bit; i++) {
    if (s21_decimal_is_set_bit(decimal2, i) != 0) {
      int256_result = s21_int256_binary_addition(int256_result, int256_tmp1);
    }
    int256_tmp1 = s21_int256_binary_shift_left(int256_tmp1, 1);
  }

  return int256_result;
}

s21_int256 s21_int256_binary_multiplication(s21_int256 decimal1,
                                            s21_decimal decimal2) {
  s21_int256 int256_result =
      s21_create_int256_from_decimal(s21_decimal_get_zero());
  s21_int256 int256_tmp1 = decimal1;

  int max_bit = s21_decimal_get_not_zero_bit(decimal2);

  for (int i = 0; i <= max_bit; i++) {
    if (s21_decimal_is_set_bit(decimal2, i) != 0) {
      int256_result = s21_int256_binary_addition(int256_result, int256_tmp1);
    }

    int256_tmp1 = s21_int256_binary_shift_left(int256_tmp1, 1);
  }

  return int256_result;
}

s21_decimal s21_int128_binary_shift_left(s21_decimal decimal, int shift) {
  s21_decimal result = decimal;
  while (shift > 0) {
    result = s21_int128_binary_shift_left_one(result);
    --shift;
  }

  return result;
}

s21_decimal s21_int128_binary_shift_right(s21_decimal decimal, int shift) {
  s21_decimal result = decimal;
  while (shift > 0) {
    result = s21_int128_binary_shift_right_one(result);
    --shift;
  }

  return result;
}

s21_int256 s21_int256_binary_shift_left(s21_int256 decimal, int shift) {
  s21_int256 result = decimal;
  while (shift > 0) {
    int b0 = s21_decimal_is_set_bit(result.decimals[0], MAX_BITS - 1);
    result.decimals[0] = s21_int128_binary_shift_left_one(result.decimals[0]);
    result.decimals[1] = s21_int128_binary_shift_left_one(result.decimals[1]);
    if (b0) {
      result.decimals[1] = s21_decimal_set_bit(result.decimals[1], 0);
    }
    --shift;
  }

  return result;
}

s21_int256 s21_int256_binary_shift_right(s21_int256 decimal, int shift) {
  s21_int256 result = decimal;
  while (shift > 0) {
    int b1 = s21_decimal_is_set_bit(result.decimals[1], 0);
    result.decimals[0] = s21_int128_binary_shift_right_one(result.decimals[0]);
    result.decimals[1] = s21_int128_binary_shift_right_one(result.decimals[1]);
    if (b1) {
      result.decimals[0] =
          s21_decimal_set_bit(result.decimals[0], MAX_BITS - 1);
    }
    --shift;
  }

  return result;
}

s21_decimal s21_int128_binary_shift_left_one(s21_decimal decimal) {
  s21_decimal result = s21_decimal_get_zero();

  int b0 = s21_is_set_bit(decimal.bits[0], MAX_BLOCK_BITS - 1);
  unsigned int result0 = decimal.bits[0];
  result0 = result0 << 1;
  result.bits[0] = result0;

  int b1 = s21_is_set_bit(decimal.bits[1], MAX_BLOCK_BITS - 1);
  unsigned int result1 = decimal.bits[1];
  result1 = result1 << 1;
  result.bits[1] = result1;

  int b2 = s21_is_set_bit(decimal.bits[2], MAX_BLOCK_BITS - 1);
  unsigned int result2 = decimal.bits[2];
  result2 = result2 << 1;
  result.bits[2] = result2;

  unsigned int result3 = decimal.bits[3];
  result3 = result3 << 1;
  result.bits[3] = result3;

  if (b0) {
    result.bits[1] = s21_set_bit(result.bits[1], 0);
  }

  if (b1) {
    result.bits[2] = s21_set_bit(result.bits[2], 0);
  }

  if (b2) {
    result.bits[3] = s21_set_bit(result.bits[3], 0);
  }

  return result;
}

s21_decimal s21_int128_binary_shift_right_one(s21_decimal decimal) {
  s21_decimal result = s21_decimal_get_zero();

  int b3 = s21_is_set_bit(decimal.bits[3], 0);
  unsigned int result3 = decimal.bits[3];
  result3 = result3 >> 1;
  result.bits[3] = result3;

  int b2 = s21_is_set_bit(decimal.bits[2], 0);
  unsigned int result2 = decimal.bits[2];
  result2 = result2 >> 1;
  result.bits[2] = result2;

  int b1 = s21_is_set_bit(decimal.bits[1], 0);
  unsigned int result1 = decimal.bits[1];
  result1 = result1 >> 1;
  result.bits[1] = result1;

  unsigned int result0 = decimal.bits[0];
  result0 = result0 >> 1;
  result.bits[0] = result0;

  if (b3) {
    result.bits[2] = s21_set_bit(result.bits[2], MAX_BLOCK_BITS - 1);
  }

  if (b2) {
    result.bits[1] = s21_set_bit(result.bits[1], MAX_BLOCK_BITS - 1);
  }

  if (b1) {
    result.bits[0] = s21_set_bit(result.bits[0], MAX_BLOCK_BITS - 1);
  }

  return result;
}

s21_decimal s21_int128_binary_subtraction(s21_decimal decimal1,
                                          s21_decimal decimal2) {
  s21_decimal result;
  decimal2 = s21_int128_binary_not(decimal2);
  decimal2 = s21_int128_binary_addition(decimal2, s21_decimal_get_one());
  result = s21_int128_binary_addition(decimal1, decimal2);

  return result;
}

s21_int256 s21_int256_binary_subtraction(s21_int256 decimal1,
                                         s21_int256 decimal2) {
  s21_int256 result;
  decimal2.decimals[0] = s21_int128_binary_not(decimal2.decimals[0]);
  decimal2.decimals[1] = s21_int128_binary_not(decimal2.decimals[1]);

  s21_int256 one = s21_create_int256_from_decimal(s21_decimal_get_one());

  decimal2 = s21_int256_binary_addition(decimal2, one);
  result = s21_int256_binary_addition(decimal1, decimal2);

  return result;
}

int s21_is_equal(s21_decimal value_1, s21_decimal value_2) {
  s21_comparison_result code = S21_COMPARISON_TRUE;

  s21_decimal tmp1 = s21_remove_trailing_zeros(value_1);
  s21_decimal tmp2 = s21_remove_trailing_zeros(value_2);

  if (tmp1.bits[0] == 0 && tmp1.bits[1] == 0 && tmp1.bits[2] == 0 &&
      tmp2.bits[0] == 0 && tmp2.bits[1] == 0 && tmp2.bits[2] == 0) {
    code = S21_COMPARISON_TRUE;
  } else {
    code = tmp1.bits[0] == tmp2.bits[0] && tmp1.bits[1] == tmp2.bits[1] &&
           tmp1.bits[2] == tmp2.bits[2] && tmp1.bits[3] == tmp2.bits[3];
  }

  return code;
}

int s21_is_greater(s21_decimal value_1, s21_decimal value_2) {
  return s21_is_less(value_2, value_1);
}

int s21_is_greater_or_equal(s21_decimal value_1, s21_decimal value_2) {
  return s21_is_greater(value_1, value_2) || s21_is_equal(value_1, value_2);
}

int s21_is_less(s21_decimal value_1, s21_decimal value_2) {
  s21_comparison_result code = S21_COMPARISON_FALSE;

  int sign1 = s21_decimal_get_sign(value_1);
  int sign2 = s21_decimal_get_sign(value_2);

  if (s21_is_equal(value_1, s21_decimal_get_zero()) &&
      s21_is_equal(value_2, s21_decimal_get_zero())) {
    code = S21_COMPARISON_FALSE;
  } else if (sign1 == S21_NEGATIVE && sign2 == S21_POSITIVE) {
    code = S21_COMPARISON_TRUE;
  } else if (sign1 == S21_POSITIVE && sign2 == S21_NEGATIVE) {
    code = S21_COMPARISON_FALSE;
  } else if (sign1 == S21_NEGATIVE && sign2 == S21_NEGATIVE) {
    code = s21_is_less_handle(s21_abs(value_2), s21_abs(value_1));
  } else {
    code = s21_is_less_handle(value_1, value_2);
  }

  return code;
}

int s21_is_less_handle(s21_decimal value_1, s21_decimal value_2) {
  s21_comparison_result code = S21_COMPARISON_FALSE;
  s21_decimal tmp1 = value_1;
  s21_decimal tmp2 = value_2;
  s21_int256 tmp1l;
  s21_int256 tmp2l;

  s21_decimal_leveling(tmp1, tmp2, &tmp1l, &tmp2l);

  int compare = s21_int256_binary_compare(tmp1l, tmp2l);

  if (compare == -1) {
    code = S21_COMPARISON_TRUE;
  } else {
    code = S21_COMPARISON_FALSE;
  }

  return code;
}

int s21_is_less_or_equal(s21_decimal value_1, s21_decimal value_2) {
  return s21_is_less(value_1, value_2) || s21_is_equal(value_1, value_2);
}

int s21_is_not_equal(s21_decimal value_1, s21_decimal value_2) {
  return !s21_is_equal(value_1, value_2);
}

s21_decimal s21_decimal_get_from_char(char c) {
  s21_decimal result;

  switch (c) {
    case '0':
      result = s21_decimal_get_zero();
      break;
    case '1':
      result = s21_decimal_get_one();
      break;
    case '2':
      s21_from_int_to_decimal(2, &result);
      break;
    case '3':
      s21_from_int_to_decimal(3, &result);
      break;
    case '4':
      s21_from_int_to_decimal(4, &result);
      break;
    case '5':
      s21_from_int_to_decimal(5, &result);
      break;
    case '6':
      s21_from_int_to_decimal(6, &result);
      break;
    case '7':
      s21_from_int_to_decimal(7, &result);
      break;
    case '8':
      s21_from_int_to_decimal(8, &result);
      break;
    case '9':
      s21_from_int_to_decimal(9, &result);
      break;
  }

  return result;
}

int s21_get_float_exp_from_string(char *str) {
  int result = 0;
  char *ptr = str;
  while (*ptr) {
    if (*ptr == 'E') {
      ++ptr;
      result = strtol(ptr, NULL, 10);
      break;
    }
    ++ptr;
  }

  return result;
}

s21_decimal s21_float_string_to_decimal(char *str) {
  int digits_counter = 6;
  s21_decimal result = s21_decimal_get_zero();
  char *ptr = str;

  int exp = s21_get_float_exp_from_string(str);

  while (*ptr) {
    if (*ptr == '.') {
      ++ptr;
      continue;
    } else if (*ptr >= '0' && *ptr <= '9') {
      s21_decimal tmp = s21_decimal_get_zero();
      s21_mul(s21_decimal_get_from_char(*ptr),
              s21_int128_get_ten_pow(digits_counter), &tmp);
      s21_add(result, tmp, &result);
      --digits_counter;
      ++ptr;
    } else {
      break;
    }
  }
  exp = exp - 6;

  if (exp > 0) {
    s21_mul(result, s21_int128_get_ten_pow(exp), &result);
  } else if (exp < 0) {
    if (exp < -28) {
      s21_div(result, s21_int128_get_ten_pow(28), &result);
      exp += 28;
    }
    s21_div(result, s21_int128_get_ten_pow(-exp), &result);
  }
  return result;
}

int s21_from_decimal_to_float(s21_decimal src, float *dst) {
  s21_conversion_result code = S21_CONVERSION_OK;
  if (!dst) {
    code = S21_CONVERSION_ERROR;
  } else if (!s21_is_correct_decimal(src)) {
    code = S21_CONVERSION_ERROR;
    *dst = 0.0;
  } else if (s21_is_equal(src, s21_decimal_get_zero())) {
    int sign = s21_decimal_get_sign(src);
    if (sign == S21_NEGATIVE) {
      *dst = -0.0;
    } else {
      *dst = 0.0;
    }
    code = S21_CONVERSION_OK;
  } else {
    *dst = 0.0;
    double tmp = 0.0;
    int sign = s21_decimal_get_sign(src);
    int power = s21_decimal_get_power(src);

    for (int i = 0; i < MAX_BLOCK_NUMBER; i++) {
      if (s21_decimal_is_set_bit(src, i) != 0) {
        tmp += pow(2.0, i);
      }
    }

    double powerten = pow(10, power);
    tmp /= powerten;

    if (sign == S21_NEGATIVE) {
      tmp *= -1.0;
    }

    *dst = (float)tmp;
  }

  return code;
}

int s21_from_decimal_to_int(s21_decimal src, int *dst) {
  s21_conversion_result code = S21_CONVERSION_OK;

  if (!dst) {
    code = S21_CONVERSION_ERROR;
  } else if (!s21_is_correct_decimal(src)) {
    code = S21_CONVERSION_ERROR;
    *dst = 0;
  } else {
    *dst = 0;
    s21_decimal truncated_decimal = s21_decimal_get_zero();
    s21_truncate(src, &truncated_decimal);

    if (s21_is_less(truncated_decimal, s21_decimal_get_int_min()) ==
        S21_COMPARISON_TRUE) {
      code = S21_CONVERSION_ERROR;
    } else if (s21_is_greater(truncated_decimal, s21_decimal_get_int_max()) ==
               S21_COMPARISON_TRUE) {
      code = S21_CONVERSION_ERROR;
    } else {
      for (int i = 0; i < MAX_BLOCK_NUMBER; i++) {
        if (s21_decimal_is_set_bit(truncated_decimal, i) != 0) {
          *dst += pow(2, i);
        }
      }

      if (s21_decimal_get_sign(src) == S21_NEGATIVE && *dst != -2147483648) {
        *dst = -*dst;
      }
    }
  }

  return code;
}

int s21_from_float_to_decimal(float src, s21_decimal *dst) {
  s21_conversion_result code = S21_CONVERSION_OK;
  if (!dst) {
    code = S21_CONVERSION_ERROR;
  } else if (src == 0.0) {
    code = S21_CONVERSION_OK;
    *dst = s21_decimal_get_zero();
    if (signbit(src) != 0) {
      s21_decimal_set_sign(dst, S21_NEGATIVE);
    }
  } else if (isinf(src) || isnan(src)) {
    code = S21_CONVERSION_ERROR;
    *dst = s21_decimal_get_inf();
    if (signbit(src) != 0) {
      s21_decimal_set_sign(dst, S21_NEGATIVE);
    }
  } else if (fabsf(src) > MAX_FLOAT_TO_CONVERT) {
    code = S21_CONVERSION_ERROR;
    *dst = s21_decimal_get_inf();
    if (signbit(src) != 0) {
      s21_decimal_set_sign(dst, S21_NEGATIVE);
    }
  } else if (fabsf(src) < MIN_FLOAT_TO_CONVERT) {
    code = S21_CONVERSION_ERROR;
    *dst = s21_decimal_get_zero();
  } else {
    *dst = s21_decimal_get_zero();
    s21_decimal result;
    char flt[64];

    sprintf(flt, "%.6E", fabsf(src));
    int exp = s21_get_float_exp_from_string(flt);
    if (exp <= -23) {
      int float_precision = exp + 28;
      sprintf(flt, "%.*E", float_precision, fabsf(src));
    }

    result = s21_float_string_to_decimal(flt);

    if (signbit(src) != 0) {
      s21_decimal_set_sign(&result, S21_NEGATIVE);
    }

    *dst = result;
  }

  return code;
}

int s21_from_int_to_decimal(int src, s21_decimal *dst) {
  s21_conversion_result code = S21_CONVERSION_OK;

  if (!dst) {
    code = S21_CONVERSION_ERROR;
  } else {
    *dst = s21_decimal_get_zero();
    int sign;

    if (src < 0) {
      sign = S21_NEGATIVE;
      if (src != INT_MIN) {
        src = -src;
      }
    } else {
      sign = S21_POSITIVE;
    }

    dst->bits[0] = src;
    s21_decimal_set_sign(dst, sign);
  }

  return code;
}

s21_decimal s21_create_decimal_from_array(int data1, int data2, int data3,
                                          int data4) {
  s21_decimal decimal;

  decimal.bits[0] = data1;
  decimal.bits[1] = data2;
  decimal.bits[2] = data3;
  decimal.bits[3] = data4;

  return decimal;
}

s21_decimal s21_create_decimal_from_data(int sign, int power, int data1,
                                         int data2, int data3) {
  s21_decimal decimal;
  s21_clear_decimal(&decimal);

  decimal.bits[0] = data1;
  decimal.bits[1] = data2;
  decimal.bits[2] = data3;

  s21_decimal_set_power(&decimal, power);
  s21_decimal_set_sign(&decimal, sign);

  return decimal;
}

s21_decimal s21_create_decimal_from_strings(char *str1, char *str2, char *str3,
                                            char *str4) {
  s21_decimal decimal = s21_decimal_get_zero();
  int error = 0;

  error = s21_decimal_set_bits_from_string(&decimal.bits[0], str1);

  if (error == 0) {
    error = s21_decimal_set_bits_from_string(&decimal.bits[1], str2);
  }

  if (error == 0) {
    error = s21_decimal_set_bits_from_string(&decimal.bits[2], str3);
  }

  if (error == 0) {
    error = s21_decimal_set_bits_from_string(&decimal.bits[3], str4);
  }

  if (error == 1) {
    decimal = s21_decimal_get_inf();
  }

  return decimal;
}

int s21_decimal_set_bits_from_string(int *bits, char *str) {
  int index = 0;
  int error = 0;

  for (int i = (int)strlen(str) - 1; i >= 0; i--) {
    if (str[i] == ' ') {
      continue;
    } else if (str[i] == '1') {
      *bits = s21_set_bit(*bits, index);
    } else if (str[i] == '0') {
      *bits = s21_reset_bit(*bits, index);
    } else {
      error = 1;
      break;
    }
    ++index;
  }

  return error;
}

void s21_clear_decimal(s21_decimal *decimal) {
  decimal->bits[0] = 0;
  decimal->bits[1] = 0;
  decimal->bits[2] = 0;
  decimal->bits[3] = 0;
}

void s21_decimal_null_service_bits(s21_decimal *value) { value->bits[3] = 0; }

s21_decimal s21_decimal_get_zero(void) {
  s21_decimal result;
  s21_clear_decimal(&result);

  return result;
}

s21_decimal s21_decimal_get_one(void) {
  s21_decimal result;
  s21_clear_decimal(&result);
  result.bits[0] = 1;

  return result;
}

s21_decimal s21_decimal_get_ten(void) {
  s21_decimal result;
  s21_clear_decimal(&result);
  result.bits[0] = 10;

  return result;
}

s21_decimal s21_int128_get_ten_pow(int pow) { return all_ten_pows[pow]; }

s21_decimal s21_decimal_get_zerofive(void) {
  s21_decimal result = {{0x5, 0x0, 0x0, 0x10000}};

  return result;
}

s21_decimal s21_decimal_get_min(void) {
  s21_decimal result = {{0x1, 0x0, 0x0, 0x1C0000}};

  return result;
}

s21_decimal s21_decimal_get_max(void) {
  s21_decimal result = {{0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x0}};

  return result;
}

s21_decimal s21_decimal_get_int_max(void) {
  s21_decimal result = {{0x7FFFFFFF, 0x0, 0x0, 0x0}};

  return result;
}

s21_decimal s21_decimal_get_int_min(void) {
  s21_decimal result = {{0x80000000, 0x0, 0x0, 0x80000000}};

  return result;
}

s21_int256 s21_create_int256_from_decimal(s21_decimal value_1) {
  s21_int256 result;
  result.decimals[0] = value_1;
  result.decimals[1] = s21_decimal_get_zero();
  return result;
}

s21_decimal s21_decimal_get_inf(void) {
  s21_decimal result = {{0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x7FFFFFFF}};

  return result;
}

int s21_is_correct_decimal(s21_decimal decimal) {
  int code = 1;

  if (s21_decimal_get_empty1(decimal) != 0 ||
      s21_decimal_get_empty2(decimal) != 0) {
    code = 0;
  } else {
    int power = s21_decimal_get_power(decimal);
    if (power < 0 || power > 28) {
      code = 0;
    }
  }

  return code;
}

int s21_decimal_get_sign(s21_decimal decimal) {
  decimal_bit3 bits3;
  bits3.i = decimal.bits[3];

  return bits3.parts.sign;
}

int s21_decimal_get_power(s21_decimal decimal) {
  decimal_bit3 bits3;
  bits3.i = decimal.bits[3];

  return bits3.parts.power;
}

int s21_decimal_get_empty1(s21_decimal decimal) {
  decimal_bit3 bits3;
  bits3.i = decimal.bits[3];

  return bits3.parts.empty1;
}

int s21_decimal_get_empty2(s21_decimal decimal) {
  decimal_bit3 bits3;
  bits3.i = decimal.bits[3];

  return bits3.parts.empty2;
}

void s21_decimal_set_sign(s21_decimal *decimal, int sign) {
  decimal_bit3 bits3;
  bits3.i = decimal->bits[3];
  if (sign == S21_POSITIVE) {
    bits3.parts.sign = S21_POSITIVE;
  } else {
    bits3.parts.sign = S21_NEGATIVE;
  }

  decimal->bits[3] = bits3.i;
}

void s21_decimal_set_power(s21_decimal *decimal, int power) {
  decimal_bit3 bits3;
  bits3.i = decimal->bits[3];
  bits3.parts.power = power;

  decimal->bits[3] = bits3.i;
}

int s21_decimal_even(s21_decimal value) { return (value.bits[0] & 1) != 1; }

int s21_floor(s21_decimal value, s21_decimal *result) {
  s21_other_result code = S21_OTHER_OK;

  if (!result) {
    code = S21_OTHER_ERROR;
  } else if (!s21_is_correct_decimal(value)) {
    code = S21_OTHER_ERROR;
    *result = s21_decimal_get_inf();
  } else {
    *result = s21_decimal_get_zero();
    int sign = s21_decimal_get_sign(value);
    s21_decimal fractional;
    s21_decimal value_unsigned_truncated;
    s21_decimal value_unsigned = s21_abs(value);

    s21_truncate(value_unsigned, &value_unsigned_truncated);

    s21_sub(value_unsigned, value_unsigned_truncated, &fractional);

    if (sign == S21_NEGATIVE &&
        s21_is_greater(fractional, s21_decimal_get_zero())) {
      s21_add(value_unsigned_truncated, s21_decimal_get_one(),
              &value_unsigned_truncated);
    }

    *result = value_unsigned_truncated;
    s21_decimal_set_sign(result, sign);
  }

  return code;
}

int s21_negate(s21_decimal value, s21_decimal *result) {
  s21_other_result code = S21_OTHER_OK;

  if (!result) {
    code = S21_OTHER_ERROR;
  } else {
    *result = value;
    s21_decimal_set_sign(result, !s21_decimal_get_sign(*result));

    if (!s21_is_correct_decimal(value)) {
      code = S21_OTHER_ERROR;
    }
  }

  return code;
}

s21_decimal s21_remove_trailing_zeros(s21_decimal value) {
  s21_decimal result = value;
  int power = s21_decimal_get_power(value);
  int sign = s21_decimal_get_sign(value);

  if (power > 0 && s21_is_correct_decimal(value)) {
    s21_decimal remainder = s21_decimal_get_zero();
    s21_decimal tmp = value;
    s21_decimal_null_service_bits(&tmp);

    while (power > 0) {
      tmp = s21_int128_binary_division(tmp, s21_int128_get_ten_pow(1),
                                       &remainder);
      if (s21_int128_binary_equal_zero(remainder)) {
        --power;
        result = tmp;
      } else {
        break;
      }
    }
    s21_decimal_set_power(&result, power);
    s21_decimal_set_sign(&result, sign);
  }

  return result;
}

int s21_round(s21_decimal value, s21_decimal *result) {
  s21_other_result code = S21_OTHER_OK;

  if (!result) {
    code = S21_OTHER_ERROR;
  } else if (!s21_is_correct_decimal(value)) {
    code = S21_OTHER_ERROR;
    *result = s21_decimal_get_inf();
  } else {
    *result = s21_decimal_get_zero();
    int sign = s21_decimal_get_sign(value);
    s21_decimal fractional;
    s21_decimal value_unsigned_truncated;
    s21_decimal value_unsigned = s21_abs(value);
    s21_truncate(value_unsigned, &value_unsigned_truncated);

    s21_sub(value_unsigned, value_unsigned_truncated, &fractional);

    value_unsigned_truncated =
        s21_round_banking(value_unsigned_truncated, fractional);

    *result = value_unsigned_truncated;
    s21_decimal_set_sign(result, sign);
  }

  return code;
}

s21_decimal s21_round_banking(s21_decimal integral, s21_decimal fractional) {
  s21_decimal zerofive = s21_decimal_get_zerofive();
  s21_decimal result;

  if (s21_is_equal(fractional, zerofive)) {
    if (s21_decimal_even(integral)) {
      result = integral;
    } else {
      result = s21_int128_binary_addition(integral, s21_decimal_get_one());
    }
  } else if (s21_is_greater(fractional, zerofive)) {
    result = s21_int128_binary_addition(integral, s21_decimal_get_one());
  } else {
    result = integral;
  }

  return result;
}

int s21_truncate(s21_decimal value, s21_decimal *result) {
  s21_other_result code = S21_OTHER_OK;

  if (!result) {
    code = S21_OTHER_ERROR;
  } else if (!s21_is_correct_decimal(value)) {
    code = S21_OTHER_ERROR;
    *result = s21_decimal_get_inf();
  } else {
    *result = s21_decimal_get_zero();
    int power = s21_decimal_get_power(value);
    s21_decimal tmp = value;
    s21_decimal_null_service_bits(&tmp);

    tmp = s21_int128_binary_division(tmp, s21_int128_get_ten_pow(power), NULL);

    *result = tmp;
    if (s21_decimal_get_sign(value) == S21_NEGATIVE) {
      s21_decimal_set_sign(result, S21_NEGATIVE);
    }
  }

  return code;
}
