/**
* The MIT License (MIT)
*
* Copyright (c) 2015-2016 MUSE / Inria
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
**/
#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN

#define _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS // VS2015 compat

// C RunTime Header Files
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// Windows Header Files:
#include <windows.h>

#define COMMLIBRARY_EXPORT
#define HTTPSERVERLIBRARY_EXPORT
#define KNOWNLIBRARY_EXPORT
#define ESMLIBRARY_EXPORT
#define SETTINGSLIBRARY_EXPORT
#define STORELIBRARY_EXPORT
#define TRACELIBRARY_EXPORT
#define UPDATELIBRARY_EXPORT
#define UPLOADLIBRARY_EXPORT

