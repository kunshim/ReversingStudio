# ReversingStudio
Reversing tool based on intel pintool. It helps track events that occur when you click a mouse or type a keyboard, and data flow analysis is possible.

## Build instructions
### Dependencies
* Make for windows
* cygwin
* Visual Studio 2022

1. Add cygwin64\bin to PATH : `set PATH=%PATH%;\cygwin64\bin`
2. Download latest pintool and move to `PROJECTS_DIR\Pin`
3. Type follow command on cmd

```bash
# Must use cmd. Powershell can't find cl and link. After run cmd, you can use powershell
# You may need to set environmental variables. Check out the Visual Studio folder.
# For x64 applications, use vcvasr64 instead. 
vcvars32 
make -j4
```
