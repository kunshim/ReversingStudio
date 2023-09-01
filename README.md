# ReversingStudio
Reversing tool based on intel pintool. It helps track events that occur when you click a mouse or type a keyboard, and data flow analysis is possible.

## Build instructions
### Dependencies
* Make for windows
* cygwin
* Visual Studio 2022
* pintool 3.28
* Intel cpu for best condition

1. Add cygwin64\bin to PATH : `set PATH=%PATH%;\cygwin64\bin`
2. Download latest pintool and move to `PROJECTS_DIR\Pin`
3. Type follow command on cmd

```powershell
# Must use cmd. Powershell can't find cl and link. After run cmd, you can use powershell
# You may need to set environmental variables. Check out the Visual Studio folder.
# For x64 applications, use vcvasr64 instead. 
vcvars32
make ARCH=x86 #Use ARCH=x64 for 64bit application
```
## Features
### WndProc finder
WndProc finder searches for all `WndProc` that exist in your program. 
It search for functions whose second argument is called with `WM_COMMAND`


```
                                      Address                         Disassemble                               Target Address     Count
             6ba5414e (win32dialog.dll!.text)                            call eax                    66a0c0 (******.exe!.text)         2
             6ba70da2 (win32dialog.dll!.text)             jmp dword ptr [eax+0x8]             6ba563d0 (win32dialog.dll!.text)         2
           763823b1 (user32.dll!Ordinal_2713)                            call ecx             6ba567b0 (win32dialog.dll!.text)         2
```
Like above result, you can see the functions that are assumed to be `WndProc` with some additional information.

### Call tracker
Tracks all calls and branches that occur within a specific function. 
You can keep track of the `WndProc` functions you find with the WndProc Finder.
Call Tracker also anlyze abi of the function and predict argument.
Call Tracker also creates coverage files that can be used by **IDA Pro LightHouse** plugins. 
```
Tracking analysis of 6b1f67b0
Branch : 6b1f68fc (win32dialog.dll!6b1f6874)
Branch : 6b1f697c (win32dialog.dll!6b1f68fc)
Branch : 6b1f69e2 (win32dialog.dll!6b1f697c)

Call : __stdcall 758bd890 (user32.dll!GetWindowLongW)
Parent : 6b1f67b0 (win32dialog.dll!6b1f67b0)

Call : __stdcall 758d1b80 (user32.dll!GetDlgCtrlID)
Parent : 6b1f67b0 (win32dialog.dll!6b1f67b0)

Call : __thiscall 6b1f44c0 (win32dialog.dll!6b1f44c0)
Parent : 6b1f67b0 (win32dialog.dll!6b1f67b0)
Branch : 6b1f4508 (win32dialog.dll!6b1f44c0)
Branch : 6b1f454f (win32dialog.dll!6b1f4508)
Branch : 6b1f487d (win32dialog.dll!6b1f454f)

Call : __stdcall a19cf0 (nine_kokoiro.exe!a19cf0)
Parent : 6b1f44c0 (win32dialog.dll!6b1f44c0)
...

Call : __thiscall ad4dd0 (nine_kokoiro.exe!ad4dd0)
Parent : a1a250 (nine_kokoiro.exe!a19cf0)
Branch : ad4e43 (nine_kokoiro.exe!ad4dd0)
Tracking analysis of 6b1f67b0 end
                                      Address    Count
                      nine_kokoiro.exe!ac3242         1
                      nine_kokoiro.exe!ac3050         1
                    user32.dll!GetWindowLongW         1
                      nine_kokoiro.exe!ac1c90         1
                      nine_kokoiro.exe!ac4110         1
                      user32.dll!GetDlgCtrlID         1
                      nine_kokoiro.exe!a19cf0         1
                      nine_kokoiro.exe!a19cf0         1
                      nine_kokoiro.exe!ad4dd0         1
                     win32dialog.dll!6b1f3f70         1
                     win32dialog.dll!6b1f44c0         1

```

### Data-flow analyzer
**not yet**
Powerfull tool for data flow analyze and backward analyze. 
dsd