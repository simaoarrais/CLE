/**
 * @file prog1Utils.h (interface file)
 * @author Afonso Campos (afonso.campos@ua.pt)
 * @author Simão Arrais (simaoarrais@ua.pt)
 * @brief Problem name: Vowel Count.
 * 
 * Utility functions for UTF8 text processing.
 * 
 * Functions:
 *     \li getLetterSize
 *     \li isAlpha
 *     \li isSeparator
 *     \li isVowel.
 * 
 * @version 0.1
 * @date 2023-03-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef PROG1_UTILS_H
#define PROG1_UTILS_H

#include <stdbool.h>

/**
 * @brief Get the size in bytes of UTF8 character.
 * 
 * @param byte first byte of the character
 * @return unsigned int : size in bytes of the whole character
 */
extern unsigned int getLetterSize(unsigned char byte);

/**
 * @brief Check if UTF8 character is a separator.
 * 
 * @param bytes array of bytes of UTF8 character
 * @param size size of UTF8 character
 * @return true : UTF8 character is separator
 * @return false : UTF8 character is not separator
 */
extern bool isSeparator(unsigned char bytes[4], int size);

/**
 * @brief Check if UTF8 character is a vowel, if so, return the vowel without accentuation and lowercase
 * 
 * @param bytes array of bytes of UTF8 character
 * @param size size of UTF8 character
 * @return unsigned char : 'x' if not a vowel, otherwise, vowel with no accentuation and lower-case 
 */
extern unsigned char isVowel(unsigned char bytes[4], int size);

/**
 * @brief Check if UTF8 character is alpha.
 * 
 * @param bytes array of bytes of UTF8 character
 * @param size size of UTF8 character
 * @return true : UTF8 character is alpha
 * @return false : UTF8 character is not alpha
 */
extern bool isAlpha(unsigned char bytes[4], int size);

#endif 
