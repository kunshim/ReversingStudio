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
//xchg 연산의 오퍼랜드 중 메모리 오퍼랜드가 있는 경우 처리
void xchgMem(INS ins, ADDRINT ea, size_t size)
{
	if (!taintStart)
		return;
#ifdef db
	cout << INS_Disassemble(ins) << endl;
#endif
	//다른 하나는 무조건 reg
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
//xchg 연산의 오퍼랜드가 모두 레지스터인 경우
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
//명령어의 오퍼랜드 유형을 반환
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
	//AVX2 까지만 지원
	//Todo xor 초기화 지원?
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
	//오퍼랜드가 3개인 경우
	//마지막 오퍼랜드는 상수, 메모리, 레지스터
	//두번재 오퍼랜드는 레지스터
	//첫번째 오퍼랜드는 레지스터
	else if (count == 3)
	{
		//reg -> mem
		if (false && INS_OperandIsMemory(ins, 0))
		{
			//마지막 오퍼랜드가 상수인 경우는 오퍼랜드가 2개인 것 처럼 해석이 가능하다
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
					//레지스터 2개중 하나만 taint 돼있으면 해당 연산의 결과는 taint 되는 것으로 판단한다
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
			//레지스터 2개중 하나만 taint 돼있으면 해당 연산의 결과는 taint 되는 것으로 판단한다
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
		//나머지 부분이 0으로 채워짐 movq 참조
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
	만약 eax 가 taint 되었다면, 첫줄의 경우 eax는 tainted, 둘쨋줄은 release 되어야 한다
	두 명령어의 차이는 eax가 RW 로 쓰였나, W로 쓰였나의 차이
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
			//multi element 인 경우
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
	//xchg 의 경우 따로처리 (cmpxchg가 여기 포함되는지는 확인필요)
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
	//SIMD 연산인경우
	else if (multiElemState == MULTI_ELEM_STATE::ONLY)
	{
		//메모리 오퍼랜드가 있는 경우
		if (INS_MemoryOperandCount(ins) != 0)
		{
			//쓰기 오퍼랜드가 있는 경우
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
		//레지스터 오퍼랜드만 존재하는 경우
		else
		{
			//쓰기 오퍼랜드가 있는 경우
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
	//SIMD 레지스터와 범용 레지스터, 메모리가 혼합되서 사용하는 연산의 경우
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
	//일반적인 연산
	else
	{
		//여기에는 Implicit 한 오퍼랜드들도 포함되어있기 때문에 필요한 오퍼랜드를 분리해야된다.
		//ex) push edi 에서 [esp] 가 Implicit 한 쓰기 오퍼랜드임     
		vector<REG> regW, regR;
		int count = INS_OperandCount(ins);
		for (int i = 0; i < count; i++)
		{
			// Implicit 하지 않은 상수 오퍼랜드를 가진 경우에 대해서 먼저 처리해준다
			if (INS_OperandIsImmediate(ins, i) && !INS_OperandIsImplicit(ins, i))
			{
				//상수와 메모리 오퍼랜드를 가진 경우
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
			//읽기, 쓰기 레지스터 오퍼랜드를 추출한다 
			else if (INS_OperandWritten(ins, i) && INS_OperandIsReg(ins, i) && INS_OperandReg(ins, i) != REG_EFLAGS)
				regW.push_back(INS_OperandReg(ins, i));
			else if (INS_OperandReadOnly(ins, i) && INS_OperandIsReg(ins, i) && INS_OperandReg(ins, i) != REG_EFLAGS)
				regR.push_back(INS_OperandReg(ins, i));
		}
		if (isReadOnlyInstruction(ins))		
			return;
		
		//읽기, 쓰기 오퍼랜드는 2개를 초과할 수 없다. 

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