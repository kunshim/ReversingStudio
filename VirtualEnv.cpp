#include "VirtualEnv.h"

TaintEnv::TaintEnv()
{
	memset(regTaintTable, 0, sizeof(regTaintTable));
	memset(regTaintTable, 0, sizeof(regTaintTable));
	memset(xmmTaintTable, 0, sizeof(xmmTaintTable));
	memset(ymmTaintTable, 0, sizeof(ymmTaintTable));
	memset(zmmTaintTable, 0, sizeof(zmmTaintTable));	
	memset(regRelateTable, 0, sizeof(regRelateTable));
	
	setRelate(REG_AL, REG_AX, REG_EAX, REG_GAX);
	setRelate(REG_AH, REG_AX, REG_EAX, REG_GAX);
	setRelate(REG_AX, REG_AL, REG_AH, REG_EAX, REG_GAX);
	setRelate(REG_EAX, REG_AL, REG_AH, REG_AX, REG_GAX);
	setRelate(REG_GAX, REG_AL, REG_AH, REG_AX, REG_EAX);
	
	setRelate(REG_BL, REG_BX, REG_EBX, REG_GBX);
	setRelate(REG_BH, REG_BX, REG_EBX, REG_GBX);
	setRelate(REG_BX, REG_BL, REG_BH, REG_EBX, REG_GBX);
	setRelate(REG_EBX, REG_BL, REG_BH, REG_BX, REG_GBX);
	setRelate(REG_GBX, REG_BL, REG_BH, REG_BX, REG_EBX);
	
	setRelate(REG_CL, REG_CX, REG_ECX, REG_GCX);
	setRelate(REG_CH, REG_CX, REG_ECX, REG_GCX);
	setRelate(REG_CX, REG_CL, REG_CH, REG_ECX, REG_GCX);
	setRelate(REG_ECX, REG_CL, REG_CH, REG_CX, REG_GCX);
	setRelate(REG_GCX, REG_CL, REG_CH, REG_CX, REG_ECX);
	
	setRelate(REG_DL, REG_DX, REG_EDX, REG_GDX);
	setRelate(REG_DH, REG_DX, REG_EDX, REG_GDX);
	setRelate(REG_DX, REG_DL, REG_DH, REG_EDX, REG_GDX);
	setRelate(REG_EDX, REG_DL, REG_DH, REG_DX, REG_GDX);
	setRelate(REG_GDX, REG_DL, REG_DH, REG_DX, REG_EDX);

	setRelate(REG_DI, REG_EDI, REG_GDI);
	setRelate(REG_EDI, REG_DI, REG_GDI);
	setRelate(REG_GDI, REG_DI, REG_EDI);

	setRelate(REG_SI, REG_ESI, REG_GSI);
	setRelate(REG_ESI, REG_SI, REG_GSI);
	setRelate(REG_GSI, REG_SI, REG_ESI);	
	
	setRelate(REG_SP, REG_ESP, REG_STACK_PTR);
	setRelate(REG_ESP, REG_SP, REG_STACK_PTR);
	setRelate(REG_STACK_PTR, REG_SP, REG_ESP);

	setRelate(REG_BP, REG_EBP, REG_GBP);
	setRelate(REG_EBP, REG_BP, REG_GBP);
	setRelate(REG_GBP, REG_BP, REG_EBP);

#ifdef TARGET_IA32e
	setRelate(REG_R8B, REG_R8W, REG_R8D, REG_R8);
	setRelate(REG_R8W, REG_R8D, REG_R8D, REG_R8);
	setRelate(REG_R8D, REG_R8W, REG_R8B, REG_R8);
	setRelate(REG_R8, REG_R8W, REG_R8B, REG_R8D);

	setRelate(REG_R9B, REG_R9W, REG_R9D, REG_R9);
	setRelate(REG_R9W, REG_R9D, REG_R9D, REG_R9);
	setRelate(REG_R9D, REG_R9W, REG_R9B, REG_R9);
	setRelate(REG_R9, REG_R9W, REG_R9B, REG_R9D);

	setRelate(REG_R10B, REG_R10W, REG_R10D, REG_R10);
	setRelate(REG_R10W, REG_R10D, REG_R10D, REG_R10);
	setRelate(REG_R10D, REG_R10W, REG_R10B, REG_R10);
	setRelate(REG_R10, REG_R10W, REG_R10B, REG_R10D);

	setRelate(REG_R11B, REG_R11W, REG_R11D, REG_R11);
	setRelate(REG_R11W, REG_R11D, REG_R11D, REG_R11);
	setRelate(REG_R11D, REG_R11W, REG_R11B, REG_R11);
	setRelate(REG_R11, REG_R11W, REG_R11B, REG_R11D);

	setRelate(REG_R12B, REG_R12W, REG_R12D, REG_R12);
	setRelate(REG_R12W, REG_R12D, REG_R12D, REG_R12);
	setRelate(REG_R12D, REG_R12W, REG_R12B, REG_R12);
	setRelate(REG_R12, REG_R12W, REG_R12B, REG_R12D);

	setRelate(REG_R13B, REG_R13W, REG_R13D, REG_R13);
	setRelate(REG_R13W, REG_R13D, REG_R13D, REG_R13);
	setRelate(REG_R13D, REG_R13W, REG_R13B, REG_R13);
	setRelate(REG_R13, REG_R13W, REG_R13B, REG_R13D);

	setRelate(REG_R14B, REG_R14W, REG_R14D, REG_R14);
	setRelate(REG_R14W, REG_R14D, REG_R14D, REG_R14);
	setRelate(REG_R14D, REG_R14W, REG_R14B, REG_R14);
	setRelate(REG_R14, REG_R14W, REG_R14B, REG_R14D);

	setRelate(REG_R15B, REG_R15W, REG_R15D, REG_R15);
	setRelate(REG_R15W, REG_R15D, REG_R15D, REG_R15);
	setRelate(REG_R15D, REG_R15W, REG_R15B, REG_R15);
	setRelate(REG_R15, REG_R15W, REG_R15B, REG_R15D);
#endif
}

void TaintEnv::setRelate(int num, ...)
{
	va_list ap;
	va_start(ap, num);
	REG base = va_arg(ap, REG);	
	for (int i = 1; i < num; i++)
	{
		regRelateTable[base][i - 1] = va_arg(ap, REG);
	}
	va_end(ap);
}

bool TaintEnv::memoryOperandIsTainted(INS& ins, ADDRINT ea, size_t size) 
{
	return (isTainted(ea, size) || isTainted(INS_MemoryBaseReg(ins)) || isTainted(INS_MemoryIndexReg(ins)));
}
void TaintEnv::taintMem(ADDRINT addr, size_t size)
{
	std::cout << "[WRITE] " << hexstr(addr) << "("  << std::hex << size << ")" << " tainted\n";
	if (isMemoryCommitted(addr, size))
	{
		MASKING:
		if (ALIGN(addr + size) == ALIGN(addr))
		{
			uint8_t* map = committedMemory[ALIGN(addr)];
			addr &= 0xFFF;
			for (int i = 0; i < size; i++, addr++)
			{
				int idx = addr / 8;
				int target = addr % 8;
				uint8_t mask = map[idx];
				mask |= 1 << target;
				map[idx] = mask;
			}
		}
		else
		{
			uint8_t* map1 = committedMemory[ALIGN(addr)];
			uint8_t* map2 = committedMemory[ALIGN(addr + size)];
			int extra = addr + size - ALIGN(addr + size);
			addr &= 0xFFF;
			for (int i = addr; i < 0x1000; i++)
			{
				int idx = i / 8;
				int target = i % 8;
				uint8_t mask = map1[idx];
				mask |= 1 << target;
				map1[idx] = mask;
			}
			for (int i = 0; i < extra; i++)
			{
				int idx = i / 8;
				int target = i % 8;
				uint8_t mask = map2[idx];
				mask |= 1 << target;
				map2[idx] = mask;
			}
		}
	}
	else
	{
		if (ALIGN(addr + size) == ALIGN(addr))
			committedMemory[ALIGN(addr)] = new uint8_t[0x1000];
		else
		{
			committedMemory[ALIGN(addr)] = new uint8_t[0x1000];
			committedMemory[ALIGN(addr + size)] = new uint8_t[0x1000];
		}
		goto MASKING;
	}
}

void TaintEnv::releaseMem(ADDRINT addr, size_t size)
{
	if (!isTainted(addr, size))
		return;
	std::cout << hexstr(addr) << " ~ " << hexstr(addr + size) << " released\n";
	if (isMemoryCommitted(addr, size))
	{
		if (ALIGN(addr + size) == ALIGN(addr))
		{
			uint8_t* map = committedMemory[ALIGN(addr)];
			addr &= 0xFFF;
			for (int i = 0; i < size; i++, addr++)
			{
				int idx = addr / 8;
				int target = addr % 8;
				uint8_t mask = map[idx];
				mask ^= 1 << target; //이미 특정 비트가 활성화 되어있기 때문에 xor 연산을 통해서 다시 0으로 만들어줄 수 있다.
				map[idx] = mask;
			}
		}
		else
		{
			uint8_t* map1 = committedMemory[ALIGN(addr)];
			uint8_t* map2 = committedMemory[ALIGN(addr + size)];
			int extra = addr + size - ALIGN(addr + size);
			addr &= 0xFFF;
			for (int i = addr; i < 0x1000; i++)
			{
				int idx = i / 8;
				int target = i % 8;
				uint8_t mask = map1[idx];
				mask ^= 1 << target;
				map1[idx] = mask;
			}
			for (int i = 0; i < extra; i++)
			{
				int idx = i / 8;
				int target = i % 8;
				uint8_t mask = map2[idx];
				mask ^= 1 << target;
				map2[idx] = mask;
			}
		}
	}
}
bool TaintEnv::isTainted(REG reg, int idx, size_t size) const
{
	if (reg >= REG_XMM_BASE && reg <= REG_XMM_LAST)
	{
		switch (size)
		{
		case 1:
			return xmmTaintTable[reg - REG_XMM_BASE][idx * size];			
		case 2:
			return *reinterpret_cast <uint16_t*>(xmmTaintTable[reg - REG_XMM_BASE][idx * size]);			
		case 4:
			return *reinterpret_cast <uint32_t*>(xmmTaintTable[reg - REG_XMM_BASE][idx * size]);
			
		case 8:
			return *reinterpret_cast <uint64_t*>(xmmTaintTable[reg - REG_XMM_BASE][idx * size]);			
		}
	}
	else if (reg >= REG_YMM_BASE && reg <= REG_YMM_LAST)
	{
		switch (size)
		{
		case 1:
			return ymmTaintTable[reg - REG_YMM_BASE][idx * size];			
		case 2:
			return *reinterpret_cast <uint16_t*>(ymmTaintTable[reg - REG_YMM_BASE][idx * size]);
		case 4:
			return *reinterpret_cast <uint32_t*>(ymmTaintTable[reg - REG_YMM_BASE][idx * size]);
		case 8:
			return *reinterpret_cast <uint64_t*>(ymmTaintTable[reg - REG_YMM_BASE][idx * size]);			
		}
	}
	else if (reg >= REG_ZMM_BASE && reg <= REG_ZMM_LAST)
	{
		switch (size)
		{
		case 1:
			return zmmTaintTable[reg - REG_ZMM_BASE][idx * size];
		case 2:
			return *reinterpret_cast <uint16_t*>(zmmTaintTable[reg - REG_ZMM_BASE][idx * size]);
		case 4:
			return *reinterpret_cast <uint32_t*>(zmmTaintTable[reg - REG_ZMM_BASE][idx * size]);
		case 8:
			return *reinterpret_cast <uint64_t*>(zmmTaintTable[reg - REG_ZMM_BASE][idx * size]);
		}
	}
}
bool TaintEnv::isTainted(ADDRINT addr, size_t size)
{
	if (!isMemoryCommitted(addr, size))
		return false;
	if (ALIGN(addr + size) == ALIGN(addr))
	{
		const uint8_t* map = committedMemory[ALIGN(addr)];
		addr &= 0xFFF;
		for (int i = 0; i < size; i++, addr++)
		{
			int idx = addr / 8;
			int target = addr % 8;
			if (map[idx] >> target)
				return true;
		}
		return false;
	}
	//2개의 영역에 걸쳐서 매핑되있는 경우 
	else
	{
		const uint8_t* map1 = committedMemory[ALIGN(addr)];
		const uint8_t* map2 = committedMemory[ALIGN(addr + size)];
		ADDRINT origin = addr;
		addr &= 0xFFF;
		int i;
		for (i = 0; addr < 0x1000; i++, addr++)
		{
			int idx = addr / 8;
			int target = addr % 8;
			if (map1[idx] >> target)
				return true;
		}
		addr = 0;
		for (; i < size; i++, addr++)
		{
			int idx = addr / 8;
			int target = addr % 8;
			if (map2[idx] >> target)
				return true;
		}
	}
}

void TaintEnv::releaseReg(const REG reg, int idx, int size)
{
	std::cout << REG_StringShort(reg) << "(" << size << ")" << " released\n";
	if (reg >= REG_XMM_BASE && reg <= REG_XMM_LAST)
	{
		switch (size)
		{
		case 1:
			xmmTaintTable[reg - REG_XMM_BASE][idx * size] = 0;
			break;
		case 2:
			*reinterpret_cast <uint16_t*>(xmmTaintTable[reg - REG_XMM_BASE][idx * size]) = 0;
			break;
		case 4:
			*reinterpret_cast <uint32_t*>(xmmTaintTable[reg - REG_XMM_BASE][idx * size]) = 0;
			break;
		case 8:
			*reinterpret_cast <uint64_t*>(xmmTaintTable[reg - REG_XMM_BASE][idx * size]) = 0;
			break;
		}
	}
	else if (reg >= REG_YMM_BASE && reg <= REG_YMM_LAST)
	{
		switch (size)
		{
		case 1:
			ymmTaintTable[reg - REG_YMM_BASE][idx * size] = 0;
			break;
		case 2:
			*reinterpret_cast <uint16_t*>(ymmTaintTable[reg - REG_YMM_BASE][idx * size]) = 0;
			break;
		case 4:
			*reinterpret_cast <uint32_t*>(ymmTaintTable[reg - REG_YMM_BASE][idx * size]) = 0;
			break;
		case 8:
			*reinterpret_cast <uint64_t*>(ymmTaintTable[reg - REG_YMM_BASE][idx * size]) = 0;
			break;
		}
	}
	else if (reg >= REG_ZMM_BASE && reg <= REG_ZMM_LAST)
	{
		switch (size)
		{
		case 1:
			zmmTaintTable[reg - REG_ZMM_BASE][idx * size] = 0;
			break;
		case 2:
			*reinterpret_cast <uint16_t*>(zmmTaintTable[reg - REG_ZMM_BASE][idx * size]) = 0;
			break;
		case 4:
			*reinterpret_cast <uint32_t*>(zmmTaintTable[reg - REG_ZMM_BASE][idx * size]) = 0;
			break;
		case 8:
			*reinterpret_cast <uint64_t*>(zmmTaintTable[reg - REG_ZMM_BASE][idx * size]) = 0;
			break;
		}
	}
}

void TaintE::taintReg(const REG reg, int idx, int size)
{
	std::cout << "[WRITE] " << REG_StringShort(reg) << "(" << size << ")" << " tainted\n";
	if (reg >= REG_XMM_BASE && reg <= REG_XMM_LAST)
	{
		switch (size)
		{
		case 1:
			xmmTaintTable[reg - REG_XMM_BASE][idx * size] = 1;
			break;
		case 2:
			*reinterpret_cast <uint16_t*>(xmmTaintTable[reg - REG_XMM_BASE][idx * size]) = 1;
			break;
		case 4:
			*reinterpret_cast <uint32_t*>(xmmTaintTable[reg - REG_XMM_BASE][idx * size]) = 1;
			break;
		case 8:
			*reinterpret_cast <uint64_t*>(xmmTaintTable[reg - REG_XMM_BASE][idx * size]) = 1;
			break;
		}
	}
	else if (reg >= REG_YMM_BASE && reg <= REG_YMM_LAST)
	{
		switch (size)
		{
		case 1:
			ymmTaintTable[reg - REG_YMM_BASE][idx * size] = 1;
			break;
		case 2:
			*reinterpret_cast <uint16_t*>(ymmTaintTable[reg - REG_YMM_BASE][idx * size]) = 1;
			break;
		case 4:
			*reinterpret_cast <uint32_t*>(ymmTaintTable[reg - REG_YMM_BASE][idx * size]) = 1;
			break;
		case 8:
			*reinterpret_cast <uint64_t*>(ymmTaintTable[reg - REG_YMM_BASE][idx * size]) = 1;
			break;
		}
	}
	else if (reg >= REG_ZMM_BASE && reg <= REG_ZMM_LAST)
	{
		switch (size)
		{
		case 1:
			zmmTaintTable[reg - REG_ZMM_BASE][idx * size] = 1;
			break;
		case 2:
			*reinterpret_cast <uint16_t*>(zmmTaintTable[reg - REG_ZMM_BASE][idx * size]) = 1;
			break;
		case 4:
			*reinterpret_cast <uint32_t*>(zmmTaintTable[reg - REG_ZMM_BASE][idx * size]) = 1;
			break;
		case 8:
			*reinterpret_cast <uint64_t*>(zmmTaintTable[reg - REG_ZMM_BASE][idx * size]) = 1;
			break;
		}
	}
}