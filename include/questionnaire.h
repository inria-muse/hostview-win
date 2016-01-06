#pragma once

#include <string>
#include <vector>
#include <stdio.h>
#include "comm.h"

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

void ESMAPI SaveAppsList(TCHAR *szUser, TCHAR *szFilename, size_t nSize);
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
void ESMAPI SubmitQuestionnaire(const TCHAR *szResult);
