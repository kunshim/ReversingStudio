CXX := cl
LD := link
CXXFLAGS := /utf-8 /EHs- /EHa- /wd4530 /DTARGET_WINDOWS /MD /O2 /nologo /Gy /Oi- /GR- /GS- /DPIN_CRT=1 /Zc:threadSafeInit- /Zc:sizedDealloc- /wd5208 /FIinclude/msvc_compat.h \
			/D_WINDOWS_H_PATH_="C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um"
X86FLAGS := /D__i386__ /DTARGET_IA32 /DHOST_IA32
X86INCLUDES := -I"Pin/source/include/pin" \
			-I"Pin/source/include/pin/gen" \
			-I"Pin/extras/stlport/include" \
			-I"Pin/extras" \
			-I"Pin/extras/crt/include/kernel/uapi" \
			-I"extras/libstdc++/include" \
			-I"Pin/source/tools/InstLib" \
			-I"Pin/source/tools/Utils" \
			-I"Pin/extras/xed-ia32/include/xed" \
			-I"Pin/extras/components/include" \
			-I"Pin/extras/crt/include/kernel/uapi/asm-x86" \
			-I"Pin/extras/crt/include/arch-x86" \
			-I"Pin/extras/crt" \
			-I"Pin/extras/crt/include" \
			-I"Pin/extras/stlport/include/stl" \
			-I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\shared" \
			-I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um"
X86LDFLAGS := /DLL /EXPORT:main  /NODEFAULTLIB /NOLOGO /INCREMENTAL:NO /IGNORE:4210 /IGNORE:4049 /DYNAMICBASE /NXCOMPAT /MACHINE:x86 /ENTRY:Ptrace_DllMainCRTStartup@12 /OPT:REF Pin/ia32/runtime/pincrt/crtbeginS.obj
X86LDINCLUDE :=  /LIBPATH:Pin/ia32/lib \
				/LIBPATH:Pin/ia32/lib \
				/LIBPATH:Pin/ia32/runtime/pincrt \
				/LIBPATH:Pin/extras/xed-ia32/lib 
X86LDLIB := pin.lib xed.lib pinipc.lib pincrt.lib kernel32.lib

X64INCLUDES := -I"Pin/extras/crt/include/arch-x86_64"
X64FLAGS := /DTARGET_IA32E /DHOST_IA32E /D__LP64__
JOBS ?= 4
SOURCE_DIR = ./

ifndef arch
arch = x86
endif

ifeq ($(arch), x64)
CXXFLAGS += $(X64FLAGS) $(X86INCLUDES)
else
CXXFLAGS += $(X86FLAGS) $(X86INCLUDES)
LDFLAGS := $(X86LDFLAGS)
INCLUDE := $(X86LDINCLUDE) $(X86LDLIB)
endif

OBJ_DIR := obj-$(arch)
CXX_SOURCES = image.cpp call.cpp calltrack.cpp
OBJECTS = $(patsubst %.cpp, $(OBJ_DIR)/%.obj, $(CXX_SOURCES))

.PHONY: all clean

all: 
	@make -j$(JOBS) wndproc unique
	@echo "All build complete!"

$(OBJ_DIR)/%.obj: %.cpp | create_dir
	@echo "Compiling $<"
	@$(CXX) $(CXXFLAGS) /c /Fo$@ $<

wndproc: obj-x86/wndproc.obj $(OBJECTS) 
	@echo "Link $@.dll with $(strip $(patsubst $(OBJ_DIR)/%, % , $^))"
	@$(LD) $(LDFLAGS) /out:$(OBJ_DIR)/$@.dll $< $(OBJECTS) $(INCLUDE)
	

unique: obj-x86/unique.obj $(OBJECTS) 
	@echo "Link $@.dll with $(strip $(patsubst $(OBJ_DIR)/%, % , $^))"
	@$(LD) $(LDFLAGS) /out:$(OBJ_DIR)/$@.dll $< $(OBJECTS) $(INCLUDE)

create_dir:
	@mkdir -p $(OBJ_DIR)

clean:
	@rm $(OBJ_DIR)/*.*
	@rmdir $(OBJ_DIR)
	@echo "clean up!"

