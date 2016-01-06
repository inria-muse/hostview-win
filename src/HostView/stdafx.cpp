/**
 * The MIT License (MIT)
 * 
 * Copyright (c) 2015 Muse / INRIA
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

#include "stdafx.h"

std::tstring GetTags(const TCHAR *szCategory)
{
	std::tstring result;

	FILE *f = NULL;

	TCHAR szFilename[MAX_PATH] = {0};
	_stprintf_s(szFilename, _T("tags\\%s"), szCategory);
	_tfopen_s(&f, szFilename, _T("r"));

	if (f)
	{
		
		fseek(f, 0, SEEK_END);
		long fsize = ftell(f);
		fseek(f, 0, SEEK_SET);

		char *szBuffer = new char[fsize + 1];
		fread(szBuffer, fsize, 1, f);
		szBuffer[fsize] = 0;

		TCHAR *szStr = new TCHAR[fsize + 1];
		_stprintf_s(szStr, fsize + 1, _T("%S"), szBuffer);
		result = szStr;

		delete [] szStr;
		delete [] szBuffer;

		fclose(f);
	}

	return result;
}

void SetTags(const TCHAR* szCategory, const TCHAR * szTags)
{
	FILE *f = NULL;
	TCHAR szFilename[MAX_PATH] = {0};
	_stprintf_s(szFilename, _T("tags\\%s"), szCategory);
	_tfopen_s(&f, szFilename, _T("w"));

	if (f)
	{
		_ftprintf_s(f, _T("%s"), szTags);
		fclose(f);
	}
}

void AddTag(const TCHAR *szCategory, const TCHAR * szTag)
{
	std::tstring tags = GetTags(szCategory);
	if (tags.length() > 0)
	{
		tags += L", ";
	}
	tags += std::tstring(szTag);

	SetTags(szCategory, tags.c_str());
}

