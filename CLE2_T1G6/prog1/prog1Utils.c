/**
 * @file prog1Utils.c (implementation file)
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

#include <stdbool.h>

/**
 * @brief Get the size in bytes of UTF8 character.
 * 
 * @param byte first byte of the character
 * @return unsigned int : size in bytes of the whole character
 */
unsigned int getLetterSize(unsigned char byte) {
    unsigned int size = 1;
    if((byte & 0xF8) == 0xF0) size = 4;
    else if((byte & 0xF0) == 0xE0) size = 3;
    else if((byte & 0xE0) == 0xC0) size = 2;
    else if((byte & 0x80) == 0x00) size = 1;
    return size;
}

/**
 * @brief Check if UTF8 character is alpha.
 * 
 * @param bytes array of bytes of UTF8 character
 * @param size size of UTF8 character
 * @return true : UTF8 character is alpha
 * @return false : UTF8 character is not alpha
 */
bool isAlpha(unsigned char bytes[4], int size) {
    bool ret = false;
    if(size == 1) { // a-z and A-Z
        if(((bytes[0] >= 0x41) && (bytes[0] <= 0x90)) ||
           ((bytes[0] >= 0x61) && (bytes[0] <= 0x7A))) ret = true;
    }
    else if(size == 2) { // acentos
        if(bytes[0] == 0xC3) {
            if(((bytes[1] <= 0xB6) && (bytes[1] >= 0x98)) || 
            ((bytes[1] <= 0x96) && (bytes[1] >= 0x80)) || 
            ((bytes[1] <= 0xBF) && (bytes[1] >= 0xB8))) ret = true; 
        }
    }
    return ret;
}

/**
 * @brief Check if UTF8 character is a separator.
 * 
 * @param bytes array of bytes of UTF8 character
 * @param size size of UTF8 character
 * @return true : UTF8 character is separator
 * @return false : UTF8 character is not separator
 */
bool isSeparator(unsigned char bytes[4], int size) {
    bool ret = false;
    if(size == 1) {
        if((bytes[0] == 0x9)  || (bytes[0] == 0xD)  || 
           (bytes[0] == 0xA)  || (bytes[0] == 0x20) ||
           (bytes[0] == 0x22) || (bytes[0] == '!')  ||
           (bytes[0] == '-')  || (bytes[0] == '[')  ||
           (bytes[0] == ']')  || (bytes[0] == '(')  ||
           (bytes[0] == ')')  || (bytes[0] == '.')  ||
           (bytes[0] == ',')  || (bytes[0] == ':')  ||
           (bytes[0] == ';')  || (bytes[0] == '?')  ) ret = true;
    }
    else if(size == 3) {
        if((bytes[0] == 0xE2) & (bytes[1] == 0x80)) {
            if((bytes[2] == 0x9C) || (bytes[2] == 0x9D) ||
               (bytes[2] == 0x93) || (bytes[2] == 0xA6)) ret = true;
        }
    }
    return ret;
}

/**
 * @brief Check if UTF8 character is a vowel, if so, return the vowel without accentuation and lowercase
 * 
 * @param bytes array of bytes of UTF8 character
 * @param size size of UTF8 character
 * @return unsigned char : 'x' if not a vowel, otherwise, vowel with no accentuation and lower-case 
 */
unsigned char isVowel(unsigned char bytes[4], int size) {
    char res = 'x'; // x means not a vowel, something else
    if(size == 1) {
        if((bytes[0] == 'a') || (bytes[0] == 'e') || 
           (bytes[0] == 'i') || (bytes[0] == 'o') ||
           (bytes[0] == 'u') || (bytes[0] == 'y') ) res = bytes[0];
        else {
            switch(bytes[0]) {
                case 'A':
                    res = 'a';
                    break;
                case 'E':
                    res = 'e';
                    break;
                case 'I':
                    res = 'i';
                    break;
                case 'O':
                    res = 'o';
                    break;
                case 'U':
                    res = 'u';
                    break;
                case 'Y':
                    res = 'y';
                    break;
            }
        }
    }
    else if(size == 2) {
        if(bytes[0] == 0xC3) {
            if(((bytes[1] <= 0xA6) && (bytes[1] >= 0xA0)) || 
            ((bytes[1] <= 0x86) && (bytes[1] >= 0x80))) res = 'a'; 
            else if(((bytes[1] <= 0xAB) && (bytes[1] >= 0xA8)) || 
            ((bytes[1] <= 0x8B) && (bytes[1] >= 0x88))) res = 'e';
            else if(((bytes[1] <= 0xAF) && (bytes[1] >= 0xAC)) || 
            ((bytes[1] <= 0x8F) && (bytes[1] >= 0x8C))) res = 'i';
            else if(((bytes[1] <= 0xB6) && (bytes[1] >= 0xB2)) ||
            ((bytes[1] <= 0x96) && (bytes[1] >= 0x92)) || (bytes[1] == 0xB8) || 
            (bytes[1] == 0xB0) || (bytes[1] == 0x98)) res = 'o';
            else if(((bytes[1] <= 0xBC) && (bytes[1] >= 0xB9)) || 
            ((bytes[1] <= 0x99) && (bytes[1] >= 0x9C))) res = 'u';
            else if((bytes[1] == 0xBF) || (bytes[1] == 0xBD) || (bytes[1] == 0x9D)) res = 'y';
        }
    }
    return res;
}
