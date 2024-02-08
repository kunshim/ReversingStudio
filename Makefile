CXX := clang-cl
LD := lld-link
CXXFLAGS := /wd4530 /DTARGET_WINDOWS /MT /O2 /nologo /WX- /FIinclude/msvc_compat.h /GR- /GS- /EHs- /EHa- /Oi- -Wno-non-c-typedef-for-linkage -Wno-microsoft-include -Wno-unicode -Wint-to-pointer-cast
MACROS := /D "PIN_CRT=1" /D "TARGET_WINDOWS" /D "_LIBCPP_DISABLE_AVAILABILITY" /D "_LIBCPP_NO_VCRUNTIME" /D "__BIONIC__" /D "_WINDOWS_H_PATH_=C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um"
X86FLAGS := /D "TARGET_IA32" /D "__i386__" /D "HOST_IA32"
PIN_ROOT = Pin
X86INCLUDES := /I$(PIN_ROOT)/source/include/pin \
			/I$(PIN_ROOT)/source/include/pin/gen \
			/I$(PIN_ROOT)/extras/cxx/include \
			/I$(PIN_ROOT)/extras \
			/I$(PIN_ROOT)/extras/crt/include \
			/I$(PIN_ROOT)/extras/crt \
			/I$(PIN_ROOT)/extras/crt/include/arch-x86 \
			/I$(PIN_ROOT)/extras/crt/include/kernel/uapi \
			/I$(PIN_ROOT)/extras/crt/include/kernel/uapi/asm-x86 \
			/I$(PIN_ROOT)/extras/components/include \
			/I$(PIN_ROOT)/extras/xed-ia32/include/xed


X86LDFLAGS := /safeseh:no /DEBUG /LTCG /DLL /EXPORT:main  /NODEFAULTLIB /NOLOGO /INCREMENTAL:NO /IGNORE:4210 /IGNORE:4049 /DYNAMICBASE /NXCOMPAT /MACHINE:x86 /ENTRY:Ptrace_DllMainCRTStartup@12 /OPT:REF Pin/ia32/runtime/pincrt/crtbeginS.obj
X86LDINCLUDE := /LIBPATH:Pin/ia32/lib \
				/LIBPATH:Pin/ia32/runtime/pincrt \
				/LIBPATH:Pin/extras/xed-ia32/lib 
X86LDLIB := pin.lib xed.lib pinipc.lib pincrt.lib kernel32.lib c++.lib

X64INCLUDES := /I$(PIN_ROOT)/source/include/pin \
			/I$(PIN_ROOT)/source/include/pin/gen \
			/I$(PIN_ROOT)/extras/cxx/include \
			/I$(PIN_ROOT)/extras \
			/I$(PIN_ROOT)/extras/crt/include \
			/I$(PIN_ROOT)/extras/crt \
			/I$(PIN_ROOT)/extras/crt/include/arch-x86_64 \
			/I$(PIN_ROOT)/extras/crt/include/kernel/uapi \
			/I$(PIN_ROOT)/extras/crt/include/kernel/uapi/asm-x86 \
			/I$(PIN_ROOT)/extras/components/include \
			/I$(PIN_ROOT)/extras/xed-intel64/include/xed

X64LDFLAGS := /safeseh:no /DEBUG /LTCG /DLL /EXPORT:main  /NODEFAULTLIB /NOLOGO /INCREMENTAL:NO /IGNORE:4210 /IGNORE:4049 /DYNAMICBASE /NXCOMPAT /MACHINE:x64 /ENTRY:Ptrace_DllMainCRTStartup /OPT:REF Pin/intel64/runtime/pincrt/crtbeginS.obj
X64FLAGS := /DTARGET_IA32E -m64 /D__LP64__ /DHOST_IA32E
X64LDINCLUDE := /LIBPATH:Pin/intel64/lib \
				/LIBPATH:Pin/intel64/runtime/pincrt \
				/LIBPATH:Pin/extras/xed-intel64/lib \
				/LIBPATH:C:/Users/kunsh/Downloads/boost_1_83_0/stage/lib
X64LDLIB = $(X86LDLIB)

JOBS ?= 4
SOURCE_DIR = ./

ifndef arch
arch = x86
endif

ifeq ($(arch), x64)
CXXFLAGS += $(X64FLAGS) $(X64INCLUDES) $(MACROS)
LDFLAGS := $(X64LDFLAGS)
INCLUDE := $(X64LDINCLUDE) $(X64LDLIB)
else
CXXFLAGS += $(X86FLAGS) $(X86INCLUDES) $(MACROS)
LDFLAGS := $(X86LDFLAGS)
INCLUDE := $(X86LDINCLUDE) $(X86LDLIB)
endif

OBJ_DIR := obj-$(arch)
CXX_SOURCES = image.cpp call.cpp calltrack.cpp
OBJECTS = $(patsubst %.cpp, $(OBJ_DIR)/%.obj, $(CXX_SOURCES))

.PHONY: all clean

all: wndproc unique
	@echo "All build complete!"

$(OBJ_DIR)/%.obj: %.cpp | create_dir
	@echo "Compiling $<"
	$(CXX) $(CXXFLAGS) -c -o $@ $<

wndproc: obj-$(arch)/wndproc.obj $(OBJECTS) 
	@echo "Link $@.dll with $(strip $(patsubst $(OBJ_DIR)/%, % , $^))"
	$(LD) $(LDFLAGS) /out:$(OBJ_DIR)/$@.dll $< $(OBJECTS) $(INCLUDE)
	

unique: obj-$(arch)/unique.obj $(OBJECTS) 
	@echo "Link $@.dll with $(strip $(patsubst $(OBJ_DIR)/%, % , $^))"
	@$(LD) $(LDFLAGS) /out:$(OBJ_DIR)/$@.dll $< $(OBJECTS) $(INCLUDE)

taint: obj-$(arch)/taint.obj $(OBJECTS) 
	@echo "Link $@.dll with $(strip $(patsubst $(OBJ_DIR)/%, % , $^))"
	@$(LD) $(LDFLAGS) /out:$(OBJ_DIR)/$@.dll $< $(OBJECTS) $(INCLUDE)

create_dir:
	@mkdir $(OBJ_DIR)

clean:
	@rm $(OBJ_DIR)/*.*
	@rmdir $(OBJ_DIR)
	@echo "clean up!"

