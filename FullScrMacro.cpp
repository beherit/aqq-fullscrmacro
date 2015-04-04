//---------------------------------------------------------------------------
// Copyright (C) 2013-2015 Krzysztof Grochocki
//
// This file is part of FullScrMacro
//
// FullScrMacro is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3, or (at your option)
// any later version.
//
// FullScrMacro is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GNU Radio; see the file COPYING. If not, write to
// the Free Software Foundation, Inc., 51 Franklin Street,
// Boston, MA 02110-1301, USA.
//---------------------------------------------------------------------------

#include <vcl.h>
#include <windows.h>
#include <inifiles.hpp>
#include <process.h>
#include <IdHashMessageDigest.hpp>
#include <PluginAPI.h>
#include <LangAPI.hpp>
#pragma hdrstop
#include "SettingsFrm.h"

int WINAPI DllEntryPoint(HINSTANCE hinst, unsigned long reason, void* lpReserved)
{
	return 1;
}
//---------------------------------------------------------------------------

//Uchwyt-do-formy-ustawien---------------------------------------------------
TSettingsForm *hSettingsForm;
//Struktury-glowne-----------------------------------------------------------
TPluginLink PluginLink;
TPluginInfo PluginInfo;
//Uchwyt-do-okna-timera------------------------------------------------------
HWND hTimerFrm;
//PID-procesu----------------------------------------------------------------
DWORD ProcessPID;
//Poprzedni-stan-konta-------------------------------------------------------
int LastState;
UnicodeString LastStatus;
bool MacroExecuted = false;
//TIMERY---------------------------------------------------------------------
#define TIMER_CHKACTIVEWINDOW 10
#define TIMER_SETSTATUS 20
//SETTINGS-------------------------------------------------------------------
int State;
UnicodeString Status;
int DelayValue;
//FORWARD-AQQ-HOOKS----------------------------------------------------------
INT_PTR __stdcall OnColorChange(WPARAM wParam, LPARAM lParam);
INT_PTR __stdcall OnLangCodeChanged(WPARAM wParam, LPARAM lParam);
INT_PTR __stdcall OnThemeChanged(WPARAM wParam, LPARAM lParam);
INT_PTR __stdcall ServiceFullScrMacroFastSettingsItem(WPARAM wParam, LPARAM lParam);
//FORWARD-TIMER--------------------------------------------------------------
LRESULT CALLBACK TimerFrmProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//---------------------------------------------------------------------------

//Pobieranie sciezki katalogu prywatnego wtyczek
UnicodeString GetPluginUserDir()
{
	return StringReplace((wchar_t*)PluginLink.CallService(AQQ_FUNCTION_GETPLUGINUSERDIR, 0, 0), "\\", "\\\\", TReplaceFlags() << rfReplaceAll);
}
//---------------------------------------------------------------------------

//Pobieranie sciezki do skorki kompozycji
UnicodeString GetThemeSkinDir()
{
	return StringReplace((wchar_t*)PluginLink.CallService(AQQ_FUNCTION_GETTHEMEDIR, 0, 0), "\\", "\\\\", TReplaceFlags() << rfReplaceAll) + "\\\\Skin";
}
//---------------------------------------------------------------------------

//Pobieranie sciezki ikony z interfejsu AQQ
UnicodeString GetIconPath(int Icon)
{
	return StringReplace((wchar_t*)PluginLink.CallService(AQQ_FUNCTION_GETPNG_FILEPATH, Icon, 0), "\\", "\\\\", TReplaceFlags() << rfReplaceAll);
}
//--------------------------------------------------------------------------

//Sprawdzanie czy wlaczona jest zaawansowana stylizacja okien
bool ChkSkinEnabled()
{
	TStrings* IniList = new TStringList();
	IniList->SetText((wchar_t*)PluginLink.CallService(AQQ_FUNCTION_FETCHSETUP, 0, 0));
	TMemIniFile *Settings = new TMemIniFile(ChangeFileExt(Application->ExeName, ".INI"));
	Settings->SetStrings(IniList);
	delete IniList;
	UnicodeString SkinsEnabled = Settings->ReadString("Settings", "UseSkin", "1");
	delete Settings;
	return StrToBool(SkinsEnabled);
}
//---------------------------------------------------------------------------

//Sprawdzanie ustawien animacji AlphaControls
bool ChkThemeAnimateWindows()
{
	TStrings* IniList = new TStringList();
	IniList->SetText((wchar_t*)PluginLink.CallService(AQQ_FUNCTION_FETCHSETUP, 0, 0));
	TMemIniFile *Settings = new TMemIniFile(ChangeFileExt(Application->ExeName, ".INI"));
	Settings->SetStrings(IniList);
	delete IniList;
	UnicodeString AnimateWindowsEnabled = Settings->ReadString("Theme", "ThemeAnimateWindows", "1");
	delete Settings;
	return StrToBool(AnimateWindowsEnabled);
}
//---------------------------------------------------------------------------
bool ChkThemeGlowing()
{
	TStrings* IniList = new TStringList();
	IniList->SetText((wchar_t*)PluginLink.CallService(AQQ_FUNCTION_FETCHSETUP, 0, 0));
	TMemIniFile *Settings = new TMemIniFile(ChangeFileExt(Application->ExeName, ".INI"));
	Settings->SetStrings(IniList);
	delete IniList;
	UnicodeString GlowingEnabled = Settings->ReadString("Theme", "ThemeGlowing", "1");
	delete Settings;
	return StrToBool(GlowingEnabled);
}
//---------------------------------------------------------------------------

//Pobieranie ustawien koloru AlphaControls
int GetHUE()
{
	return (int)PluginLink.CallService(AQQ_SYSTEM_COLORGETHUE, 0, 0);
}
//---------------------------------------------------------------------------
int GetSaturation()
{
	return (int)PluginLink.CallService(AQQ_SYSTEM_COLORGETSATURATION, 0, 0);
}
//---------------------------------------------------------------------------
int GetBrightness()
{
	return (int)PluginLink.CallService(AQQ_SYSTEM_COLORGETBRIGHTNESS, 0, 0);
}
//---------------------------------------------------------------------------

//Kodowanie ciagu znakow do Base64
UnicodeString EncodeBase64(UnicodeString Str)
{
	return (wchar_t*)PluginLink.CallService(AQQ_FUNCTION_BASE64, (WPARAM)Str.w_str(), 3);
}
//---------------------------------------------------------------------------

//Dekodowanie ciagu znakow z Base64
UnicodeString DecodeBase64(UnicodeString Str)
{
	return (wchar_t*)PluginLink.CallService(AQQ_FUNCTION_BASE64, (WPARAM)Str.w_str(), 2);
}
//---------------------------------------------------------------------------

//Pobieranie aktualnego opisu konta glownego
UnicodeString GetStatus()
{
	TPluginStateChange PluginStateChange;
	PluginLink.CallService(AQQ_FUNCTION_GETNETWORKSTATE, (WPARAM)&PluginStateChange, 0);
	return (wchar_t*)PluginStateChange.Status;
}
//---------------------------------------------------------------------------

//Pobieranie stanu konta glownego
int GetState()
{
	TPluginStateChange PluginStateChange;
	PluginLink.CallService(AQQ_FUNCTION_GETNETWORKSTATE, (WPARAM)&PluginStateChange, 0);
	return PluginStateChange.NewState;
}
//---------------------------------------------------------------------------

//Ustawianie nowego stanu konta
void SetStatus(int NewState, UnicodeString NewStatus)
{
	TPluginStateChange PluginStateChange;
	PluginLink.CallService(AQQ_FUNCTION_GETNETWORKSTATE, (WPARAM)&PluginStateChange, 0);
	//PluginStateChange.cbSize = sizeof(TPluginStateChange);
	PluginStateChange.NewState = NewState;
	PluginStateChange.Status = NewStatus.w_str();
	PluginStateChange.Force = true;
	PluginLink.CallService(AQQ_SYSTEM_SETSHOWANDSTATUS, 0, (LPARAM)&PluginStateChange);
}
//---------------------------------------------------------------------------

//Sprawdzanie czy wskazane okno jest pelno ekranowe
bool ChkFullScreenMode(HWND hWnd)
{
	//Pobieranie wymiarow aktywnego okna
	TRect ActiveFrmRect;
	GetWindowRect(hWnd, &ActiveFrmRect);;
	//Sprawdzanie szerokosci/wysokosci okna
	if((ActiveFrmRect.Width()==Screen->Width)&&(ActiveFrmRect.Height()==Screen->Height))
	{
		//Pobieranie klasy aktywnego okna
		wchar_t WClassName[128];
		GetClassNameW(hWnd, WClassName, sizeof(WClassName));
		//Wyjatek dla pulpitu oraz programu DeskScapes
		if(((UnicodeString)WClassName!="Progman")&&((UnicodeString)WClassName!="SysListView32")&&((UnicodeString)WClassName!="WorkerW")&&((UnicodeString)WClassName!="NDesk"))
		{
			return true;
		}
	}
	//Szukanie aplikacji Metro UI
	else
	{
		//Pobieranie klasy aktywnego okna
		wchar_t WClassName[128];
		GetClassNameW(hWnd, WClassName, sizeof(WClassName));
		//Aplikacja Metro UI
		if(AnsiPos("Windows.UI.Core.CoreWindow", (UnicodeString)WClassName)) return true;
	}
	return false;
}
//---------------------------------------------------------------------------

//Serwis szybkiego dostepu do ustawien wtyczki
INT_PTR __stdcall ServiceFullScrMacroFastSettingsItem(WPARAM wParam, LPARAM lParam)
{
	//Przypisanie uchwytu do formy ustawien
	if(!hSettingsForm)
	{
		Application->Handle = (HWND)SettingsForm;
		hSettingsForm = new TSettingsForm(Application);
	}
	//Pokaznie okna ustawien
	hSettingsForm->Show();

	return 0;
}
//---------------------------------------------------------------------------

//Usuwanie elementu szybkiego dostepu do ustawien wtyczki
void DestroyFullScrMacroFastSettings()
{
	//Usuwanie elementu szybkiego dostepu do ustawien wtyczki
	TPluginAction BuildFullScrMacroFastSettingsItem;
	ZeroMemory(&BuildFullScrMacroFastSettingsItem, sizeof(TPluginAction));
	BuildFullScrMacroFastSettingsItem.cbSize = sizeof(TPluginAction);
	BuildFullScrMacroFastSettingsItem.pszName = L"FullScrMacroFastSettingsItemButton";
	PluginLink.CallService(AQQ_CONTROLS_DESTROYPOPUPMENUITEM, 0, (LPARAM)(&BuildFullScrMacroFastSettingsItem));
	//Usuwanie serwisu szybkiego dostepu do ustawien wtyczki
	PluginLink.DestroyServiceFunction(ServiceFullScrMacroFastSettingsItem);
}
//---------------------------------------------------------------------------

//Tworzenie elementu szybkiego dostepu do ustawien wtyczki
void BuildFullScrMacroFastSettings()
{
	//Tworzenie serwisu szybkiego dostepu do ustawien wtyczki
	PluginLink.CreateServiceFunction(L"sFullScrMacroFastSettingsItem", ServiceFullScrMacroFastSettingsItem);
	//Tworzenie elementu szybkiego dostepu do ustawien wtyczki
	TPluginAction BuildFullScrMacroFastSettingsItem;
	ZeroMemory(&BuildFullScrMacroFastSettingsItem, sizeof(TPluginAction));
	BuildFullScrMacroFastSettingsItem.cbSize = sizeof(TPluginAction);
	BuildFullScrMacroFastSettingsItem.pszName = L"FullScrMacroFastSettingsItemButton";
	BuildFullScrMacroFastSettingsItem.pszCaption = L"FullScrMacro";
	BuildFullScrMacroFastSettingsItem.IconIndex = 63;
	BuildFullScrMacroFastSettingsItem.pszService = L"sFullScrMacroFastSettingsItem";
	BuildFullScrMacroFastSettingsItem.pszPopupName = L"popPlugins";
	PluginLink.CallService(AQQ_CONTROLS_CREATEPOPUPMENUITEM, 0, (LPARAM)(&BuildFullScrMacroFastSettingsItem));
}
//---------------------------------------------------------------------------

//Procka okna timera
LRESULT CALLBACK TimerFrmProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//Notfikacja timera
	if(uMsg==WM_TIMER)
	{
		//Timer sprawdzajacy aktywne okno
		if(wParam==TIMER_CHKACTIVEWINDOW)
		{
			//Pobieranie uchwytu do aktywnego okna
			HWND hActiveFrm = GetForegroundWindow();
			//Pobieranie klasy aktywnego okna
			wchar_t* ClassName = new wchar_t[128];
			GetClassNameW(hActiveFrm, ClassName, 128);
			//Pobranie PID procesu aktywnego okna
			DWORD PID;
			GetWindowThreadProcessId(hwnd, &PID);
			//Wyjatki w sprawdzaniu okien
			if(((UnicodeString)ClassName!="MSTaskListWClass")
			&&((UnicodeString)ClassName!="TaskSwitcherWnd"))
			{
				//Sprawdzenie czy wskazane okno jest pelno ekranowe
				if((ChkFullScreenMode(hActiveFrm))&&((PID!=ProcessPID)||((UnicodeString)ClassName=="ShockwaveFlashFullScreen")))
				{
					//Makro nie zostalo uruchomione & uruchomienie makra jest mozliwe
					if((!MacroExecuted)&&((0<GetState())&&(GetState()<6)))
					{
						//Opozniona zmiana stanu kont
						if(DelayValue)
						{
							//Odznaczenie wykonania makra
							MacroExecuted = true;
							//Wlaczenie timera zmiany stanu kont
							SetTimer(hTimerFrm, TIMER_SETSTATUS, DelayValue*1000, (TIMERPROC)TimerFrmProc);
						}
						//Natychmiastowa zmiana stanu kont
						else
						{
							//Odznaczenie wykonania makra
							MacroExecuted = true;
							//Pobieranie aktualnego stanu konta glownego
							LastState = GetState();
							LastStatus = GetStatus();
							//Ustawienie nowego stanu kont
							if(!Status.IsEmpty()) SetStatus(State, Status);
							else SetStatus(State, LastStatus);
						}
					}
				}
				//Przywracanie poprzedniego stanu kont
				else if(MacroExecuted)
				{
					//Wylaczenie timera zmiany stanu kont
					KillTimer(hTimerFrm, TIMER_SETSTATUS);
					//Odznaczenie wykonania makra
					MacroExecuted = false;
					//Ustawienie poprzedniego stanu kont
					SetStatus(LastState, LastStatus);
				}
			}
		}
		//Timer ustawiania nowego stanu
		else if(wParam==TIMER_SETSTATUS)
		{
			//Wylaczenie timera
			KillTimer(hTimerFrm, TIMER_SETSTATUS);
			//Pobieranie aktualnego stanu konta glownego
			LastState = GetState();
			LastStatus = GetStatus();
			//Ustawienie nowego stanu kont
			if(!Status.IsEmpty()) SetStatus(State, Status);
			else SetStatus(State, LastStatus);
		}

		return 0;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------

//Hook na zmiane kolorystyki AlphaControls
INT_PTR __stdcall OnColorChange(WPARAM wParam, LPARAM lParam)
{
	//Okno ustawien zostalo juz stworzone
	if(hSettingsForm)
	{
		//Wlaczona zaawansowana stylizacja okien
		if(ChkSkinEnabled())
		{
			TPluginColorChange ColorChange = *(PPluginColorChange)wParam;
			hSettingsForm->sSkinManager->HueOffset = ColorChange.Hue;
			hSettingsForm->sSkinManager->Saturation = ColorChange.Saturation;
			SettingsForm->sSkinManager->Brightness = ColorChange.Brightness;
		}
	}

	return 0;
}
//---------------------------------------------------------------------------

//Hook na zmiane lokalizacji
INT_PTR __stdcall OnLangCodeChanged(WPARAM wParam, LPARAM lParam)
{
  //Czyszczenie cache lokalizacji
	ClearLngCache();
	//Pobranie sciezki do katalogu prywatnego uzytkownika
	UnicodeString PluginUserDir = GetPluginUserDir();
	//Ustawienie sciezki lokalizacji wtyczki
	UnicodeString LangCode = (wchar_t*)lParam;
	LangPath = PluginUserDir + "\\\\Languages\\\\FullScrMacro\\\\" + LangCode + "\\\\";
	if(!DirectoryExists(LangPath))
	{
		LangCode = (wchar_t*)PluginLink.CallService(AQQ_FUNCTION_GETDEFLANGCODE, 0, 0);
		LangPath = PluginUserDir + "\\\\Languages\\\\FullScrMacro\\\\" + LangCode + "\\\\";
	}
	//Aktualizacja lokalizacji form wtyczki
	for(int i=0; i<Screen->FormCount; i++)
		LangForm(Screen->Forms[i]);

	return 0;
}
//---------------------------------------------------------------------------

//Hook na zmianê kompozycji
INT_PTR __stdcall OnThemeChanged(WPARAM wParam, LPARAM lParam)
{
	//Okno ustawien zostalo juz stworzone
	if(hSettingsForm)
	{
		//Wlaczona zaawansowana stylizacja okien
		if(ChkSkinEnabled())
		{
			//Pobieranie sciezki nowej aktywnej kompozycji
			UnicodeString ThemeSkinDir = StringReplace((wchar_t*)lParam, "\\", "\\\\", TReplaceFlags() << rfReplaceAll) + "\\\\Skin";
			//Plik zaawansowanej stylizacji okien istnieje
			if(FileExists(ThemeSkinDir + "\\\\Skin.asz"))
			{
				//Dane pliku zaawansowanej stylizacji okien
				ThemeSkinDir = StringReplace(ThemeSkinDir, "\\\\", "\\", TReplaceFlags() << rfReplaceAll);
				hSettingsForm->sSkinManager->SkinDirectory = ThemeSkinDir;
				hSettingsForm->sSkinManager->SkinName = "Skin.asz";
				//Ustawianie animacji AlphaControls
				if(ChkThemeAnimateWindows()) hSettingsForm->sSkinManager->AnimEffects->FormShow->Time = 200;
				else hSettingsForm->sSkinManager->AnimEffects->FormShow->Time = 0;
				hSettingsForm->sSkinManager->Effects->AllowGlowing = ChkThemeGlowing();
				//Zmiana kolorystyki AlphaControls
				hSettingsForm->sSkinManager->HueOffset = GetHUE();
				hSettingsForm->sSkinManager->Saturation = GetSaturation();
				hSettingsForm->sSkinManager->Brightness = GetBrightness();
				//Aktywacja skorkowania AlphaControls
				hSettingsForm->sSkinManager->Active = true;
			}
			//Brak pliku zaawansowanej stylizacji okien
			else hSettingsForm->sSkinManager->Active = false;
		}
		//Zaawansowana stylizacja okien wylaczona
		else hSettingsForm->sSkinManager->Active = false;
	}

	return 0;
}
//---------------------------------------------------------------------------

//Zapisywanie zasobów
void ExtractRes(wchar_t* FileName, wchar_t* ResName, wchar_t* ResType)
{
	TPluginTwoFlagParams PluginTwoFlagParams;
	PluginTwoFlagParams.cbSize = sizeof(TPluginTwoFlagParams);
	PluginTwoFlagParams.Param1 = ResName;
	PluginTwoFlagParams.Param2 = ResType;
	PluginTwoFlagParams.Flag1 = (int)HInstance;
	PluginLink.CallService(AQQ_FUNCTION_SAVERESOURCE,(WPARAM)&PluginTwoFlagParams,(LPARAM)FileName);
}
//---------------------------------------------------------------------------

//Obliczanie sumy kontrolnej pliku
UnicodeString MD5File(UnicodeString FileName)
{
	if(FileExists(FileName))
	{
		UnicodeString Result;
		TFileStream *fs;
		fs = new TFileStream(FileName, fmOpenRead | fmShareDenyWrite);
		try
		{
			TIdHashMessageDigest5 *idmd5= new TIdHashMessageDigest5();
			try
			{
				Result = idmd5->HashStreamAsHex(fs);
			}
			__finally
			{
				delete idmd5;
			}
		}
		__finally
		{
			delete fs;
		}
		return Result;
	}
	else return 0;
}
//---------------------------------------------------------------------------

//Odczyt ustawien
void LoadSettings()
{
	TIniFile *Ini = new TIniFile(GetPluginUserDir()+"\\\\FullScrMacro\\\\Settings.ini");
	State = Ini->ReadInteger("Settings", "State", 5);
	Status = DecodeBase64(Ini->ReadString("Settings", "Status64", ""));
	DelayValue = Ini->ReadInteger("Settings", "Delay", 3);

	delete Ini;
}
//---------------------------------------------------------------------------

extern "C" INT_PTR __declspec(dllexport) __stdcall Load(PPluginLink Link)
{
	//Linkowanie wtyczki z komunikatorem
	PluginLink = *Link;
	//Pobranie PID procesu AQQ
	ProcessPID = getpid();
	//Sciezka folderu prywatnego wtyczek
	UnicodeString PluginUserDir = GetPluginUserDir();
  //Tworzenie katalogow lokalizacji
	if(!DirectoryExists(PluginUserDir + "\\\\Languages"))
		CreateDir(PluginUserDir + "\\\\Languages");
	if(!DirectoryExists(PluginUserDir + "\\\\Languages\\\\FullScrMacro"))
		CreateDir(PluginUserDir + "\\\\Languages\\\\FullScrMacro");
	if(!DirectoryExists(PluginUserDir + "\\\\Languages\\\\FullScrMacro\\\\EN"))
		CreateDir(PluginUserDir + "\\\\Languages\\\\FullScrMacro\\\\EN");
	if(!DirectoryExists(PluginUserDir + "\\\\Languages\\\\FullScrMacro\\\\PL"))
		CreateDir(PluginUserDir + "\\\\Languages\\\\FullScrMacro\\\\PL");
	//Wypakowanie plikow lokalizacji
	//B76E14CAC6117BF20671BA39F34AB05C
	if(!FileExists(PluginUserDir + "\\\\Languages\\\\FullScrMacro\\\\EN\\\\TSettingsForm.lng"))
		ExtractRes((PluginUserDir + "\\\\Languages\\\\FullScrMacro\\\\EN\\\\TSettingsForm.lng").w_str(), L"EN_SETTINGSFRM", L"DATA");
	else if(MD5File(PluginUserDir + "\\\\Languages\\\\FullScrMacro\\\\EN\\\\TSettingsForm.lng")!="B76E14CAC6117BF20671BA39F34AB05C")
		ExtractRes((PluginUserDir + "\\\\Languages\\\\FullScrMacro\\\\EN\\\\TSettingsForm.lng").w_str(), L"EN_SETTINGSFRM", L"DATA");
	//2BEB77B67771065FD33194361DEF4563
	if(!FileExists(PluginUserDir + "\\\\Languages\\\\FullScrMacro\\\\PL\\\\TSettingsForm.lng"))
		ExtractRes((PluginUserDir + "\\\\Languages\\\\FullScrMacro\\\\PL\\\\TSettingsForm.lng").w_str(), L"PL_SETTINGSFRM", L"DATA");
	else if(MD5File(PluginUserDir + "\\\\Languages\\\\FullScrMacro\\\\PL\\\\TSettingsForm.lng")!="2BEB77B67771065FD33194361DEF4563")
		ExtractRes((PluginUserDir + "\\\\Languages\\\\FullScrMacro\\\\PL\\\\TSettingsForm.lng").w_str(), L"PL_SETTINGSFRM", L"DATA");
  //Ustawienie sciezki lokalizacji wtyczki
	UnicodeString LangCode = (wchar_t*)PluginLink.CallService(AQQ_FUNCTION_GETLANGCODE, 0, 0);
	LangPath = PluginUserDir + "\\\\Languages\\\\FullScrMacro\\\\" + LangCode + "\\\\";
	if(!DirectoryExists(LangPath))
	{
		LangCode = (wchar_t*)PluginLink.CallService(AQQ_FUNCTION_GETDEFLANGCODE, 0, 0);
		LangPath = PluginUserDir + "\\\\Languages\\\\FullScrMacro\\\\" + LangCode + "\\\\";
	}
	//Tworzeniu katalogu z ustawieniami wtyczki
	if(!DirectoryExists(PluginUserDir+"\\\\FullScrMacro"))
		CreateDir(PluginUserDir+"\\\\FullScrMacro");
	//Tworzenie interfejsu szybkiego dostepu do ustawien wtyczki
	BuildFullScrMacroFastSettings();
	//Hook na zmiane kolorystyki AlphaControls
	PluginLink.HookEvent(AQQ_SYSTEM_COLORCHANGEV2, OnColorChange);
	//Hook na zmiane lokalizacji
	PluginLink.HookEvent(AQQ_SYSTEM_LANGCODE_CHANGED, OnLangCodeChanged);
	//Hook na zmiane kompozycji
	PluginLink.HookEvent(AQQ_SYSTEM_THEMECHANGED, OnThemeChanged);
	//Odczyt ustawien
	LoadSettings();
	//Rejestowanie klasy okna timera
	WNDCLASSEX wincl;
	wincl.cbSize = sizeof (WNDCLASSEX);
	wincl.style = 0;
	wincl.lpfnWndProc = TimerFrmProc;
	wincl.cbClsExtra = 0;
	wincl.cbWndExtra = 0;
	wincl.hInstance = HInstance;
	wincl.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wincl.hCursor = LoadCursor(NULL, IDC_ARROW);
	wincl.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
	wincl.lpszMenuName = NULL;
	wincl.lpszClassName = L"TFullScrMacro";
	wincl.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	RegisterClassEx(&wincl);
	//Tworzenie okna timera
	hTimerFrm = CreateWindowEx(0, L"TFullScrMacro", L"",	0, 0, 0, 0, 0, NULL, NULL, HInstance, NULL);
	//Timer na sprawdzanie aktywnego okna
	SetTimer(hTimerFrm, TIMER_CHKACTIVEWINDOW, 500, (TIMERPROC)TimerFrmProc);

	return 0;
}
//---------------------------------------------------------------------------

extern "C" INT_PTR __declspec(dllexport) __stdcall Unload()
{
	//Wyladowanie timerow
	for(int TimerID=10;TimerID<=20;TimerID=TimerID+10) KillTimer(hTimerFrm, TimerID);
	//Usuwanie okna timera
	DestroyWindow(hTimerFrm);
	//Wyrejestowanie klasy okna timera
	UnregisterClass(L"TFullScrMacro", HInstance);
	//Usuwanie interfejsu szybkiego dostepu do ustawien wtyczki
	DestroyFullScrMacroFastSettings();
	//Wyladowanie wszystkich hookow
	PluginLink.UnhookEvent(OnColorChange);
	PluginLink.UnhookEvent(OnLangCodeChanged);
	PluginLink.UnhookEvent(OnThemeChanged);

	return 0;
}
//---------------------------------------------------------------------------

//Ustawienia wtyczki
extern "C" INT_PTR __declspec(dllexport)__stdcall Settings()
{
	//Przypisanie uchwytu do formy ustawien
	if(!hSettingsForm)
	{
		Application->Handle = (HWND)SettingsForm;
		hSettingsForm = new TSettingsForm(Application);
	}
	//Pokaznie okna ustawien
	hSettingsForm->Show();

	return 0;
}
//---------------------------------------------------------------------------

extern "C" PPluginInfo __declspec(dllexport) __stdcall AQQPluginInfo(DWORD AQQVersion)
{
	PluginInfo.cbSize = sizeof(TPluginInfo);
	PluginInfo.ShortName = L"FullScrMacro";
	PluginInfo.Version = PLUGIN_MAKE_VERSION(1,1,2,0);
	PluginInfo.Description = L"Wtyczka zmienia stan wszystkich kont, gdy aktywna jest aplikacja pe³noekranowa.";
	PluginInfo.Author = L"Krzysztof Grochocki";
	PluginInfo.AuthorMail = L"kontakt@beherit.pl";
	PluginInfo.Copyright = L"Krzysztof Grochocki";
	PluginInfo.Homepage = L"http://beherit.pl";
	PluginInfo.Flag = 0;
	PluginInfo.ReplaceDefaultModule = 0;

	return &PluginInfo;
}
//---------------------------------------------------------------------------
