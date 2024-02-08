#include "Instruction.h"

std::unordered_map<OPCODE, MyIns> MyIns::insCache;
std::stringstream MyIns::error;

MyIns::MyIns(INS& ins)
{
	//TODO segment regiseter / prefix support	
	if (insCache.find(INS_Opcode(ins)) != insCache.end())
	{
		*this = insCache[INS_Opcode(ins)];
		return;
	}
	disassemble = INS_Disassemble(ins);
	int multiElemCount = 0, singleElemCount = 0;
	operandCount = 0;
	explicitOperandCount = 0;
	for (int i = 0; i < INS_OperandCount(ins); i++)
	{
		if (INS_OperandIsImplicit(ins, i))
		{
			if (INS_OperandIsReg(ins, i) && INS_OperandReg(ins, i) != REG_EFLAGS)
			{
				operandCount++;
				Operand operand = Operand(INS_OperandReg(ins, i), true);
				operand.setDefaultMember(ins, i);
				operands.push_back(operand);
				if (operand.isReadOperand())
					readOperands.push_back(operand);
				else
					writeOperands.push_back(operand);
			}
			else if (INS_OperandIsReg(ins, i) && INS_OperandReg(ins, i) == REG_EFLAGS)
				continue;
			else if (INS_OperandIsMemory(ins, i)) //push rdi -> qword ptr [rsp] is implicit memory operand
			{
				operandCount++;
				REG base = INS_OperandMemoryBaseReg(ins, i);
				ADDRINT displacement = INS_OperandMemoryDisplacement(ins, i);
				REG index = INS_OperandMemoryIndexReg(ins, i);
				uint8_t scale = INS_OperandMemoryScale(ins, i);
				Operand operand = Operand(displacement, base, index, scale, true);
				operand.setDefaultMember(ins, i);
				operands.push_back(operand);
				if (operand.isReadOperand())
					readOperands.push_back(operand);
				else
					writeOperands.push_back(operand);
			}
			else
				error << "[Exception] " << disassemble << " has implicit operand. But can't processing.\n";
			if (INS_OperandElementCount(ins, i) > 1)
				multiElemCount++;
			else
				singleElemCount++;

		}
		else
		{
			explicitOperandCount++;
			operandCount++;
			if (INS_OperandIsReg(ins, i))
			{
				Operand operand = Operand(INS_OperandReg(ins, i));
				operand.setDefaultMember(ins, i);
				operands.push_back(operand);
				if (operand.isReadOperand())
					readOperands.push_back(operand);
				else
					writeOperands.push_back(operand);
			}
			else if (INS_OperandIsMemory(ins, i))
			{
				REG base = INS_OperandMemoryBaseReg(ins, i);
				ADDRINT displacement = INS_OperandMemoryDisplacement(ins, i);
				REG index = INS_OperandMemoryIndexReg(ins, i);
				uint8_t scale = INS_OperandMemoryScale(ins, i);
				Operand operand = Operand(displacement, base, index, scale, true);
				operand.setDefaultMember(ins, i);
				operands.push_back(operand);
				if (operand.isReadOperand())
					readOperands.push_back(operand);
				else
					writeOperands.push_back(operand);
			}
			else if (INS_OperandIsImmediate(ins, i))
			{
				Operand operand = Operand();
				operand.operandSize = INS_OperandSize(ins, i);
			}
			else
				error << "[Exception] " << disassemble << " has explicit operand. But can't processing.\n";
			if (INS_OperandElementCount(ins, i) > 1)
				multiElemCount++;
			else
				singleElemCount++;
		}
	}
	readOnly = writeOperands.size() == 0 ? true : false;
	if (INS_IsXchg(ins))
	{
		if (writeOperands[0].isMemoryOperand())
			type = XCHG_MEM_REG;
		else
			type = XCHG_REG_REG;
	}
	//Pure SIMD instruction
	else if (multiElemCount && !singleElemCount)
	{
		if (writeOperands.size() > 1)
		{
			type = UNDEFINED;	
			error << "[Exception] " << disassemble << " has multi write-operands.\n";
		}
		else if (!isReadOnly)
		{
			if (operandCount == 2)
			{				
				Operand w = writeOperands[0];
				Operand o = readOperands[0];
				if (w.isMemoryOperand() && o.isRegisterOperand())
					type = SIMD_MEM_REG;
				else if (w.isRegisterOperand() && o.isMemoryOperand())
					type = SIMD_REG_MEM;
				else
					type = SIMD_REG_REG;
			}
			else if (operandCount == 3)
			{
				
				Operand w = writeOperands[0];
				Operand r1 = readOperands[0], r2 = readOperands[1];
				if (r2.isImmediate())
					type = SIMD_REG_REG_IMM;
				else if (r2.isMemoryOperand())
					type = SIMD_REG_REG_MEM;
				else if (r2.isRegisterOperand())
					type = SIMD_REG_REG_REG;
				else
					type = UNDEFINED;
			}					
		}
		else
		{
			error << "[Exception] " << disassemble << " is multi operand instruction. But readonly.\n";
			type = UNDEFINED;
		}
	}
	//Pure general instruction
	//add eax, 1인 경우 rw 인 오퍼랜드가 있다. 
	else if (!multiElemCount && singleElemCount)
	{
		if (!isReadOnly)
		{
			if (operandCount == 2)
			{
				//rw 한 오퍼랜드의 처리를 해주어야 한다. 
				
				Operand w = writeOperands[0];
				Operand r = readOperands[0];
				if (w.isMemoryOperand())
				{
					if (r.isImmediate())
						type = GENERAL_MEM_IMM;
					else
						type = GENERAL_MEM_REG;
				}
				else if (w.isRegisterOperand())
				{

				}
			}
			else if (operandCount == 3)
			{

			}
			else
			{

			}
		}
	}
	if (type == UNDEFINED)	
		error << getDescription() << "\n";
	insCache[INS_Opcode(ins)] = *this;
}

std::string MyIns::getDescription() const
{
	std::stringstream desc;
	desc << disassemble << "\n";
	switch (type)
	{
	case XCHG_REG_REG:
		desc << "Type : XCHG_REG_REG\n";
		break;
	case XCHG_MEM_REG:
		desc << "Type : XCHG_MEM_REG\n";
		break;
	case SIMD_REG_MEM:
		desc << "Type : SIMD_REG_MEM\n";
		break;
	case SIMD_MEM_REG:
		desc << "Type : SIMD_MEM_REG\n";
		break;
	case SIMD_REG_REG:
		desc << "Type : SIMD_REG_REG\n";
		break;
	case SIMD_REG_REG_IMM:
		desc << "Type : SIMD_REG_REG_IMM\n";
		break;
	case SIMD_REG_REG_MEM:
		desc << "Type : SIMD_REG_REG_MEM\n";
		break;
	case SIMD_REG_REG_REG:
		desc << "Type : SIMD_REG_REG_REG\n";
		break;
	case GENERAL_REG_REG:
		desc << "Type : GENERAL_REG_REG\n";
		break;
	case GENERAL_MEM_REG:
		desc << "Type : GENERAL_MEM_REG\n";
		break;
	case GENERAL_REG_IMM:
		desc << "Type : GENERAL_REG_IMM\n";
		break;
	case GENERAL_MEM_IMM:
		desc << "Type : GENERAL_MEM_IMM\n";
		break;
	case MIXED_REG_GENERAL_REG_SIMD:
		desc << "Type : MIXED_REG_GENERAL_REG_SIMD\n";
		break;
	case MIXED_REG_SIMD_REG_GENERAL:
		desc << "Type : MIXED_REG_SIMD_REG_GENERAL\n";
		break;
	case MIXED_MEM_REG:
		desc << "Type : MIXED_MEM_REG\n";
		break;
	case MIXD_REG_MEM:
		desc << "Type : MIXD_REG_MEM\n";
		break;
	case UNDEFINED:
		desc << "Type : Undefined\n";
		break;
	}
	desc << "Operand count : " << operandCount << "\n";
	desc << "Explicit operand count : " << explicitOperandCount << "\n";
	for (const auto& op : operands)
		desc << op.getDescription() << " ";

	return desc.str();
}