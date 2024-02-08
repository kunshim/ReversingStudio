#pragma once
#include "pin.H"
#include <iostream>
#include <unordered_map>
namespace WINDOWS
{
#include "Windows.h"
}
#define ALIGN(x) (((x) >> 12) << 12)

class TaintEnv
{
public:
	TaintEnv();
	//Taint general register
	void taintReg(const REG reg)
	{
		std::cout << "[WRITE] " << REG_StringShort(reg) << " tainted\n";
		regTaintTable[reg] = 1;
		for (int i = 0; ; i++)
		{
			if (!regRelateTable[reg][i])
				break;
			regTaintTable[regRelateTable[reg][i]] = 1;
		}
	}
	/// <summary>
	/// Taint multi element register
	/// </summary>
	/// <param name="reg">register</param>
	/// <param name="idx">element index</param>
	/// <param name="size">element size</param>
	void taintReg(const REG reg, int idx, int size);
	void releaseReg(const REG reg)
	{
		regTaintTable[reg] = 0;
		for (int i = 0; ; i++)
		{
			if (!regRelateTable[reg][i])
				break;
			regTaintTable[regRelateTable[reg][i]] = 0;
		}
	}
	void releaseReg(const REG reg, int idx, int size);
	void taintMem(ADDRINT addr, size_t size);
	void releaseMem(ADDRINT addr, size_t size);
	bool isTainted(REG reg) const
	{
		ASSERTX(reg < REG_XMM_BASE);
		return regTaintTable[reg];
	}
	bool isTainted(ADDRINT addr, size_t size);
	bool isTainted(REG reg, int idx, size_t size) const;
	bool memoryOperandIsTainted(INS& ins, ADDRINT ea, size_t size);
private:	
	uint8_t regTaintTable[512]; //register taint table
	uint16_t regRelateTable[512][6];
	uint8_t xmmTaintTable[REG_XMM_LAST - REG_XMM_BASE + 1][0x10];
	uint8_t ymmTaintTable[REG_YMM_LAST - REG_YMM_BASE + 1][0x20]; 
	uint8_t zmmTaintTable[REG_ZMM_LAST - REG_ZMM_BASE + 1][0x40]; 
	std::unordered_map<ADDRINT, uint8_t*> committedMemory; //store memory is committed
	void setRelate(int num, ...);
	bool isMemoryCommitted(ADDRINT addr, size_t size = 1) const
	{
		if (size == 1)
			return committedMemory.find(ALIGN(addr)) != committedMemory.end();
		else
			return (committedMemory.find(ALIGN(addr)) != committedMemory.end()) && (committedMemory.find(ALIGN(addr + size)) != committedMemory.end());
	}
};
