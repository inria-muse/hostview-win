SetCompressor /SOLID /FINAL lzma

;--------------------------------
;Include Modern UI

  !include "MUI.nsh"
  !include "FileFunc.nsh"
  !include "LogicLib.nsh"
  !include nsDialogs.nsh


;--------------------------------
;General

; The name of the installer
Name "HostView"

; The file to write
OutFile "hostview_installer.exe"

RequestExecutionLevel admin

; These leave either "1" or "0" in $0.
Function is64bit
  System::Call "kernel32::GetCurrentProcess() i .s"
  System::Call "kernel32::IsWow64Process(i s, *i .r0)"
FunctionEnd

Function un.is64bit
  System::Call "kernel32::GetCurrentProcess() i .s"
  System::Call "kernel32::IsWow64Process(i s, *i .r0)"
FunctionEnd

Function isFirefoxInstalled
  ;HKEY_LOCAL_MACHINE\SOFTWARE\Mozilla\Mozilla Firefox
  ReadRegStr $R8 HKLM "SOFTWARE\Mozilla\Mozilla Firefox" "CurrentVersion"
  ReadRegStr $R9 HKLM "SOFTWARE\Mozilla\Mozilla Firefox\$R8\Main" "PathToExe"
FunctionEnd

VIProductVersion "1.0.0.1"
VIAddVersionKey /LANG=1033 "FileVersion" "1.0.0.1"
VIAddVersionKey /LANG=1033 "ProductName" "HostView"
VIAddVersionKey /LANG=1033 "FileDescription" "HostView Installer"
VIAddVersionKey /LANG=1033 "LegalCopyright" ""

;--------------------------------
; Windows API Definitions

!define SC_MANAGER_ALL_ACCESS           0x3F
!define SERVICE_ALL_ACCESS              0xF01FF

; Service Types
!define SERVICE_FILE_SYSTEM_DRIVER      0x00000002
!define SERVICE_KERNEL_DRIVER           0x00000001
!define SERVICE_WIN32_OWN_PROCESS       0x00000010
!define SERVICE_WIN32_SHARE_PROCESS     0x00000020
!define SERVICE_INTERACTIVE_PROCESS     0x00000100

; Service start options
!define SERVICE_AUTO_START              0x00000002
!define SERVICE_BOOT_START              0x00000000
!define SERVICE_DEMAND_START            0x00000003
!define SERVICE_DISABLED                0x00000004
!define SERVICE_SYSTEM_START            0x00000001

; Service Error control
!define SERVICE_ERROR_CRITICAL          0x00000003
!define SERVICE_ERROR_IGNORE            0x00000000
!define SERVICE_ERROR_NORMAL            0x00000001
!define SERVICE_ERROR_SEVERE            0x00000002

; Service Control Options
!define SERVICE_CONTROL_STOP            0x00000001
!define SERVICE_CONTROL_PAUSE           0x00000002



;--------------------------------
;Interface Settings
  !define MUI_HEADERIMAGE
  !define MUI_HEADERIMAGE_BITMAP "logo.bmp" ; optional
  !define MUI_ABORTWARNING

;--------------------------------
;Pages

!insertmacro MUI_PAGE_LICENSE "LICENSE"
; Don't let user choose where to install the files. WinPcap doesn't let people,
; and it's one less thing for us to worry about.
;Page custom optionsPage doOptions
; Page to get a users' password

Page custom ExtensionPageShow


Page custom PasswordPageShow PasswordPageLeave
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages

!insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Reserves

;--------------------------------
ReserveFile "options.ini"
!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS

!insertmacro GetParameters
!insertmacro GetOptions

; This function is called on startup. IfSilent checks
; if the flag /S was specified. If so, it sets the installer
; to run in "silent mode" which displays no windows and accepts
; all defaults.

; We also check if there is a previously installed winpcap
; on this system. If it's the same as the version we're installing,
; abort the install. If not, prompt the user about whether to
; replace it or not.

Function .onInit
  !insertmacro MUI_INSTALLOPTIONS_EXTRACT "options.ini"
  ; Always use the requested /D= $INSTDIR if given.
  StrCmp $INSTDIR "" "" instdir_nochange
  ; On 64-bit Windows, $PROGRAMFILES is "C:\Program Files (x86)" and
  ; $PROGRAMFILES64 is "C:\Program Files". We want "C:\Program Files"
  ; on 32-bit or 64-bit.
  StrCpy $INSTDIR "$PROGRAMFILES\HostView"
  Call is64bit
  StrCmp $0 "0" instdir_nochange
  StrCpy $INSTDIR "$PROGRAMFILES64\HostView"
  instdir_nochange:
FunctionEnd

;Function optionsPage
;  !insertmacro MUI_HEADER_TEXT "HostView Options" ""
;  !insertmacro MUI_INSTALLOPTIONS_DISPLAY "options.ini"
;FunctionEnd

;Function doOptions
;	Call is64bit
;	StrCmp $0 "0" install_32bit install_64bit
;
;install_32bit:
;	SetRegView 32
;	Goto install_continue
;install_64bit:
;	SetRegView 64
;
;install_continue:
;  ReadINIStr $0 "$PLUGINSDIR\options.ini" "Field 2" "State"
;  WriteRegStr HKLM "Software\HostView" "enduser" "$0"
;FunctionEnd


## Password is
!define Password ""
Var Dialog
Var Label
Var Pass
Function PasswordPageShow
	!insertmacro MUI_HEADER_TEXT "Password" "Setting up your password"

	nsDialogs::Create 1018
	Pop $Dialog

	${If} $Dialog == error
		Abort
	${EndIf}

	${NSD_CreateLabel} 0 0 90% 74u "Please choose a password that will let you retrieve your data from the online application.\
 $\nThe data we collect is anonymized (we process the data but CANNOT identify you from it and \
  CANNOT retrieve the data nor the password for you). \
  $\nThis password is the only way you (and ONLY YOU) can access and decide to visualize or delete your personal data. \
  $\nToghether with the machine you are using at the moment, this password will allow you to manage your data from this URL:"  

  ${NSD_CreateLink} 10 120 100% 12 "http://muse.inria.fr/ucnstudydemo/"
Var /GLOBAL LINK 
Pop $LINK 
${NSD_OnClick} $LINK onClickMyLink  

 Pop $Label
 
  ${NSD_CreatePassword} 0 150 100% 14u "Please type your desired password"  #if you change this message, change accordingly also the string check in the return function
	Pop $Pass # Page HWND
    SendMessage $Pass ${EM_SETPASSWORDCHAR} 0 0
 
 nsDialogs::Show

FunctionEnd

## Validate password
Function PasswordPageLeave
  ${NSD_GetText} $Pass $R0 ##todo check what we need to store, $Pass or the text version

  ${If} $R0 ==  "Please type your desired password"  
  ${OrIf} $R0 ==  ""
    MessageBox MB_OK|MB_ICONEXCLAMATION "You need to input a password"
    Abort
  ${Else}
    MessageBox MB_OK|MB_ICONINFORMATION "Password correctly stored"
  ${EndIf}
FunctionEnd

Function onClickMyLink
	Pop $0 ; don't forget to pop HWND of the stack
	ExecShell "open" "http://muse.inria.fr/ucnstudydemo/"
FunctionEnd


Function onClickExtensionLink
	Pop $0 ; don't forget to pop HWND of the stack
	ExecShell "open" "https://project.inria.fr/bottlenet/"
FunctionEnd

Function ExtensionPageShow
	!insertmacro MUI_HEADER_TEXT "Browser extension" "Installing additional plugin for your browser"


	nsDialogs::Create 1018
	Pop $Dialog

	${If} $Dialog == error
		Abort
	${EndIf}

	${NSD_CreateLabel} 0 0 90% 74u "Please, if you use Chrome or Firefox as browser, install this addditional plugin. $\n\
	It won't require more than one click.$\n\
	This browser extension will help us getting some information about the quality of the communication when you are watching videos from \
	Youtube, Netflix, etc. TODO more info??"  

	${NSD_CreateLink} 10 120 100% 12 "Browser extension plugin"
	Var /GLOBAL EXTENSION_LINK 
	Pop $EXTENSION_LINK 
	${NSD_OnClick} $EXTENSION_LINK onClickExtensionLink  

	Pop $Label
	nsDialogs::Show
  
  FunctionEnd

;Function ComponentsPageShow
;
;  ## Disable the Back button
;  GetDlgItem $R0 $HWNDPARENT 3
;  EnableWindow $R0 0
;
;FunctionEnd

## Just a dummy section. Do I need this?
;Section 'A section'
;SectionEnd

;--------------------------------
; The stuff to install
Section "WinPcap" SecWinPcap

	SetAutoClose true

	SetRegView 32
	WriteRegDWORD HKLM "Software\Microsoft\Internet Explorer\MAIN\FeatureControl\FEATURE_BROWSER_EMULATION" "HostView.exe" 10001
	SetRegView 64
	WriteRegDWORD HKLM "Software\Microsoft\Internet Explorer\MAIN\FeatureControl\FEATURE_BROWSER_EMULATION" "HostView.exe" 10001

	Call is64bit
	StrCmp $0 "0" install_32bit install_64bit

install_32bit:
	SetOutPath $INSTDIR
	File "..\bin\Win32\Release\HostView.exe"
	File "..\bin\Win32\Release\hostviewcli.exe"
	File "..\bin\Win32\Release\pcap.dll"
	File "..\bin\Win32\Release\proc.dll"
	File "..\bin\Win32\Release\store.dll"
	SetRegView 32
	Goto npfdone

install_64bit:
	SetOutPath $INSTDIR
	File "..\bin\x64\Release\HostView.exe"
	File "..\bin\x64\Release\hostviewcli.exe"
	File "..\bin\x64\Release\pcap.dll"
	File "..\bin\x64\Release\proc.dll"
	File "..\bin\x64\Release\store.dll"
	SetRegView 64

npfdone:
	; install winpcap if case
    IfFileExists "$SYSDIR\wpcap.dll" WinPcapExists WinPcapDoesNotExist

	WinPcapDoesNotExist:
	WriteRegStr HKLM "Software\HostView" "InstalledWinpcap" "Yes"
	SetOutPath $TEMP
	File ".\winpcap_bundle\winpcap-4.13.exe"
	nsExec::Exec "$TEMP\winpcap-4.13.exe /S /NPFSTARTUP=YES"

	WinPcapExists:
	; do nothing else

	SetOutPath $INSTDIR
	File "7za.exe"

	SetOutPath "$INSTDIR\html\"
	File /r "..\src\HostView\html\*"

	nsExec::Exec "$INSTDIR\hostviewcli.exe /install"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "HostView" "$\"$INSTDIR\HostView.exe$\""

	; write uninstaller
	WriteUninstaller "$INSTDIR\uninstall.exe"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\HostView" \
					"DisplayName" "HostView"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\HostView" \
					"DisplayIcon" "$\"$INSTDIR\uninstall.exe$\""
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\HostView" \
					"Publisher" "Muse, INRIA"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\HostView" \
					"UninstallString" "$\"$INSTDIR\uninstall.exe$\""

	; write settings
	SetOutPath $INSTDIR
	File "settings"

  ; write certificate
  File "ca-bundle.crt"

  ; Use previously given password to store for identification
  ; (hased from input)
  Pop $R0
  Crypto::HashData "SHA2" $R0
  Pop $0
  FileOpen $4 "$INSTDIR\pass" w
  FileWrite $4 $0
  FileClose $4

  ; create a random value and save it to a file at the destination
  ; (used as a hash salt)
  Crypto::RNG
  Pop $0 ; $0 now contains 100 bytes of random data in hex format
  FileOpen $4 "$INSTDIR\salt" w
  FileWrite $4 $0
  FileClose $4

  ; TODO move this to a final page that starts the app once the installation looks complete
  ; TODO re-activate the explorer extension
	SetOutPath $INSTDIR
	File "..\bin\Win32\Release\hostviewbho.dll"
	Exec "regsvr32 /s $\"$INSTDIR\hostviewbho.dll$\""

	; start everything
	nsExec::Exec "net start $\"HostView Service$\""
	Exec "$\"$INSTDIR\HostView.exe$\""

SectionEnd ; end the section

;--------------------------------
;Uninstaller Section

Section "Uninstall"

	Exec "regsvr32 /s /u $\"$INSTDIR\hostviewbho.dll$\""
	nsExec::Exec "$INSTDIR\HostView.exe /stop"
	nsExec::Exec "net stop $\"HostView Service$\""
	nsExec::Exec "$INSTDIR\hostviewcli.exe /remove"

	; delete from both hives
	SetRegView 32
	DeleteRegValue HKLM "Software\Microsoft\Internet Explorer\MAIN\FeatureControl\FEATURE_BROWSER_EMULATION" "HostView.exe"
	SetRegView 64
	DeleteRegValue HKLM "Software\Microsoft\Internet Explorer\MAIN\FeatureControl\FEATURE_BROWSER_EMULATION" "HostView.exe"

	Call un.is64bit
	StrCmp $0 "0" rmEmulation32 rmEmulation64

rmEmulation64:
	SetRegView 64
	Goto emulationContinue

rmEmulation32:
	SetRegView 32

emulationContinue:
	DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "HostView"

	RMDir /r "$INSTDIR\"

	ReadRegStr $0 HKLM "Software\HostView" "InstalledWinpcap"
	${If} $0 == "Yes"
		nsExec::Exec "$PROGRAMFILES64\winpcap\uninstall.exe /S"
		nsExec::Exec "$PROGRAMFILES\winpcap\uninstall.exe /S"
	${EndIf}

;	DeleteRegValue HKLM "Software\HostView" "enduser"
	DeleteRegValue HKLM "Software\HostView" "InstalledWinpcap"
	DeleteRegValue HKLM "Software\HostView" "HostView"
	DeleteRegKey HKLM "Software\HostView"

	; remove uninstall keys
	DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\HostView" "DisplayName"
	DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\HostView" "DisplayIcon"
	DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\HostView" "Publisher"
	DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\HostView" "UninstallString"
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\HostView"

SectionEnd
