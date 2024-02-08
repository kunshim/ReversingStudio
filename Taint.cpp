#include "pin.H"
#include <iostream>
#include <fstream>
#include <string>
#include "VirtualEnv.h"
#include <intrin.h>
#include "Image.h"

using namespace std;
ofstream outputFile;
ofstream except;
TaintEnv env;
bool taintStart = false;
#define db
unordered_map<ADDRINT, string> insMap;


enum MULTI_ELEM_STATE
{
	ONLY,
	MIXED,
	NONE
};


void init(void* v)
{
}

void fini(INT32 code, void* v)
{
	outputFile.close();
	except.close();
}
//xchg ������ ���۷��� �� �޸� ���۷��尡 �ִ� ��� ó��
void xchgMem(INS ins, ADDRINT ea, size_t size)
{
	if (!taintStart)
		return;
#ifdef db
	cout << INS_Disassemble(ins) << endl;
#endif
	//�ٸ� �ϳ��� ������ reg
	REG reg;
	for (int i = 0; i < INS_OperandCount(ins); i++)
	{
		if (!INS_OperandIsImplicit(ins, i) && INS_OperandIsReg(ins, i))
		{
			reg = INS_OperandReg(ins, i);
			break;
		}
	}
	bool memTainted = env.memoryOperandIsTainted(ins, ea, size);
	//only register tainted
	if (!memTainted && env.isTainted(reg))
	{
		env.releaseReg(ins, reg);
		env.taintMem(ea, size);

	}
	//only memory tainted
	else if (memTainted && !env.isTainted(reg))
	{
		env.releaseMem(ea, size);
		env.taintReg(reg);
	}
}
//xchg ������ ���۷��尡 ��� ���������� ���
void xchgReg(INS ins)
{
	if (!taintStart)
		return;
#ifdef db
	cout << INS_Disassemble(ins) << endl;
#endif
	REG regA, regB;
	for (int i = 0; i < INS_OperandCount(ins); i++)
	{
		if (!INS_OperandIsImplicit(ins, i) && INS_OperandIsReg(ins, i))
		{
			if (!REG_valid(regA))
				regA = INS_OperandReg(ins, i);
			else
			{
				regB = INS_OperandReg(ins, i);
				break;
			}
		}
	}
	if (env.isTainted(regA) && !env.isTainted(regB))
	{
		env.releaseReg(ins, regA);
		env.taintReg(regB);
	}
	else if (!env.isTainted(regA) && env.isTainted(regB))
	{
		env.releaseReg(ins, regB);
		env.taintReg(regA);
	}

}
//��ɾ��� ���۷��� ������ ��ȯ
MULTI_ELEM_STATE hasMultiElement(INS& ins)
{
	int count = INS_OperandCount(ins);
	int c = 0;
	int d = 0;
	for (int i = 0; i < count; i++)
	{
		if (INS_OperandIsImplicit(ins, i) && INS_OperandElementCount(ins, i) >= 2)
			c++;
		else if (INS_OperandIsImplicit(ins, i) && INS_OperandElementCount(ins, i) == 1)
			d++;
	}
	if (c > 0 && !d)
		return MULTI_ELEM_STATE::ONLY;
	else if (c > 0 && d > 0)
		return MULTI_ELEM_STATE::MIXED;
	else
		return MULTI_ELEM_STATE::NONE;
}

void simdMem(INS ins, ADDRINT ea)
{
	if (!taintStart)
		return;
#ifdef db
	cout << INS_Disassemble(ins) << endl;
#endif
	ASSERTX(INS_OperandCount(ins) <= 3);
	//AVX2 ������ ����
	//Todo xor �ʱ�ȭ ����?
	int count = INS_OperandCount(ins);
	if (count == 2)
	{
		//reg2mem
		if (INS_OperandIsReg(ins, 1))
		{
		REG2MEM:
			int size = INS_MemoryOperandElementSize(ins, 0);
			REG reg = INS_OperandReg(ins, 1);
			int regSize = REG_Size(reg);
			for (int i = 0; i < (int)(regSize / size); i++)
			{
				if (env.isTainted(reg, i, size))
					env.taintMem(ea + size * i, size);
				else
					env.releaseMem(ea + size * i, size);
			}
		}
		//mem -> reg
		else
		{
		MEM2REG:
			int size = INS_MemoryOperandElementSize(ins, 0);
			REG reg = INS_OperandReg(ins, 0);
			int regSize = REG_Size(reg);
			for (int i = 0; i < (int)(regSize / size); i++)
			{
				if (env.memoryOperandIsTainted(ins, ea, size))
					env.taintReg(reg, i, size);
				else
					env.releaseReg(reg, i, size);
			}
		}
	}
	//���۷��尡 3���� ���
	//������ ���۷���� ���, �޸�, ��������
	//�ι��� ���۷���� ��������
	//ù��° ���۷���� ��������
	else if (count == 3)
	{
		//reg -> mem
		if (false && INS_OperandIsMemory(ins, 0))
		{
			//������ ���۷��尡 ����� ���� ���۷��尡 2���� �� ó�� �ؼ��� �����ϴ�
			if (INS_OperandIsImmediate(ins, 2))
				goto REG2MEM;
			else if (INS_OperandIsReg(ins, 2))
			{
				int size = INS_MemoryOperandElementSize(ins, 0);
				REG regA = INS_OperandReg(ins, 1);
				REG regB = INS_OperandReg(ins, 2);
				int regSize = REG_Size(regA);
				for (int i = 0; i < (int)(regSize / size); i++)
				{
					//�������� 2���� �ϳ��� taint �������� �ش� ������ ����� taint �Ǵ� ������ �Ǵ��Ѵ�
					if (env.isTainted(regA, i, size) || env.isTainted(regB, i, size))
						env.taintMem(ea + size * i, size);
					else
						env.releaseMem(ea + size * i, size);
				}
			}
			else
			{
				except << INS_Disassemble(ins) << "\nOperand type mismatch!" << endl;
				exit(-1);
			}
		}
		//mem -> reg
		else
		{
			if (INS_OperandIsImmediate(ins, 2))
				goto MEM2REG;
			else
			{
				int size = INS_MemoryOperandElementSize(ins, 0);
				REG regA = INS_OperandReg(ins, 0);
				REG regB = INS_OperandReg(ins, 1);
				int regSize = REG_Size(regA);
				for (int i = 0; i < (int)(regSize / size); i++)
				{
					if (env.isTainted(regB, i, size) || env.memoryOperandIsTainted(ins, ea, size))
						env.taintReg(regA, i, size);
					else
						env.releaseReg(regA, i, size);
				}
			}

		}
	}
}

void simdReg(INS ins)
{
	if (!taintStart)
		return;
#ifdef db
	cout << INS_Disassemble(ins) << endl;
#endif
	ASSERTX(INS_OperandCount(ins) <= 3);
	//reg -> reg
	int count = INS_OperandCount(ins);
	if (count == 2)
	{
	REG2REG:
		REG regA = INS_OperandReg(ins, 0);
		REG regB = INS_OperandReg(ins, 1);
		int size = INS_OperandElementSize(ins, 0);
		int regSize = REG_Size(regA);
		for (int i = 0; i < (int)(regSize / size); i++)
		{
			if (env.isTainted(regB, i, size))
				env.taintReg(regA, i, size);
			else
				env.releaseReg(regA, i, size);
		}
	}
	else if (count == 3)
	{
		if (INS_OperandIsImmediate(ins, 2))
			goto REG2REG;
		REG regA = INS_OperandReg(ins, 0);
		REG regB = INS_OperandReg(ins, 1);
		REG regC = INS_OperandReg(ins, 2);
		int size = INS_MemoryOperandElementSize(ins, 0);
		int regSize = REG_Size(regA);
		for (int i = 0; i < (int)(regSize / size); i++)
		{
			//�������� 2���� �ϳ��� taint �������� �ش� ������ ����� taint �Ǵ� ������ �Ǵ��Ѵ�
			if (env.isTainted(regB, i, size) || env.isTainted(regC, i, size))
				env.taintReg(regA, i, size);
			else
				env.releaseReg(regA, i, size);
		}
	}

}

void simdRegMixed(INS ins)
{
	cout << hexstr(INS_Address(ins)) << " " << INS_Disassemble(ins) << endl;
	if (!taintStart)
		return;
#ifdef db
	cout << INS_Disassemble(ins) << endl;
#endif
	ASSERTX(INS_OperandCount(ins) == 2);
	REG regA = INS_OperandReg(ins, 0);
	REG regB = INS_OperandReg(ins, 1);
	//simd -> general
	if (INS_OperandElementCount(ins, 1) >= 2)
	{
		int dSize = REG_Size(regA);
		if (env.isTainted(regB, 0, dSize))
			env.taintReg(regA);
		else
			env.releaseReg(ins, regA);
	}
	//general -> simd
	else
	{
		int sSize = REG_Size(regB);
		int dSize = REG_Size(regA);
		if (env.isTainted(regB))
			env.taintReg(regA, 0, sSize);
		else
			env.releaseReg(regA, 0, sSize);
		//������ �κ��� 0���� ä���� movq ����
		for (int i = 1; i < (int)(dSize / sSize); i++)
			env.releaseReg(regA, i, sSize);
	}
}
//movq m64, xmm0
void simdReg2MemMixed(INS ins, ADDRINT ea)
{
	if (!taintStart)
		return;
#ifdef db
	cout << INS_Disassemble(ins) << endl;
#endif
	ASSERTX(INS_OperandCount(ins) == 2);
	REG reg = INS_OperandReg(ins, 1);
	int dSize = INS_OperandSize(ins, 0);
	if (env.isTainted(reg, 0, dSize))
		env.taintMem(ea, dSize);
	else
		env.releaseMem(ea, dSize);
}
//movq xmm0, m64
void simdMem2RegMixed(INS ins, ADDRINT ea)
{
	if (!taintStart)
		return;
#ifdef db
	cout << INS_Disassemble(ins) << endl;
#endif
	ASSERTX(INS_OperandCount(ins) == 2);
	REG reg = INS_OperandReg(ins, 0);
	int sSize = INS_OperandSize(ins, 1);
	int dSize = INS_OperandSize(ins, 0);
	if (env.memoryOperandIsTainted(ins, ea, sSize))
		env.taintReg(reg, 0, sSize);
	else
		env.releaseReg(reg, 0, sSize);
	for (int i = 1; i < (int)(dSize / sSize); i++)
		env.releaseReg(reg, i, sSize);
}

void generalReg(INS ins, REG dst, REG src)
{
	if (!taintStart)
		return;
#ifdef db
	cout << INS_Disassemble(ins) << endl;
#endif
	if (env.isTainted(src))
		env.taintReg(dst);
	else
		env.releaseReg(ins, dst);
}

void generalRegImm(INS ins)
{
	if (!taintStart)
		return;
#ifdef db
	cout << INS_Disassemble(ins) << endl;
#endif
	/*
	add eax, 0x10
	mov eax, 0x10
	���� eax �� taint �Ǿ��ٸ�, ù���� ��� eax�� tainted, ��¶���� release �Ǿ�� �Ѵ�
	�� ��ɾ��� ���̴� eax�� RW �� ������, W�� �������� ����
	*/
	ASSERTX(!INS_OperandReadOnly(ins, 0));
	REG reg = INS_OperandReg(ins, 0);
	if (INS_OperandWrittenOnly(ins, 0) && env.isTainted(reg))
		env.releaseReg(ins, reg);
}

void generalMem2Reg(INS ins, ADDRINT ea, size_t size, REG reg)
{
	if (!taintStart)
		return;
#ifdef db
	cout << INS_Disassemble(ins) << endl;
#endif
	if (env.memoryOperandIsTainted(ins, ea, size))
		env.taintReg(reg);
	else
	{
		cout << "this\n";
		env.releaseReg(ins, reg);
	}
}

void generalReg2Mem(INS ins, ADDRINT ea, size_t size, REG reg)
{
	if (!taintStart)
		return;
#ifdef db
	cout << INS_Disassemble(ins) << endl;
#endif
	if (env.isTainted(reg))
		env.taintMem(ea, size);
	else
		env.releaseMem(ea, size);
}

void generalMemImm(INS ins, ADDRINT ea)
{
	if (!taintStart)
		return;
#ifdef db
	cout << INS_Disassemble(ins) << endl;
#endif
	ASSERTX(!INS_OperandReadOnly(ins, 0));
	int size = INS_OperandSize(ins, 0);
	if (INS_OperandWrittenOnly(ins, 0) && env.memoryOperandIsTainted(ins, ea, size))
		env.releaseMem(ea, size);
}

bool isReadOnlyInstruction(INS& ins)
{
	int count = INS_OperandCount(ins);
	for (int i = 0; i < count; i++)
	{
		if (INS_OperandIsReg(ins, i) && INS_OperandReg(ins, i) != REG_EFLAGS && INS_OperandWritten(ins, i))
			return false;
		else if (INS_OperandIsMemory(ins, i) && INS_OperandWritten(ins, i))
			return false;
	}
	return true;
}

void printFollow(INS ins, ADDRINT ea = 0)
{
	if (!taintStart)
		return;
#ifdef db
	cout << INS_Disassemble(ins) << endl;
#endif
	int count = INS_OperandCount(ins);
	bool printed = false;
	for (int i = 0; i < count; i++)
	{
		if (!INS_OperandIsImplicit(ins, i))
		{
			int elemCount = INS_OperandElementCount(ins, i);
			if (elemCount == 1)
			{
				if (INS_OperandIsReg(ins, i))
				{
					REG reg = INS_OperandReg(ins, i);
					if (env.isTainted(reg))
					{
						if (!printed)
						{
							cout << "[READ] " << INS_Disassemble(ins) << "\n";
							printed = true;
						}
						cout << setw(30) << REG_StringShort(reg) << " is tainted\n";
					}
				}
				else if (INS_OperandIsMemory(ins, i))
				{
					if (env.memoryOperandIsTainted(ins, ea, INS_OperandSize(ins, i)))
					{
						if (!printed)
						{
							cout << "[READ] " << INS_Disassemble(ins) << "\n";
							printed = true;
						}
						cout << setw(30) << hexstr(ea) << " is tainted\n";
					}
				}
			}
			//multi element �� ���
			else
			{
				if (INS_OperandIsReg(ins, i))
				{
					int size = INS_OperandElementSize(ins, i);
					REG reg = INS_OperandReg(ins, i);
					for (int i = 0; i < elemCount; i++)
					{
						if (env.isTainted(reg, i, size))
						{
							if (!printed)
							{
								cout << "[READ] " << INS_Disassemble(ins) << "\n";
								printed = true;
							}
							cout << setw(30) << REG_StringShort(reg) << " is tainted\n";
						}
					}
				}
				else if (INS_OperandIsMemory(ins, i))
				{
					if (env.memoryOperandIsTainted(ins, ea, INS_OperandSize(ins, i)))
					{
						if (!printed)
						{
							cout << "[READ] " << INS_Disassemble(ins) << "\n";
							printed = true;
						}
						cout << setw(30) << hexstr(ea) << " is tainted\n";
					}
				}
			}
		}
	}
}

int INS_PraticalOperandCount(INS& ins)
{
	int result = 0;
	for (int i = 0; i < INS_OperandCount(ins); i++)
	{
		if (!INS_OperandIsImplicit(ins, i))
			result++;
	}
	return result;
}

void instrument(INS ins, void* v)
{		
	if (INS_IsControlFlow(ins) || INS_IsNop(ins) || INS_IsInterrupt(ins) || INS_IsSyscall(ins) || INS_IsSysenter(ins))
		return;
	insMap[INS_Address(ins)] = INS_Disassemble(ins);
	MULTI_ELEM_STATE multiElemState = hasMultiElement(ins);
	//xchg �� ��� ����ó�� (cmpxchg�� ���� ���ԵǴ����� Ȯ���ʿ�)
	if (INS_IsXchg(ins))
	{
		if (INS_MemoryOperandCount(ins) == 1)
		{
			INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(xchgMem),
				IARG_PTR, ins,
				IARG_MEMORYOP_EA, 0,
				IARG_MEMORYOP_SIZE, 0,
				IARG_END);	
		}
		else
		{
			INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(xchgReg),
				IARG_PTR, ins,
				IARG_END);
		}
	}
	//SIMD �����ΰ��
	else if (multiElemState == MULTI_ELEM_STATE::ONLY)
	{
		//�޸� ���۷��尡 �ִ� ���
		if (INS_MemoryOperandCount(ins) != 0)
		{
			//���� ���۷��尡 �ִ� ���
			if (!isReadOnlyInstruction(ins))
			{
				INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(simdMem),
					IARG_PTR, ins,
					IARG_MEMORYOP_EA, 0,
					IARG_END);
			}
			else
			{
				INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(printFollow),
					IARG_PTR, ins,
					IARG_MEMORYOP_EA, 0,
					IARG_END);
			}
		}
		//�������� ���۷��常 �����ϴ� ���
		else
		{
			//���� ���۷��尡 �ִ� ���
			if (!isReadOnlyInstruction(ins))
			{
				INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(simdReg),
					IARG_PTR, ins,
					IARG_END);
			}
			else
			{
				INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(printFollow),
					IARG_PTR, ins,
					IARG_END);
			}
		}
	}
	//SIMD �������Ϳ� ���� ��������, �޸𸮰� ȥ�յǼ� ����ϴ� ������ ���
	//ex) movd mmx, eax 
	else if (multiElemState == MULTI_ELEM_STATE::MIXED)
	{		
		if (!INS_MemoryOperandCount(ins))
		{
			if (!isReadOnlyInstruction(ins))
			{
				INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(simdRegMixed),
					IARG_PTR, ins,
					IARG_END);
			}
			else
			{
				INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(printFollow),
					IARG_PTR, ins,
					IARG_END);
			}
		}
		else
		{
			if (!isReadOnlyInstruction(ins))
			{
				if (INS_OperandIsMemory(ins, 0))
				{
					INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(simdReg2MemMixed),
						IARG_PTR, ins,
						IARG_MEMORYOP_EA, 0,
						IARG_END);
				}
				else
				{
					INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(simdMem2RegMixed),
						IARG_PTR, ins,
						IARG_MEMORYOP_EA, 0,
						IARG_END);
				}
			}
			else
			{
				INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(printFollow),
					IARG_PTR, ins,
					IARG_MEMORYOP_EA, 0,
					IARG_END);
			}
		}
	}
	//�Ϲ����� ����
	else
	{
		//���⿡�� Implicit �� ���۷���鵵 ���ԵǾ��ֱ� ������ �ʿ��� ���۷��带 �и��ؾߵȴ�.
		//ex) push edi ���� [esp] �� Implicit �� ���� ���۷�����     
		vector<REG> regW, regR;
		int count = INS_OperandCount(ins);
		for (int i = 0; i < count; i++)
		{
			// Implicit ���� ���� ��� ���۷��带 ���� ��쿡 ���ؼ� ���� ó�����ش�
			if (INS_OperandIsImmediate(ins, i) && !INS_OperandIsImplicit(ins, i))
			{
				//����� �޸� ���۷��带 ���� ���
				if (INS_MemoryOperandCount(ins))
				{
					if (!isReadOnlyInstruction(ins))
					{
						INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(generalMemImm),
							IARG_PTR, ins,
							IARG_MEMORYOP_EA, 0,
							IARG_END);
					}
					else
					{
						INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(printFollow),
							IARG_PTR, ins,
							IARG_MEMORYOP_EA, 0,
							IARG_END);
					}
				}
				else
				{
					if (!isReadOnlyInstruction(ins))
					{
						INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(generalRegImm),
							IARG_PTR, ins,
							IARG_END);
					}
					else
					{
						INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(printFollow),
							IARG_PTR, ins,
							IARG_END);
					}
				}
				return;
			}
			//�б�, ���� �������� ���۷��带 �����Ѵ� 
			else if (INS_OperandWritten(ins, i) && INS_OperandIsReg(ins, i) && INS_OperandReg(ins, i) != REG_EFLAGS)
				regW.push_back(INS_OperandReg(ins, i));
			else if (INS_OperandReadOnly(ins, i) && INS_OperandIsReg(ins, i) && INS_OperandReg(ins, i) != REG_EFLAGS)
				regR.push_back(INS_OperandReg(ins, i));
		}
		if (isReadOnlyInstruction(ins))		
			return;
		
		//�б�, ���� ���۷���� 2���� �ʰ��� �� ����. 

		if (!(regW.size() <= 1 && regR.size() <= 1))
		{
			except << "Exception : " << INS_Disassemble(ins) << "\n";
			return;
		}
		if (!INS_MemoryOperandCount(ins) && INS_PraticalOperandCount(ins) >= 2)
		{
			
			//reg -> reg
			INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(generalReg),
				IARG_PTR, ins,
				IARG_ADDRINT, regW[0],
				IARG_ADDRINT, regR[1],
				IARG_END);
		}
		else if (INS_MemoryOperandCount(ins) && INS_PraticalOperandCount(ins) >= 2 && INS_OperandIsMemory(ins, 0))
		{

		}

	}

}
ADDRINT targetMem;
void getEAX(ADDRINT val)
{
	targetMem = val;
}

void ss()
{
	env.taintMem(targetMem, 0x100);
	taintStart = true;

}

void instrumentImage(IMG img, VOID* v)
{
	Image image(IMG_Id(img));
	string imageName = image.getShortName();
	cout << imageName << endl;
	if (imageName.find("taint_test.exe") != string::npos)
	{
		PIN_LockClient();
		RTN rtn = RTN_CreateAt(image.getBaseAddress() + 0xc88c, "start22");
		PIN_UnlockClient();
		ASSERTX(RTN_Valid(rtn));
		RTN_Open(rtn);
		INS ins = RTN_InsHead(rtn);
		INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(getEAX),
			IARG_REG_VALUE, REG_EAX,
			IARG_END);
		ins = INS_Next(ins);
		except << hexstr(INS_Address(ins)) << " " << INS_Disassemble(ins) << endl;
		ASSERTX(INS_Valid(ins));
		INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ss),
			IARG_END);
		RTN_Close(rtn);
		cout << "Init end!" << endl;
	}
	Image::writeMoudleInfo("module.txt");
}

int main(int argc, char* argv[])
{
	PIN_InitSymbols();
	PIN_Init(argc, argv);
	outputFile.open("taint.txt");
	except.open("except.txt");
	if (!outputFile.is_open() || !except.is_open())
		exit(-1);
	//cout.rdbuf(outputFile.rdbuf());
	//cerr.rdbuf(outputFile.rdbuf());
	cout.tie(0);
	ios_base::sync_with_stdio(false);
	PIN_AddApplicationStartFunction(init, 0);
	INS_AddInstrumentFunction(instrument, 0);
	IMG_AddInstrumentFunction(instrumentImage, 0);
	PIN_AddFiniFunction(fini, 0);
	PIN_StartProgram();
	return 0;
}