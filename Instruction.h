#pragma once
#include <unordered_map>
#include <memory>
#include "pin.H"
#include <intrin.h>

enum INSTRUCTION_TYPE
{
	XCHG_REG_REG,
	XCHG_MEM_REG,
	SIMD_REG_MEM, 
	SIMD_MEM_REG,
	SIMD_REG_REG,
	SIMD_REG_REG_IMM,
	SIMD_REG_REG_MEM,
	SIMD_REG_REG_REG,
	GENERAL_REG_REG,
	GENERAL_MEM_REG,
	GENERAL_REG_IMM,
	GENERAL_MEM_IMM,
	GENERAL_REG_REG_IMM,
	GENERAL_MEM_REG_IMM,
	GENERAL_REG_REG_REG,
	GENERAL_MEM_REG_REG,
	MIXED_REG_GENERAL_REG_SIMD,
	MIXED_REG_SIMD_REG_GENERAL,
	MIXED_MEM_REG,
	MIXD_REG_MEM,
	UNDEFINED
};

class MyIns
{
public:
	MyIns(INS& ins);
	int getOperandCount() const
	{
		return operandCount;
	}
	int getExplicitOperandCount() const
	{
		return explicitOperandCount;
	}
	std::vector<Operand>& getOperands()
	{
		return operands;
	}
	std::vector<Operand>& getReadOperands()
	{
		return readOperands;
	}
	std::vector<Operand>& getWriteOperands()
	{
		return writeOperands;
	}
	std::string getDescription() const;
	bool isReadOnly() const
	{
		return readOnly;
	}
	INSTRUCTION_TYPE getType() const
	{
		return type;
	}
	const std::string& getDisassemble()
	{
		return disassemble;
	}
	static std::unordered_map<OPCODE, MyIns> insCache;
	static std::stringstream error;
private:
	uint8_t operandCount; //effective operand count. Does not match with INS_OperandCount
	uint8_t explicitOperandCount;
	INSTRUCTION_TYPE type;
	bool readOnly;
	std::vector<Operand> operands, readOperands, writeOperands;
	std::string disassemble;
};

class Operand
{
public:
	Operand(ADDRINT displace, REG base, REG index, uint8_t scale,  bool implicit = false);
	Operand(REG reg, bool implicit = false);
	Operand(bool immediate = true)
	{
		immediate = true;
	}
	bool isImplicit() const
	{
		return implicit;
	}
	int getElementCount() const
	{
		return elementCount;
	}
	bool isWriteOperand() const
	{
		return writtenOperand;
	}
	bool isReadOperand() const
	{
		return readOperand;
	}
	bool isReadAndWritten() const
	{
		return writtenOperand && readOperand;
	}
	bool isMemoryOperand() const
	{
		return !immediate && (reg == REG_INVALID());
	}
	bool isRegisterOperand() const
	{
		return !immediate && (reg != REG_INVALID());
	}
	bool isImmediate() const
	{
		return immediate;
	}
	std::string getDescription() const
	{		
		std::stringstream ss;
		if (isImplicit())
			ss << "implicit ";
		else
			ss << "explicit ";
		if (isReadAndWritten())
			ss << "read/write ";
		else if (isReadOperand())
			ss << "read ";
		else if (isWriteOperand())
			ss << "write ";
		if (isRegisterOperand())
			ss << REG_StringShort(reg) << "\n";
		else if (isMemoryOperand())
		{
			if (displace != 0)
				ss << hexstr(displace) << " ";
			if (base != REG_INVALID())
				ss << REG_StringShort(base) << " ";
			if (index != REG_INVALID())
				ss << REG_StringShort(index) << " ";
			if (scale != 0)
				ss << scale;
		}
		else
			ss << "immediate operand ";		
		return ss.str();
	}
private:
	friend class MyIns;
	bool readOperand;
	bool writtenOperand;
	bool implicit;	
	bool immediate;
	uint8_t elementCount;
	uint8_t elementSize;
	uint16_t operandSize;
	ADDRINT displace;
	REG reg;
	REG base;
	REG index;
	uint8_t scale;
	void setDefaultMember(INS& ins, int i)
	{
		elementCount = INS_OperandElementCount(ins, i);
		elementSize = INS_OperandElementSize(ins, i);
		readOperand = INS_OperandRead(ins, i);
		writtenOperand = INS_OperandWritten(ins, i);
		operandSize = INS_OperandSize(ins, i);
	}

};

