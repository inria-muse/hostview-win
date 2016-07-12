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

#include <string>
#include <vector>
#include <map>

#ifndef tstring
#define tstring wstring
#endif

#if defined(ESMLIBRARY_EXPORT) // inside DLL
#   define ESMAPI   __declspec(dllexport)
#else // outside DLL
#   define ESMAPI   __declspec(dllimport)
#endif  // ESMLIBRARY_EXPORT

void ESMAPI UpdateQuestionnaireAppsList(TCHAR *szUser, TCHAR *szApp, TCHAR *szDescription);
void ESMAPI UpdateQuestionnaireTabsList(TCHAR *szUser, TCHAR *szHost, TCHAR *szBrowser);

void ESMAPI SaveAppsList(TCHAR *szUser, TCHAR *szFilename, size_t nSize, ULONG applistHist);
void ESMAPI GenerateQuestionnaireCommand(TCHAR *szUser, BOOL fOnDemand, TCHAR *szCmdLine, size_t nSize);

size_t ESMAPI QueryQuestionnaireCounter();
void ESMAPI SetQuestionnaireCounter(size_t nCounter);

struct App
{
	std::tstring app;
	std::tstring description;

	// used to specify app info
	// in case of a browser tab
	std::tstring extra;

	DWORD time;

	App(std::tstring app, std::tstring description, std::tstring extra, DWORD time)
	{
		this->app = app;
		this->description = description;
		this->extra = extra;
		this->time = time;
	}
};

typedef std::vector<App> AppListT;

void ESMAPI LoadAppList(TCHAR *szCmdLine, AppListT* &appList);
void ESMAPI ClearAppList(AppListT* &appList);

void ESMAPI SubmitQuestionnaire(const DWORD dur, const TCHAR *szResult);
void ESMAPI SubmitQuestionnaireActivity(const TCHAR *szResult);
void ESMAPI SubmitQuestionnaireProblem(const TCHAR *szResult);
void ESMAPI SubmitQuestionnaireDone();