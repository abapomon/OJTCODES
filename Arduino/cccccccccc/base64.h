#pragma once
#include <Arduino.h>

inline bool isBase64(char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

inline int base64_dec_len(const char* input, int len) {
  int padding = 0;
  if (len >= 1 && input[len - 1] == '=') padding++;
  if (len >= 2 && input[len - 2] == '=') padding++;
  return (len * 3) / 4 - padding;
}

inline void base64_decode(char* output, const char* input, int len) {
  const char* base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

  int i = 0, j = 0;
  uint8_t char_array_4[4], char_array_3[3];

  while (len-- && (*input != '=') && isBase64(*input)) {
    char_array_4[i++] = *input++;
    if (i == 4) {
      for (i = 0; i < 4; i++)
        char_array_4[i] = strchr(base64_chars, char_array_4[i]) - base64_chars;

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; i < 3; i++)
        output[j++] = char_array_3[i];

      i = 0;
    }
  }

  if (i) {
    for (int k = i; k < 4; k++)
      char_array_4[k] = 0;

    for (int k = 0; k < 4; k++)
      char_array_4[k] = strchr(base64_chars, char_array_4[k]) - base64_chars;

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (int k = 0; k < i - 1; k++)
      output[j++] = char_array_3[k];
  }
}
