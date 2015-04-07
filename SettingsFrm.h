//---------------------------------------------------------------------------
// Copyright (C) 2013-2015 Krzysztof Grochocki
//
// This file is part of FullScrMacro
//
// FullScrMacro is free software: you can redistribute it and/or modify
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
// along with GNU Radio. If not, see <http://www.gnu.org/licenses/>.
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#ifndef SettingsFrmH
#define SettingsFrmH
#define WM_ALPHAWINDOWS (WM_USER + 666)
//---------------------------------------------------------------------------
#include "acAlphaImageList.hpp"
#include "sBevel.hpp"
#include "sButton.hpp"
#include "sComboBox.hpp"
#include "sEdit.hpp"
#include "sMemo.hpp"
#include "sSkinManager.hpp"
#include "sSkinProvider.hpp"
#include "sSpinEdit.hpp"
#include <System.Actions.hpp>
#include <System.Classes.hpp>
#include <Vcl.ActnList.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.ImgList.hpp>
#include <Vcl.StdCtrls.hpp>
//---------------------------------------------------------------------------
class TSettingsForm : public TForm
{
__published:	// IDE-managed Components
	TsBevel *Bevel;
	TsButton *CancelButton;
	TsButton *OKButton;
	TActionList *ActionList;
	TAction *aExit;
	TsSkinManager *sSkinManager;
	TsSkinProvider *sSkinProvider;
	TsComboBox *StateComboBox;
	TsAlphaImageList *sAlphaImageList;
	TsMemo *StatusMemo;
	TsSpinEdit *sSpinEdit;
	TAction *aSelectMemo;
	TAction *aLoadSettings;
	TAction *aSaveSettings;
	void __fastcall aExitExecute(TObject *Sender);
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall FormShow(TObject *Sender);
	void __fastcall StateComboBoxDrawItem(TWinControl *Control, int Index, TRect &Rect,
          TOwnerDrawState State);
	void __fastcall aSelectMemoExecute(TObject *Sender);
	void __fastcall aLoadSettingsExecute(TObject *Sender);
	void __fastcall aSaveSettingsExecute(TObject *Sender);
	void __fastcall OKButtonClick(TObject *Sender);
	void __fastcall sSkinManagerSysDlgInit(TacSysDlgData DlgData, bool &AllowSkinning);
private:	// User declarations
public:		// User declarations
	__fastcall TSettingsForm(TComponent* Owner);
	void __fastcall WMTransparency(TMessage &Message);
	BEGIN_MESSAGE_MAP
	MESSAGE_HANDLER(WM_ALPHAWINDOWS,TMessage,WMTransparency);
	END_MESSAGE_MAP(TForm)
};
//---------------------------------------------------------------------------
extern PACKAGE TSettingsForm *SettingsForm;
//---------------------------------------------------------------------------
#endif
