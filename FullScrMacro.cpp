//---------------------------------------------------------------------------
// Copyright (C) 2013 Krzysztof Grochocki
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
#pragma hdrstop
#pragma argsused
#include <inifiles.hpp>
#include <PluginAPI.h>
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
int __stdcall OnColorChange(WPARAM wParam, LPARAM lParam);
int __stdcall OnThemeChanged(WPARAM wParam, LPARAM lParam);
int __stdcall ServiceFullScrMacroFastSettingsItem(WPARAM wParam, LPARAM lParam);
//FORWARD-TIMER--------------------------------------------------------------
LRESULT CALLBACK TimerFrmProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//---------------------------------------------------------------------------

//Konwersja ciagu znakow na potrzeby INI
UnicodeString StrToIniStr(UnicodeString Str)
{
  //Definicja zmiennych
  wchar_t Buffer[50010];
  wchar_t* B;
  wchar_t* S;
  //Przekazywanie ciagu znakow
  S = Str.w_str();
  //Ustalanie wskaznika
  B = Buffer;
  //Konwersja znakow
  while(*S!='\0')
  {
	switch(*S)
	{
	  case 13:
	  case 10:
		if((*S==13)&&(S[1]==10)) S++;
		else if((*S==10)&&(S[1] == 13)) S++;
		*B = '\\';
		B++;
		*B = 'n';
		B++;
		S++;
	  break;
	  default:
		*B = *S;
		B++;
		S++;
	  break;
	}
  }
  *B = '\0';
  //Zwracanie zkonwertowanego ciagu znakow
  return (wchar_t*)Buffer;
}
//---------------------------------------------------------------------------
UnicodeString IniStrToStr(UnicodeString Str)
{
  //Definicja zmiennych
  wchar_t Buffer[50010];
  wchar_t* B;
  wchar_t* S;
  //Przekazywanie ciagu znakow
  S = Str.w_str();
  //Ustalanie wskaznika
  B = Buffer;
  //Konwersja znakow
  while(*S!='\0')
  {
	if((S[0]=='\\')&&(S[1]=='n'))
	{
	  *B = 13;
	  B++;
	  *B = 10;
	  B++;
	  S++;
	  S++;
	}
	else
	{
	  *B = *S;
	  B++;
	  S++;
	}
  }
  *B = '\0';
  //Zwracanie zkonwertowanego ciagu znakow
  return (wchar_t*)Buffer;
}
//---------------------------------------------------------------------------

//Pobieranie sciezki katalogu prywatnego wtyczek
UnicodeString GetPluginUserDir()
{
  return StringReplace((wchar_t*)PluginLink.CallService(AQQ_FUNCTION_GETPLUGINUSERDIR,0,0), "\\", "\\\\", TReplaceFlags() << rfReplaceAll);
}
//---------------------------------------------------------------------------

//Pobieranie sciezki do skorki kompozycji
UnicodeString GetThemeSkinDir()
{
  return StringReplace((wchar_t*)PluginLink.CallService(AQQ_FUNCTION_GETTHEMEDIR,0,0), "\\", "\\\\", TReplaceFlags() << rfReplaceAll) + "\\\\Skin";
}
//---------------------------------------------------------------------------

//Pobieranie sciezki ikony z interfejsu AQQ
UnicodeString GetIconPath(int Icon)
{
  return StringReplace((wchar_t*)PluginLink.CallService(AQQ_FUNCTION_GETPNG_FILEPATH,Icon,0), "\\", "\\\\", TReplaceFlags() << rfReplaceAll);
}
//--------------------------------------------------------------------------

//Sprawdzanie czy  wlaczona jest zaawansowana stylizacja okien
bool ChkSkinEnabled()
{
  TStrings* IniList = new TStringList();
  IniList->SetText((wchar_t*)PluginLink.CallService(AQQ_FUNCTION_FETCHSETUP,0,0));
  TMemIniFile *Settings = new TMemIniFile(ChangeFileExt(Application->ExeName, ".INI"));
  Settings->SetStrings(IniList);
  delete IniList;
  UnicodeString SkinsEnabled = Settings->ReadString("Settings","UseSkin","1");
  delete Settings;
  return StrToBool(SkinsEnabled);
}
//---------------------------------------------------------------------------

//Sprawdzanie ustawien animacji AlphaControls
bool ChkThemeAnimateWindows()
{
  TStrings* IniList = new TStringList();
  IniList->SetText((wchar_t*)PluginLink.CallService(AQQ_FUNCTION_FETCHSETUP,0,0));
  TMemIniFile *Settings = new TMemIniFile(ChangeFileExt(Application->ExeName, ".INI"));
  Settings->SetStrings(IniList);
  delete IniList;
  UnicodeString AnimateWindowsEnabled = Settings->ReadString("Theme","ThemeAnimateWindows","1");
  delete Settings;
  return StrToBool(AnimateWindowsEnabled);
}
//---------------------------------------------------------------------------
bool ChkThemeGlowing()
{
  TStrings* IniList = new TStringList();
  IniList->SetText((wchar_t*)PluginLink.CallService(AQQ_FUNCTION_FETCHSETUP,0,0));
  TMemIniFile *Settings = new TMemIniFile(ChangeFileExt(Application->ExeName, ".INI"));
  Settings->SetStrings(IniList);
  delete IniList;
  UnicodeString GlowingEnabled = Settings->ReadString("Theme","ThemeGlowing","1");
  delete Settings;
  return StrToBool(GlowingEnabled);
}
//---------------------------------------------------------------------------

//Pobieranie ustawien koloru AlphaControls
int GetHUE()
{
  return (int)PluginLink.CallService(AQQ_SYSTEM_COLORGETHUE,0,0);
}
//---------------------------------------------------------------------------
int GetSaturation()
{
  return (int)PluginLink.CallService(AQQ_SYSTEM_COLORGETSATURATION,0,0);
}
//---------------------------------------------------------------------------

//Pobieranie aktualnego opisu konta glownego
UnicodeString GetStatus()
{
  TPluginStateChange PluginStateChange;
  PluginLink.CallService(AQQ_FUNCTION_GETNETWORKSTATE,(WPARAM)&PluginStateChange,0);
  return (wchar_t*)PluginStateChange.Status;
}
//---------------------------------------------------------------------------

//Pobieranie stanu konta glownego
int GetState()
{
  TPluginStateChange PluginStateChange;
  PluginLink.CallService(AQQ_FUNCTION_GETNETWORKSTATE,(WPARAM)&PluginStateChange,0);
  return PluginStateChange.NewState;
}
//---------------------------------------------------------------------------

//Ustawianie nowego stanu konta
void SetStatus(int NewState, UnicodeString NewStatus)
{
  TPluginStateChange PluginStateChange;
  PluginLink.CallService(AQQ_FUNCTION_GETNETWORKSTATE,(WPARAM)&PluginStateChange,0);
  //PluginStateChange.cbSize = sizeof(TPluginStateChange);
  PluginStateChange.NewState = NewState;
  PluginStateChange.Status = NewStatus.w_str();
  PluginStateChange.Force = true;
  PluginLink.CallService(AQQ_SYSTEM_SETSHOWANDSTATUS,0,(LPARAM)&PluginStateChange);
}
//---------------------------------------------------------------------------

//Sprawdzanie czy wskazane okno jest pelno ekranowe
bool ChkFullScreenMode(HWND hWnd)
{
  //Pobieranie wymiarow aktywnego okna
  TRect ActiveFrmRect;
  GetWindowRect(hWnd,&ActiveFrmRect);;
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
	if(AnsiPos("Windows.UI.Core.CoreWindow",(UnicodeString)WClassName)) return true;
  }
  return false;
}
//---------------------------------------------------------------------------

//Serwis szybkiego dostepu do ustawien wtyczki
int __stdcall ServiceFullScrMacroFastSettingsItem(WPARAM wParam, LPARAM lParam)
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
  TPluginAction BuildFullScrMacroFastSettingsItem;
  ZeroMemory(&BuildFullScrMacroFastSettingsItem,sizeof(TPluginAction));
  BuildFullScrMacroFastSettingsItem.cbSize = sizeof(TPluginAction);
  BuildFullScrMacroFastSettingsItem.pszName = L"FullScrMacroFastSettingsItemButton";
  PluginLink.CallService(AQQ_CONTROLS_DESTROYPOPUPMENUITEM,0,(LPARAM)(&BuildFullScrMacroFastSettingsItem));
}
//---------------------------------------------------------------------------

//Tworzenie elementu szybkiego dostepu do ustawien wtyczki
void BuildFullScrMacroFastSettings()
{
  TPluginAction BuildFullScrMacroFastSettingsItem;
  ZeroMemory(&BuildFullScrMacroFastSettingsItem,sizeof(TPluginAction));
  BuildFullScrMacroFastSettingsItem.cbSize = sizeof(TPluginAction);
  BuildFullScrMacroFastSettingsItem.pszName = L"FullScrMacroFastSettingsItemButton";
  BuildFullScrMacroFastSettingsItem.pszCaption = L"FullScrMacro";
  BuildFullScrMacroFastSettingsItem.IconIndex = 63;
  BuildFullScrMacroFastSettingsItem.pszService = L"sFullScrMacroFastSettingsItem";
  BuildFullScrMacroFastSettingsItem.pszPopupName = L"popPlugins";
  PluginLink.CallService(AQQ_CONTROLS_CREATEPOPUPMENUITEM,0,(LPARAM)(&BuildFullScrMacroFastSettingsItem));
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
	  //Wyjatki w sprawdzaniu okien
	  if(((UnicodeString)ClassName!="MSTaskListWClass")
	  &&((UnicodeString)ClassName!="TaskSwitcherWnd"))
	  {
		//Sprawdzenie czy wskazane okno jest pelno ekranowe
		if(ChkFullScreenMode(hActiveFrm))
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
			  SetTimer(hTimerFrm,TIMER_SETSTATUS,DelayValue*1000,(TIMERPROC)TimerFrmProc);
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
			  if(!Status.IsEmpty()) SetStatus(State,Status);
			  else SetStatus(State,LastStatus);
		    }
		  }
		}
		//Przywracanie poprzedniego stanu kont
		else if(MacroExecuted)
		{
		  //Wylaczenie timera zmiany stanu kont
		  KillTimer(hTimerFrm,TIMER_SETSTATUS);
		  //Odznaczenie wykonania makra
		  MacroExecuted = false;
		  //Ustawienie poprzedniego stanu kont
		  SetStatus(LastState,LastStatus);
		}
	  }
	}
	//Timer ustawiania nowego stanu
	else if(wParam==TIMER_SETSTATUS)
	{
      //Wylaczenie timera
	  KillTimer(hTimerFrm,TIMER_SETSTATUS);
	  //Pobieranie aktualnego stanu konta glownego
	  LastState = GetState();
	  LastStatus = GetStatus();
	  //Ustawienie nowego stanu kont
	  if(!Status.IsEmpty()) SetStatus(State,Status);
	  else SetStatus(State,LastStatus);
    }

	return 0;
  }

  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------

//Hook na zmiane kolorystyki AlphaControls
int __stdcall OnColorChange(WPARAM wParam, LPARAM lParam)
{
  //Okno ustawien zostalo juz stworzone
  if(hSettingsForm)
  {
	//Wlaczona zaawansowana stylizacja okien
	if(ChkSkinEnabled())
	{
	  hSettingsForm->sSkinManager->HueOffset = wParam;
	  hSettingsForm->sSkinManager->Saturation = lParam;
	}
  }

  return 0;
}
//---------------------------------------------------------------------------

//Hook na zmianę kompozycji
int __stdcall OnThemeChanged(WPARAM wParam, LPARAM lParam)
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

//Odczyt ustawien
void LoadSettings()
{
  TIniFile *Ini = new TIniFile(GetPluginUserDir()+"\\\\FullScrMacro\\\\Settings.ini");
  State = Ini->ReadInteger("Settings","State",5);
  Status = UTF8ToUnicodeString(IniStrToStr(Ini->ReadString("Settings","Status","").Trim().w_str()));
  DelayValue = Ini->ReadInteger("Settings","Delay",3);

  delete Ini;
}
//---------------------------------------------------------------------------

extern "C" int __declspec(dllexport) __stdcall Load(PPluginLink Link)
{
  //Linkowanie wtyczki z komunikatorem
  PluginLink = *Link;
  //Sciezka folderu prywatnego wtyczek
  UnicodeString PluginUserDir = GetPluginUserDir();
  //Tworzeniu katalogu z ustawieniami wtyczki
  if(!DirectoryExists(PluginUserDir+"\\\\FullScrMacro"))
   CreateDir(PluginUserDir+"\\\\FullScrMacro");
  //Tworzenie serwisu szybkiego dostepu do ustawien wtyczki
  PluginLink.CreateServiceFunction(L"sFullScrMacroFastSettingsItem",ServiceFullScrMacroFastSettingsItem);
  //Tworzenie interfejsu szybkiego dostepu do ustawien wtyczki
  BuildFullScrMacroFastSettings();
  //Hook na zmiane kolorystyki AlphaControls
  PluginLink.HookEvent(AQQ_SYSTEM_COLORCHANGE,OnColorChange);
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
  SetTimer(hTimerFrm,TIMER_CHKACTIVEWINDOW,500,(TIMERPROC)TimerFrmProc);

  return 0;
}
//---------------------------------------------------------------------------

extern "C" int __declspec(dllexport) __stdcall Unload()
{
  //Wyladowanie timerow
  for(int TimerID=10;TimerID<=20;TimerID=TimerID+10) KillTimer(hTimerFrm,TimerID);
  //Usuwanie okna timera
  DestroyWindow(hTimerFrm);
  //Wyrejestowanie klasy okna timera
  UnregisterClass(L"TFullScrMacro",HInstance);
  //Usuwanie interfejsu szybkiego dostepu do ustawien wtyczki
  DestroyFullScrMacroFastSettings();
  //Usuwanie serwisu szybkiego dostepu do ustawien wtyczki
  PluginLink.DestroyServiceFunction(ServiceFullScrMacroFastSettingsItem);
  //Wyladowanie wszystkich hookow
  PluginLink.UnhookEvent(OnColorChange);
  PluginLink.UnhookEvent(OnThemeChanged);

  return 0;
}
//---------------------------------------------------------------------------

//Ustawienia wtyczki
extern "C" int __declspec(dllexport)__stdcall Settings()
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
  PluginInfo.Version = PLUGIN_MAKE_VERSION(1,0,1,0);
  PluginInfo.Description = L"Wtyczka zmienia stan wszystkich kont, gdy aktywna jest aplikacja pełnoekranowa.";
  PluginInfo.Author = L"Krzysztof Grochocki (Beherit)";
  PluginInfo.AuthorMail = L"kontakt@beherit.pl";
  PluginInfo.Copyright = L"Krzysztof Grochocki (Beherit)";
  PluginInfo.Homepage = L"http://beherit.pl";
  PluginInfo.Flag = 0;
  PluginInfo.ReplaceDefaultModule = 0;

  return &PluginInfo;
}
//---------------------------------------------------------------------------
