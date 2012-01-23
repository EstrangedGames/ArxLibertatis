/*
 * Copyright 2011 Arx Libertatis Team (see the AUTHORS file)
 *
 * This file is part of Arx Libertatis.
 *
 * Arx Libertatis is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Arx Libertatis is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Arx Libertatis.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ARX_CRASHREPORTER_UTILITIES_WIN32_H
#define ARX_CRASHREPORTER_UTILITIES_WIN32_H

#include "Configure.h"

#ifdef HAVE_WINAPI

HWND GetMainWindow(unsigned int processId);
bool GetWindowsVersionName(char* str, int bufferSize);
bool Is64BitWindows();
ULONG64 ConvertSystemTimeToULONG64( const SYSTEMTIME& st );

#endif

#endif // ARX_CRASHREPORTER_UTILITIES_WIN32_H