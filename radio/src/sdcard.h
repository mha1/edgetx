/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _SDCARD_H_
#define _SDCARD_H_

#include "ff.h"

extern FATFS g_FATFS_Obj;

#include "translations.h"

bool sdCardFormat();
uint32_t sdGetNoSectors();
uint32_t sdGetSize();
uint32_t sdGetFreeSectors();

unsigned int findNextFileIndex(char * filename, uint8_t size, const char * directory);

#define LIST_NONE_SD_FILE   1
#define LIST_SD_FILE_EXT    2
bool sdListFiles(const char * path, const char * extension, const uint8_t maxlen, const char * selection, uint8_t flags=0);

#endif // _SDCARD_H_
