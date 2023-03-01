#include "codeGen.h"

/* */

/* */

FILE *output;
char *currentFuncName;

int const_label_no;
int while_label_no;
int for_label_no;
int if_label_no;
int short_circuit_label;

void emit(char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vfprintf(output, fmt, args);
	fprintf(output, "\n");
	va_end(args);
}

void gen_head(char *name)
{
	emit(".text");
	emit("_start_%s:", name);
}

void gen_prologue(char *name)
{
	emit("str x30, [sp, #0]");  // store return address
	emit("str x29, [sp, #-8]"); // save old fp
	emit("add x29, sp, #-8");   // new fp
	emit("add sp, sp, #-16");   // new sp
	emit("ldr x30, =_frameSize_%s", name);
	emit("ldr w30, [x30, #0]");
	emit("sub sp, sp, w30"); // push new AR
	save_reg(output);
}

void gen_epilogue(char *name)
{
	emit("_end_%s:", name);
	restore_reg(output);
	emit("ldr x30, [x29, #8]"); // restore return address
	emit("mov sp, x29");
	emit("add sp, sp, #8");	// pop AR "add sp, x29, #8"
	emit("ldr x29, [x29,#0]"); // restore caller (old) fp
	emit("RET x30");
	emit(".data");
}

int offset = 0;
int largest = 0;

void reset_offset()
{
	offset = 0;
	largest = 0;
}

void set_offset(SymbolAttribute *attribute)
{
	int variableSize = getVariableSize(attribute->attr.typeDescriptor);
	offset = offset - variableSize;
	attribute->offset = offset;
}

int isGlobalVar(SymbolTableEntry *symbolTableEntry)
{
	return symbolTableEntry->nestingLevel == 0;
}

MyIntVector *getMyIntVector(int initialCapacity)
{
	if (initialCapacity < 0)
	{
		exit(EXIT_FAILURE);
	}

	MyIntVector *myIntVectorPtr = (MyIntVector *)malloc(sizeof(MyIntVector));
	myIntVectorPtr->capacity = initialCapacity;
	myIntVectorPtr->size = 0;
	if (initialCapacity != 0)
	{
		myIntVectorPtr->data = (int *)malloc(sizeof(int) * initialCapacity);
	}
	else
	{
		myIntVectorPtr->data = NULL;
	}

	return myIntVectorPtr;
}

void myPushBack(MyIntVector *myIntVector, int val)
{
	if (myIntVector->size == myIntVector->capacity)
	{
		int *oldData = myIntVector->data;
		int oldCapacity = myIntVector->capacity;
		myIntVector->capacity = myIntVector->capacity * 2 + 1;
		myIntVector->data = (int *)malloc(sizeof(int) * myIntVector->capacity);
		memcpy(myIntVector->data, oldData, sizeof(int) * oldCapacity);
		if (oldData)
		{
			free(oldData);
		}
	}

	myIntVector->data[myIntVector->size] = val;
	++myIntVector->size;
}

void myPopBack(MyIntVector *myIntVector)
{
	if (myIntVector->size == 0)
	{
		exit(EXIT_FAILURE);
	}
	--myIntVector->size;
}

char *int32_reg[] = {"w9", "w10", "w11", "w12", "w13"}; //caller saved
char *int32_reg_rest[] = {"w14", "w15"};				//caller saved
char *int64_reg[] = {"x9", "x10", "x11", "x12", "x13"}; //caller saved
char *int64_reg_rest[] = {"x14", "x15"};				//caller saved
char *callee_saved_int64[] = {"x19", "x20", "x21", "x22", "x23", "x24", "x25", "x26", "x27", "x28", "x29"};
char *float_reg[] = {"s16", "s17", "s18", "s19", "s20", "s21"};
char *float_reg_rest[] = {"s22", "s23"};

int_regtab IntRegTab;
float_regtab FloatRegTab;
pseu_regtab PseuRegTab;
int PseuTabStartOff = -4;

void init_regtab()
{
	PseuRegTab.busyVector = getMyIntVector(10);
}

void reset_regtab(int maxLocalVariableOffset)
{
	memset(IntRegTab.busy, 0, sizeof(IntRegTab.busy));
	memset(FloatRegTab.busy, 0, sizeof(FloatRegTab.busy));
	memset(PseuRegTab.busyVector->data, 0, sizeof(int) * PseuRegTab.busyVector->capacity);
	PseuRegTab.busyVector->size = 0;
	PseuTabStartOff = maxLocalVariableOffset - 4;
}

int get_reg(reg_type RegType)
{
	int realTableIndex = 0;
	int realRegisterCount = (RegType == INT_REG) ? INT_REG_NUMBER : FLOAT_REG_NUMBER;
	int *realRegIsAllocated = (RegType == INT_REG) ? IntRegTab.busy : FloatRegTab.busy;
	for (realTableIndex = 0; realTableIndex < realRegisterCount; ++realTableIndex)
	{
		if (!realRegIsAllocated[realTableIndex])
		{
			realRegIsAllocated[realTableIndex] = 1;
			return realTableIndex;
		}
	}

	int pseudoTableIndex = 0;
	for (pseudoTableIndex = 0; pseudoTableIndex < PseuRegTab.busyVector->size; ++pseudoTableIndex)
	{
		if (!PseuRegTab.busyVector->data[pseudoTableIndex])
		{
			PseuRegTab.busyVector->data[pseudoTableIndex] = 1;
			return (RegType == INT_REG) ? (INT_REG_NUMBER + pseudoTableIndex) : (FLOAT_REG_NUMBER + pseudoTableIndex);
		}
	}

	myPushBack(PseuRegTab.busyVector, 1);

	return (RegType == INT_REG) ? (INT_REG_NUMBER + pseudoTableIndex) : (FLOAT_REG_NUMBER + pseudoTableIndex);
}

void free_reg(reg_type RegType, int registerIndex)
{
	int realRegisterCount = (RegType == INT_REG) ? INT_REG_NUMBER : FLOAT_REG_NUMBER;
	int *realRegIsAllocated = (RegType == INT_REG) ? IntRegTab.busy : FloatRegTab.busy;

	if (registerIndex < realRegisterCount)
	{
		//free real register
		realRegIsAllocated[registerIndex] = 0;
		if (RegType == INT_REG)
		{
			IntRegTab.is64[registerIndex] = 0;
		}
	}
	else
	{
		//free pseudo register
		int pseudoTableIndex = registerIndex - realRegisterCount;
		PseuRegTab.busyVector->data[pseudoTableIndex] = 0;
	}
}

void save_reg()
{
	int index = 0;
	int reg_offset = 8;
	for (index = 0; index < INT_REG_NUMBER; ++index)
	{
		emit("str %s, [sp, #%d]", int64_reg[index], reg_offset);
		reg_offset += 8;
	}
	for (index = 0; index < INT_REST_REG_NUMBER; ++index)
	{
		emit("str %s, [sp, #%d]", int64_reg_rest[index], reg_offset);
		reg_offset += 8;
	}
	for (index = 0; index < INT_OTHER_REG_NUMBER; ++index)
	{
		emit("str %s, [sp, #%d]", callee_saved_int64[index], reg_offset);
		reg_offset += 8;
	}

	for (index = 0; index < FLOAT_REG_NUMBER; ++index)
	{
		emit("str %s, [sp, #%d]", float_reg[index], reg_offset);
		reg_offset += 4;
	}
	for (index = 0; index < FLOAT_REST_REG_NUMBER; ++index)
	{
		emit("str %s, [sp, #%d]", float_reg_rest[index], reg_offset);
		reg_offset += 4;
	}
}

void restore_reg()
{
	int index = 0;
	int reg_offset = 8;
	for (index = 0; index < INT_REG_NUMBER; ++index)
	{
		emit("ldr %s, [sp, #%d]", int64_reg[index], reg_offset);
		reg_offset += 8;
	}
	for (index = 0; index < INT_REST_REG_NUMBER; ++index)
	{
		emit("ldr %s, [sp, #%d]", int64_reg_rest[index], reg_offset);
		reg_offset += 8;
	}
	for (index = 0; index < INT_OTHER_REG_NUMBER; ++index)
	{
		emit("ldr %s, [sp, #%d]", callee_saved_int64[index], reg_offset);
		reg_offset += 8;
	}

	for (index = 0; index < FLOAT_REG_NUMBER; ++index)
	{
		emit("ldr %s, [sp, #%d]", float_reg[index], reg_offset);
		reg_offset += 4;
	}
	for (index = 0; index < FLOAT_REST_REG_NUMBER; ++index)
	{
		emit("ldr %s, [sp, #%d]", float_reg_rest[index], reg_offset);
		reg_offset += 4;
	}
}

int pseu_reg_offset(int pseudoRegisterIndex)
{
	return PseuTabStartOff - pseudoRegisterIndex * 4;
}

int genConstant(C_type constantType, void *valuePtr)
{
	int const_label = const_label_no++;

	emit(".data");

	if (constantType == INTEGERC)
	{
		int *val = (int *)valuePtr;
		emit("_CONSTANT_%d: .word %d", const_label, *val);
		emit(".align 3");
	}
	else if (constantType == FLOATC)
	{
		float *val = (float *)valuePtr;
		emit("_CONSTANT_%d: .float %f", const_label, *val);
		emit(".align 3");
	}
	else if (constantType == STRINGC)
	{
		char *val;
		val = (char *)valuePtr;
		val[strlen(valuePtr) - 1] = '\0';
		emit("_CONSTANT_%d: .ascii %s\\000\"", const_label, val);
		emit(".align 3");
		val[strlen(valuePtr) - 1] = '"';
		val[strlen(valuePtr)] = '\0';
	}

	emit(".text");

	return const_label;
}

void codeGenSetReg(reg_type RegType, char *instruction, int reg1Index, int value)
{
	char *reg1Name = NULL;
	allocate_reg(RegType, reg1Index, 0, 0, &reg1Name);
	emit("%s %s, #%d", instruction, reg1Name, value);
	memory_store(RegType, reg1Index, reg1Name);
}

void codeGenSetReg_cond(reg_type RegType, char *instruction, int reg1Index, char *cond)
{
	char *reg1Name = NULL;
	allocate_reg(RegType, reg1Index, 0, 0, &reg1Name);
	emit("%s %s, %s", instruction, reg1Name, cond);
	memory_store(RegType, reg1Index, reg1Name);
}

void allocate_reg_64(reg_type RegType, int regIndex, int needToBeLoaded, int workRegIndexIfPseudo, char **regName)
{
	int realRegisterCount = (RegType == INT_REG) ? INT_REG_NUMBER : FLOAT_REG_NUMBER;
	char **realRegisterName = (RegType == INT_REG) ? int64_reg : float_reg;
	char **workRegisterName = (RegType == INT_REG) ? int64_reg_rest : float_reg_rest;

	if (regIndex >= realRegisterCount)
	{
		//pseudo register
		int pseudoIndex = regIndex - realRegisterCount;
		*regName = workRegisterName[workRegIndexIfPseudo];
		if (needToBeLoaded)
		{
			emit("ldr %s, [x29, #%d]", *regName, pseu_reg_offset(pseudoIndex));
		}
	}
	else
	{
		*regName = realRegisterName[regIndex];
		IntRegTab.is64[regIndex] = 1;
	}
}

void genBoolfromFloat(int boolRegIndex, int floatRegIndex)
{

	int zero = 0x0;
	int constantZeroLabelNumber = genConstant(INTEGERC, &zero);
	char *boolRegName = NULL;
	allocate_reg(INT_REG, boolRegIndex, 0, 0, &boolRegName);

	char *tmpZeroRegName = int32_reg_rest[0];
	emit("ldr %s, =_CONSTANT_%d", int64_reg_rest[1], constantZeroLabelNumber);
	emit("ldr %s, [%s,#0]", tmpZeroRegName, int64_reg_rest[1]);
	char *origFloatRegName = NULL;
	allocate_reg(FLOAT_REG, floatRegIndex, 1, 1, &origFloatRegName);
	emit("str %s, [%s,#0]", origFloatRegName, int64_reg_rest[1]);
	emit("ldr %s, [%s,#0]", boolRegName, int64_reg_rest[1]);
	emit("cmp %s, %s", tmpZeroRegName, boolRegName);

	char *reg1Name = NULL;
	emit("cset %s, ne", boolRegName);

	memory_store(INT_REG, boolRegIndex, boolRegName);
}

void allocate_reg(reg_type RegType, int regIndex, int needToBeLoaded, int workRegIndexIfPseudo, char **regName)
{
	int realRegisterCount = (RegType == INT_REG) ? INT_REG_NUMBER : FLOAT_REG_NUMBER;
	char **realRegisterName;
	char **workRegisterName;

	if (regIndex >= realRegisterCount)
	{
		//pseudo register
		realRegisterName = (RegType == INT_REG) ? int32_reg : float_reg;
		workRegisterName = (RegType == INT_REG) ? int32_reg_rest : float_reg_rest;
		int pseudoIndex = regIndex - realRegisterCount;
		*regName = workRegisterName[workRegIndexIfPseudo];
		if (needToBeLoaded)
		{
			emit("ldr %s, [x29, #%d]", *regName, pseu_reg_offset(pseudoIndex));
		}
	}
	else
	{
		if (RegType == INT_REG)
		{
			if (IntRegTab.is64[regIndex] == 1)
			{
				*regName = int64_reg[regIndex];
			}
			else
			{
				*regName = int32_reg[regIndex];
			}
		}
		else
		{
			realRegisterName = float_reg;
			workRegisterName = float_reg_rest;
			*regName = realRegisterName[regIndex];
		}
	}
}

void memory_store(reg_type RegType, int regIndex, char *regName)
{
	int realRegisterCount = (RegType == INT_REG) ? INT_REG_NUMBER : FLOAT_REG_NUMBER;
	if (regIndex >= realRegisterCount)
	{
		int pseudoIndex = regIndex - realRegisterCount;
		emit("str %s, [x29, #%d]", regName, pseu_reg_offset(pseudoIndex));
	}
}

void codeGen1Reg1ImmInstruction(reg_type RegType, char *instruction, int reg1Index, int *value)
{
	char *reg1Name = NULL;
	allocate_reg(RegType, reg1Index, 0, 0, &reg1Name);

	if (RegType == INT_REG)
	{
		emit("%s %s, #%d", instruction, reg1Name, *((int *)value));
	}

	memory_store(RegType, reg1Index, reg1Name);
}
/* destination , source , source */
void genLogical(reg_type RegType, char *instruction, int dstRegIndex, int srcReg1Index, int srcReg2Index)
{
	int boolReg1Index = -1;
	int boolReg2Index = -1;

	if (RegType == FLOAT_REG)
	{
		boolReg1Index = get_reg(INT_REG);
		boolReg2Index = get_reg(INT_REG);
		genBoolfromFloat(boolReg1Index, srcReg1Index);
		genBoolfromFloat(boolReg2Index, srcReg2Index);
	}
	else if (RegType == INT_REG)
	{
		int zero = 0;
		boolReg1Index = srcReg1Index;
		boolReg2Index = srcReg2Index;
		codeGen1Reg1ImmInstruction(INT_REG, "cmp", srcReg1Index, &zero);
		codeGenSetReg_cond(INT_REG, "cset", srcReg1Index, "ne");
		codeGen1Reg1ImmInstruction(INT_REG, "cmp", srcReg2Index, &zero);
		codeGenSetReg_cond(INT_REG, "cset", srcReg2Index, "ne");
	}

	genTriple(INT_REG, instruction, dstRegIndex, boolReg1Index, boolReg2Index);

	if (RegType == FLOAT_REG)
	{
		free_reg(INT_REG, boolReg1Index);
		free_reg(INT_REG, boolReg2Index);
	}
}

void codeGenCmp0Instruction(reg_type RegType, char *instruction, int reg1Index, int imm)
{

	char *reg1Name = NULL;
	allocate_reg(RegType, reg1Index, 0, 0, &reg1Name);
	emit("%s %s, #%d", instruction, reg1Name, imm);
	memory_store(RegType, reg1Index, reg1Name);
}

void genSimple(reg_type RegType, char *instruction, int reg1Index, int reg2Index)
{
	char *reg1Name = NULL;
	allocate_reg(RegType, reg1Index, 0, 0, &reg1Name);

	char *reg2Name = NULL;
	allocate_reg(RegType, reg2Index, 1, 1, &reg2Name);

	emit("%s %s, %s", instruction, reg1Name, reg2Name);

	memory_store(RegType, reg1Index, reg1Name);
}

void genTriple(reg_type RegType, char *instruction, int reg1Index, int reg2Index, int reg3Index)
{
	char *reg1Name = NULL;
	allocate_reg(RegType, reg1Index, 0, 0, &reg1Name);

	char *reg2Name = NULL;
	allocate_reg(RegType, reg2Index, 1, 0, &reg2Name);

	char *reg3Name = NULL;
	allocate_reg(RegType, reg3Index, 1, 1, &reg3Name);

	emit("%s %s, %s, %s", instruction, reg1Name, reg2Name, reg3Name);

	memory_store(RegType, reg1Index, reg1Name);
}

void genImmediate(reg_type RegType, char *instruction, int reg1Index, int reg2Index, void *imm)
{
	char *reg1Name = NULL;
	allocate_reg_64(RegType, reg1Index, 0, 0, &reg1Name);

	char *reg2Name = NULL;
	allocate_reg_64(RegType, reg2Index, 1, 0, &reg2Name);

	if (RegType == INT_REG)
	{
		int *val = (int *)imm;
		emit("%s %s, %s, #%d", instruction, reg1Name, reg2Name, *val);
	}
	else if (RegType == FLOAT_REG)
	{
		float *val = (float *)imm;
		emit("%s %s, %s, %f", instruction, reg1Name, reg2Name, *val);
	}

	memory_store(RegType, reg1Index, reg1Name);
}

int codeGenConvertFromIntToFloat(int intRegIndex)
{
	int floatRegisterIndex = intRegIndex;
	//emit("scvtf s%d, w%d", lreg, lreg); //Signed fixed-point convert to floating-point.
	return floatRegisterIndex;
}

int codeGenConvertFromFloatToInt(int floatRegIndex)
{
	int intRegisterIndex = floatRegIndex;
	//emit("scvtf s%d, w%d", lreg, lreg); //Signed fixed-point convert to floating-point.
	return intRegisterIndex;
}

void genGlobalVarDecl(AST_NODE *varDeclListNode)
{
	AST_NODE *declNode = varDeclListNode->child;
	while (declNode)
	{
		if (Decl_Kind(declNode) == VARIABLE_DECL)
		{
			AST_NODE *typeNode = declNode->child, *idNode = typeNode->rightSibling;
			while (idNode)
			{
				SymbolTableEntry *idSymTab = ID_SymTab(idNode);
				TypeDescriptor *idTypeDesc = idSymTab->attribute->attr.typeDescriptor;
				if (idTypeDesc->kind == SCALAR_TYPE_DESCRIPTOR)
				{
					if (idTypeDesc->properties.dataType == INT_TYPE)
					{
						emit("_g_%s: .word 0", idSymTab->name);
					}
					else if (idTypeDesc->properties.dataType == FLOAT_TYPE)
					{
						emit("_g_%s: .float 0.0", idSymTab->name);
					}
				}
				else if (idTypeDesc->kind == ARRAY_TYPE_DESCRIPTOR)
				{
					int variableSize = getVariableSize(idTypeDesc);
					emit("_g_%s: .space %d", idSymTab->name, variableSize);
				}
				idNode = idNode->rightSibling;
			}
		}
		declNode = declNode->rightSibling;
	}
	return;
}

void genFunctionDecl(AST_NODE *funcDeclNode)
{
	AST_NODE *typeNode, *idNode, *paramListNode, *blockNode;
	typeNode = funcDeclNode->child;
	idNode = typeNode->rightSibling;
	currentFuncName = ID_Name(idNode);
	paramListNode = idNode->rightSibling;
	blockNode = paramListNode->rightSibling;
	gen_head(currentFuncName);
	gen_prologue(ID_Name(idNode));
	reset_regtab(ID_SymTab(idNode)->attribute->offset);

	AST_NODE *traverseListNode = blockNode->child;
	while (traverseListNode)
	{
		gen_block(traverseListNode);
		traverseListNode = traverseListNode->rightSibling;
	}

	gen_epilogue(ID_Name(idNode));
	int frameSize = abs(ID_SymTab(idNode)->attribute->offset) +
					(INT_REG_NUMBER + INT_REST_REG_NUMBER + INT_OTHER_REG_NUMBER + FLOAT_REG_NUMBER + FLOAT_REST_REG_NUMBER) * 4 +
					PseuRegTab.busyVector->size * 4;

	while (frameSize % 8 != 0)
	{
		frameSize = frameSize + 4;
	}
	frameSize = frameSize + (INT_REG_NUMBER + INT_REST_REG_NUMBER + INT_OTHER_REG_NUMBER) * 4;
	while (frameSize % 8 != 0)
	{
		frameSize = frameSize + 4;
	}
	emit("_frameSize_%s: .word %d", ID_Name(idNode),
		 frameSize + 8);
}

void genBlock(AST_NODE *blockNode)
{
	AST_NODE *traverseListNode = blockNode->child;
	while (traverseListNode)
	{
		gen_block(traverseListNode);
		traverseListNode = traverseListNode->rightSibling;
	}
}

void genExpr(AST_NODE *exprNode)
{
	if (Expr_Kind(exprNode) == BINARY_OPERATION)
	{
		AST_NODE *leftOp = exprNode->child;
		AST_NODE *rightOp = leftOp->rightSibling;
		genExprRelated(leftOp);
		genExprRelated(rightOp);
		if (leftOp->dataType == INT_TYPE)
			{
				leftOp->Reg_No = codeGenConvertFromIntToFloat(leftOp->Reg_No);
			}
			if (rightOp->dataType == INT_TYPE)
			{
				rightOp->Reg_No = codeGenConvertFromIntToFloat(rightOp->Reg_No);
			}
		if (leftOp->dataType == FLOAT_TYPE || rightOp->dataType == FLOAT_TYPE)
		{
			



			switch (exprNode->semantic_value.exprSemanticValue.op.binaryOp){
				case BINARY_OP_ADD:
					exprNode->Reg_No = leftOp->Reg_No;
					genTriple(FLOAT_REG, "fadd", exprNode->Reg_No, leftOp->Reg_No, rightOp->Reg_No);
					break;
				case BINARY_OP_SUB:
					exprNode->Reg_No = leftOp->Reg_No;
					genTriple(FLOAT_REG, "fsub", exprNode->Reg_No, leftOp->Reg_No, rightOp->Reg_No);
					break;
				case BINARY_OP_MUL:
					exprNode->Reg_No = leftOp->Reg_No;
					genTriple(FLOAT_REG, "fmul", exprNode->Reg_No, leftOp->Reg_No, rightOp->Reg_No);
					break;
				case BINARY_OP_DIV:
					exprNode->Reg_No = leftOp->Reg_No;
					genTriple(FLOAT_REG, "fdiv", exprNode->Reg_No, leftOp->Reg_No, rightOp->Reg_No);
					break;
				case BINARY_OP_EQ:
					exprNode->Reg_No = get_reg(INT_REG);
					genSimple(FLOAT_REG, "cmp", leftOp->Reg_No, rightOp->Reg_No);
					codeGenSetReg_cond(INT_REG, "cset", exprNode->Reg_No, "eq");
					free_reg(FLOAT_REG, leftOp->Reg_No);
					break;
				case BINARY_OP_GE:
					exprNode->Reg_No = get_reg(INT_REG);
					genSimple(FLOAT_REG, "fcmp", leftOp->Reg_No, rightOp->Reg_No);
					codeGenSetReg_cond(INT_REG, "cset", exprNode->Reg_No, "ge");
					free_reg(FLOAT_REG, leftOp->Reg_No);
					break;
				case BINARY_OP_LE:
					exprNode->Reg_No = get_reg(INT_REG);
					genSimple(FLOAT_REG, "fcmp", leftOp->Reg_No, rightOp->Reg_No);
					codeGenSetReg_cond(INT_REG, "cset", exprNode->Reg_No, "le");
					free_reg(FLOAT_REG, leftOp->Reg_No);
					break;
				case BINARY_OP_NE:
					exprNode->Reg_No = get_reg(INT_REG);
					genSimple(FLOAT_REG, "fcmp", leftOp->Reg_No, rightOp->Reg_No);
					codeGenSetReg_cond(INT_REG, "cset", exprNode->Reg_No, "ne");
					free_reg(FLOAT_REG, leftOp->Reg_No);
					break;
				case BINARY_OP_GT:
					exprNode->Reg_No = get_reg(INT_REG);
					genSimple(FLOAT_REG, "fcmp", leftOp->Reg_No, rightOp->Reg_No);
					codeGenSetReg_cond(INT_REG, "cset", exprNode->Reg_No, "ge");
					free_reg(FLOAT_REG, leftOp->Reg_No);
					break;
				case BINARY_OP_LT:
					exprNode->Reg_No = get_reg(INT_REG);
					genSimple(FLOAT_REG, "fcmp", leftOp->Reg_No, rightOp->Reg_No);
					codeGenSetReg_cond(INT_REG, "cset", exprNode->Reg_No, "lt");
					free_reg(FLOAT_REG, leftOp->Reg_No);
					break;
				case BINARY_OP_AND:
					exprNode->Reg_No = get_reg(INT_REG);
					genLogical(FLOAT_REG, "and", exprNode->Reg_No, leftOp->Reg_No, rightOp->Reg_No);
					free_reg(FLOAT_REG, leftOp->Reg_No);
					break;
				case BINARY_OP_OR:
					exprNode->Reg_No = get_reg(INT_REG);
					genLogical(FLOAT_REG, "orr", exprNode->Reg_No, leftOp->Reg_No, rightOp->Reg_No);
					free_reg(FLOAT_REG, leftOp->Reg_No);
					break;
				default:
					printf("unknown binary operation\n");
					break;
				}
			
			free_reg(FLOAT_REG, rightOp->Reg_No);
		}
		else if (exprNode->dataType == INT_TYPE)
		{
			
			
			exprNode->Reg_No = leftOp->Reg_No;
			
				switch (exprNode->semantic_value.exprSemanticValue.op.binaryOp)
				{
				case BINARY_OP_ADD:
					genTriple(INT_REG, "add", exprNode->Reg_No, leftOp->Reg_No, rightOp->Reg_No);
					break;
				case BINARY_OP_SUB:
					genTriple(INT_REG, "sub", exprNode->Reg_No, leftOp->Reg_No, rightOp->Reg_No);
					break;
				case BINARY_OP_MUL:
					genTriple(INT_REG, "mul", exprNode->Reg_No, leftOp->Reg_No, rightOp->Reg_No);
					break;
				case BINARY_OP_DIV:
					genTriple(INT_REG, "sdiv", exprNode->Reg_No, leftOp->Reg_No, rightOp->Reg_No);
					break;
				case BINARY_OP_EQ:
					genSimple(INT_REG, "cmp", leftOp->Reg_No, rightOp->Reg_No);
					codeGenSetReg_cond(INT_REG, "cset", exprNode->Reg_No, "eq");
					break;
				case BINARY_OP_GE:
					genSimple(INT_REG, "cmp", leftOp->Reg_No, rightOp->Reg_No);
					codeGenSetReg_cond(INT_REG, "cset", exprNode->Reg_No, "ge");
					break;
				case BINARY_OP_LE:
					genSimple(INT_REG, "cmp", leftOp->Reg_No, rightOp->Reg_No);
					codeGenSetReg_cond(INT_REG, "cset", exprNode->Reg_No, "le");

					break;
				case BINARY_OP_NE:
					genSimple(INT_REG, "cmp", leftOp->Reg_No, rightOp->Reg_No);
					codeGenSetReg_cond(INT_REG, "cset", exprNode->Reg_No, "ne");
					break;
				case BINARY_OP_GT:
					genSimple(INT_REG, "cmp", leftOp->Reg_No, rightOp->Reg_No);
					codeGenSetReg_cond(INT_REG, "cset", exprNode->Reg_No, "gt");
					break;
				case BINARY_OP_LT:
					genSimple(INT_REG, "cmp", leftOp->Reg_No, rightOp->Reg_No);
					codeGenSetReg_cond(INT_REG, "cset", exprNode->Reg_No, "lt");
					break;
				case BINARY_OP_AND:
					genLogical(INT_REG, "and", exprNode->Reg_No, leftOp->Reg_No, rightOp->Reg_No);
					break;
				case BINARY_OP_OR:
					genLogical(INT_REG, "orr", exprNode->Reg_No, leftOp->Reg_No, rightOp->Reg_No);
					break;
				default:
					printf("unknown binary operation\n");
					break;
				}
			

			free_reg(INT_REG, rightOp->Reg_No);
		}
	} //endif BINARY_OPERATION
	else if (Expr_Kind(exprNode) == UNARY_OPERATION)
	{
		int tmpZero = 0;
		AST_NODE *operand = exprNode->child;
		genExprRelated(operand);
		if (operand->dataType == FLOAT_TYPE)
		{
			switch (exprNode->semantic_value.exprSemanticValue.op.unaryOp)
			{
			case UNARY_OP_POSITIVE:
				exprNode->Reg_No = operand->Reg_No;
				break;
			case UNARY_OP_NEGATIVE:
				exprNode->Reg_No = operand->Reg_No;
				genSimple(FLOAT_REG, "fneg", exprNode->Reg_No, exprNode->Reg_No);
				break;
			case UNARY_OP_LOGICAL_NEGATION:
				exprNode->Reg_No = get_reg(INT_REG);
				genBoolfromFloat(exprNode->Reg_No, operand->Reg_No);
				free_reg(FLOAT_REG, operand->Reg_No);
				break;
			default:
				printf("unknown binary operation\n");
				break;
			}
		}
		else if (operand->dataType == INT_TYPE)
		{
			switch (exprNode->semantic_value.exprSemanticValue.op.unaryOp)
			{
			case UNARY_OP_POSITIVE:
				exprNode->Reg_No = operand->Reg_No;
				break;
			case UNARY_OP_NEGATIVE:
				exprNode->Reg_No = operand->Reg_No;
				genSimple(INT_REG, "neg", exprNode->Reg_No, exprNode->Reg_No);
				break;
			case UNARY_OP_LOGICAL_NEGATION:
				exprNode->Reg_No = operand->Reg_No;
				codeGenCmp0Instruction(INT_REG, "cmp", exprNode->Reg_No, 0);
				codeGenSetReg(INT_REG, "mov", exprNode->Reg_No, 0);
				codeGenSetReg(INT_REG, "moveq", exprNode->Reg_No, 1);
				break;
			default:
				printf("unknown binary operation\n");
				break;
			}
		}
	}
}

void genFuncCall(AST_NODE *functionCallNode)
{
	AST_NODE *functionIdNode = functionCallNode->child;
	AST_NODE *parameterList = functionIdNode->rightSibling;
	if (strcmp(ID_Name(functionIdNode), "write") == 0)
	{
		AST_NODE *firstParameter = parameterList->child;
		genExprRelated(firstParameter);
		char *parameterRegName = NULL;
		switch (firstParameter->dataType)
		{
		case INT_TYPE:
			allocate_reg(INT_REG, firstParameter->Reg_No, 1, 0, &parameterRegName);
			emit("mov w0, %s", parameterRegName);
			emit("bl _write_int");
			free_reg(INT_REG, firstParameter->Reg_No);
			break;
		case FLOAT_TYPE:
			allocate_reg(FLOAT_REG, firstParameter->Reg_No, 1, 0, &parameterRegName);
			emit("fmov s0, %s", parameterRegName);
			emit("bl _write_float");
			free_reg(FLOAT_REG, firstParameter->Reg_No);
			break;
		case CONST_STRING_TYPE:
			allocate_reg(INT_REG, firstParameter->Reg_No, 1, 0, &parameterRegName);
			emit("mov x0, %s", parameterRegName);
			emit("bl _write_str");
			free_reg(INT_REG, firstParameter->Reg_No);
			break;
		default:
			printf("error in gen function call\n");
			printf("firstParameter->Reg_No not free\n");
			break;
		}
		return;
	}

	if (strcmp(ID_Name(functionIdNode), "read") == 0)
	{
		emit("bl _read_int");
	}
	else if (strcmp(ID_Name(functionIdNode), "fread") == 0)
	{
		emit("bl _read_float");
	}
	else
	{
		emit("bl _start_%s", ID_Name(functionIdNode));
	}

	if (ID_SymTab(functionIdNode))
	{
		if (ID_SymTab(functionIdNode)->attribute->attr.functionSignature->returnType == INT_TYPE)
		{
			functionCallNode->Reg_No = get_reg(INT_REG);
			char *returnIntRegName = NULL;
			allocate_reg(INT_REG, functionCallNode->Reg_No, 0, 0, &returnIntRegName);

			emit("mov %s, w0", returnIntRegName);

			memory_store(INT_REG, functionCallNode->Reg_No, returnIntRegName);
		}
		else if (ID_SymTab(functionIdNode)->attribute->attr.functionSignature->returnType == FLOAT_TYPE)
		{
			functionCallNode->Reg_No = get_reg(FLOAT_REG);
			char *returnfloatRegName = NULL;
			allocate_reg(FLOAT_REG, functionCallNode->Reg_No, 0, 0, &returnfloatRegName);

			emit("fmov %s, s0", returnfloatRegName);

			memory_store(INT_REG, functionCallNode->Reg_No, returnfloatRegName);
		}
	}
}

int genArrayAddr(AST_NODE *idNode)
{
	AST_NODE *traverseDim = idNode->child;
	int *sizeInEachDimension = ID_SymTab(idNode)->attribute->attr.typeDescriptor->properties.arrayProperties.sizeInEachDimension;

	genExprRelated(traverseDim);
	int linearIdxRegisterIndex = traverseDim->Reg_No;
	traverseDim = traverseDim->rightSibling;

	int dimIndex = 1;

	int shiftLeftTwoBits = 2;
	genImmediate(INT_REG, "lsl", linearIdxRegisterIndex, linearIdxRegisterIndex, &shiftLeftTwoBits);

	char *linearOffsetRegName = NULL;
	if (!isGlobalVar(ID_SymTab(idNode)))
	{
		int baseOffset = ID_SymTab(idNode)->attribute->offset;
		genImmediate(INT_REG, "add", linearIdxRegisterIndex, linearIdxRegisterIndex, &baseOffset);
		allocate_reg_64(INT_REG, linearIdxRegisterIndex, 1, 0, &linearOffsetRegName);
		emit("add %s, %s, x29", linearOffsetRegName, linearOffsetRegName);
	}
	else
	{
		emit("ldr %s,= _g_%s", int64_reg_rest[0], ID_Name(idNode));
		allocate_reg_64(INT_REG, linearIdxRegisterIndex, 1, 1, &linearOffsetRegName);
		emit("add %s, %s, %s", linearOffsetRegName, linearOffsetRegName, int64_reg_rest[0]);
	}

	memory_store(INT_REG, linearIdxRegisterIndex, linearOffsetRegName);

	return linearIdxRegisterIndex;
}

void genVarRef(AST_NODE *idNode)
{
	SymbolAttribute *idAttribute = ID_SymTab(idNode)->attribute;
	if (ID_Kind(idNode) == NORMAL_ID)
	{
		if (idNode->dataType == INT_TYPE)
		{
			idNode->Reg_No = get_reg(INT_REG);
			char *loadRegName = NULL;
			if (!isGlobalVar(ID_SymTab(idNode)))
			{
				allocate_reg(INT_REG, idNode->Reg_No, 0, 0, &loadRegName);
				emit("ldr %s, [x29, #%d]", loadRegName, idAttribute->offset);
			}
			else
			{
				emit("ldr %s, =_g_%s", int64_reg_rest[0], ID_Name(idNode));
				allocate_reg(INT_REG, idNode->Reg_No, 0, 1, &loadRegName);
				emit("ldr %s, [%s,#0]", loadRegName, int64_reg_rest[0]);
			}
			memory_store(INT_REG, idNode->Reg_No, loadRegName);
		}
		else if (idNode->dataType == FLOAT_TYPE)
		{
			idNode->Reg_No = get_reg(FLOAT_REG);
			char *loadRegName = NULL;
			if (!isGlobalVar(ID_SymTab(idNode)))
			{
				allocate_reg(FLOAT_REG, idNode->Reg_No, 0, 0, &loadRegName);
				emit("ldr %s, [x29, #%d]", loadRegName, idAttribute->offset);
			}
			else
			{
				emit("ldr %s, =_g_%s", int64_reg_rest[0], ID_Name(idNode));
				allocate_reg(FLOAT_REG, idNode->Reg_No, 0, 0, &loadRegName);
				emit("ldr %s, [%s, #0]", loadRegName, int64_reg_rest[0]);
			}
			memory_store(FLOAT_REG, idNode->Reg_No, loadRegName);
		}
	}
	else if (ID_Kind(idNode) == ARRAY_ID)
	{
		if (idNode->dataType == INT_TYPE || idNode->dataType == FLOAT_TYPE)
		{
			int elementAddressRegIndex = genArrayAddr(idNode);
			char *elementAddressRegName = NULL;
			allocate_reg_64(INT_REG, elementAddressRegIndex, 1, 0, &elementAddressRegName);

			if (idNode->dataType == INT_TYPE)
			{
				char *dstRegName = NULL;
				idNode->Reg_No = get_reg(INT_REG);
				allocate_reg(INT_REG, idNode->Reg_No, 0, 0, &dstRegName);
				emit("ldr %s, [%s, #0]", dstRegName, elementAddressRegName);
				free_reg(INT_REG, elementAddressRegIndex);

				memory_store(INT_REG, idNode->Reg_No, dstRegName);
			}
			else if (idNode->dataType == FLOAT_TYPE)
			{
				char *dstRegName = NULL;
				idNode->Reg_No = get_reg(FLOAT_REG);
				allocate_reg(FLOAT_REG, idNode->Reg_No, 0, 0, &dstRegName);

				char *elementAddressRegName = NULL;
				allocate_reg(INT_REG, elementAddressRegIndex, 1, 0, &elementAddressRegName);

				emit("ldr %s, [%s, #0]", dstRegName, elementAddressRegName);
				memory_store(FLOAT_REG, idNode->Reg_No, dstRegName);

				free_reg(INT_REG, elementAddressRegIndex);
			}
		}
	}
}

void genConstRef(AST_NODE *constantNode)
{
	C_type cType = constantNode->semantic_value.const1->const_type;
	if (cType == INTEGERC)
	{
		int tmpInt = constantNode->semantic_value.const1->const_u.intval;
		int constantLabelNumber = genConstant(INTEGERC, &tmpInt);
		constantNode->Reg_No = get_reg(INT_REG);
		char *regName = NULL;
		allocate_reg(INT_REG, constantNode->Reg_No, 0, 0, &regName);
		emit("ldr %s, _CONSTANT_%d", regName, constantLabelNumber);
		memory_store(INT_REG, constantNode->Reg_No, regName);
	}
	else if (cType == FLOATC)
	{
		float tmpFloat = constantNode->semantic_value.const1->const_u.fval;
		int constantLabelNumber = genConstant(FLOATC, &tmpFloat);
		constantNode->Reg_No = get_reg(FLOAT_REG);
		char *regName = NULL;
		allocate_reg(FLOAT_REG, constantNode->Reg_No, 0, 0, &regName);
		emit("ldr %s, =_CONSTANT_%d", int64_reg_rest[0], constantLabelNumber);
		emit("ldr %s, [%s, #0]", regName, int64_reg_rest[0]);
		memory_store(FLOAT_REG, constantNode->Reg_No, regName);
	}
	else if (cType == STRINGC)
	{
		char *tmpCharPtr = constantNode->semantic_value.const1->const_u.sc;
		int constantLabelNumber = genConstant(STRINGC, tmpCharPtr);
		constantNode->Reg_No = get_reg(INT_REG);
		char *regName = NULL;
		allocate_reg_64(INT_REG, constantNode->Reg_No, 0, 0, &regName);
		emit("ldr %s, =_CONSTANT_%d", regName, constantLabelNumber);
		memory_store(INT_REG, constantNode->Reg_No, regName);
	}
}

void genExprRelated(AST_NODE *exprRelatedNode)
{
	switch (exprRelatedNode->nodeType)
	{
	case EXPR_NODE:
		genExpr(exprRelatedNode);
		break;
	case STMT_NODE:
		genFuncCall(exprRelatedNode);
		break;
	case IDENTIFIER_NODE:
		genVarRef(exprRelatedNode);
		break;
	case CONST_VALUE_NODE:
		genConstRef(exprRelatedNode);
		break;
	default:
		printf("unknown expr related node\n");
		exprRelatedNode->dataType = ERROR_TYPE;
		break;
	}
}

void genAssign(AST_NODE *assignmentStmtNode)
{
	AST_NODE *leftOp = assignmentStmtNode->child;
	AST_NODE *rightOp = leftOp->rightSibling;
	genExprRelated(rightOp);

	if (ID_Kind(leftOp) == NORMAL_ID)
	{
		if (leftOp->dataType == INT_TYPE)
		{
			char *rightOpRegName = NULL;
			allocate_reg(INT_REG, rightOp->Reg_No, 1, 0, &rightOpRegName);
			if (!isGlobalVar(ID_SymTab(leftOp)))
			{
				emit("str %s, [x29, #%d]", rightOpRegName, ID_SymTab(leftOp)->attribute->offset);
			}
			else
			{
				int tmp_reg_index = get_reg(INT_REG);
				char *tmp_reg_name = int64_reg[tmp_reg_index];
				emit("ldr %s, =_g_%s", tmp_reg_name, ID_Name(leftOp));
				emit("str %s, [%s, #0]", rightOpRegName, tmp_reg_name);
				free_reg(INT_REG, tmp_reg_index);
			}
			leftOp->Reg_No = rightOp->Reg_No;
		}
		else if (leftOp->dataType == FLOAT_TYPE)
		{
			char *rightOpRegName = NULL;
			allocate_reg(FLOAT_REG, rightOp->Reg_No, 1, 0, &rightOpRegName);
			if (!isGlobalVar(ID_SymTab(leftOp)))
			{
				emit("str %s, [x29, #%d]", rightOpRegName, ID_SymTab(leftOp)->attribute->offset);
			}
			else
			{
				int tmp_reg_index = get_reg(INT_REG);
				char *tmp_reg_name = int64_reg[tmp_reg_index];
				emit("ldr %s, =_g_%s", tmp_reg_name, ID_Name(leftOp));
				emit("str %s, [%s, #0]", rightOpRegName, tmp_reg_name);
				free_reg(INT_REG, tmp_reg_index);
			}
			leftOp->Reg_No = rightOp->Reg_No;
		}
	}
	else if (ID_Kind(leftOp) == ARRAY_ID)
	{
		int elementAddressRegIndex = genArrayAddr(leftOp);

		char *elementAddressRegName = NULL;
		allocate_reg(INT_REG, elementAddressRegIndex, 1, 0, &elementAddressRegName);
		if (leftOp->dataType == INT_TYPE)
		{
			char *rightOpRegName = NULL;
			allocate_reg(INT_REG, rightOp->Reg_No, 1, 1, &rightOpRegName);
			emit("str %s, [%s, #0]", rightOpRegName, elementAddressRegName);

			leftOp->Reg_No = rightOp->Reg_No;
		}
		else if (leftOp->dataType == FLOAT_TYPE)
		{
			char *rightOpRegName = NULL;
			allocate_reg(FLOAT_REG, rightOp->Reg_No, 1, 0, &rightOpRegName);

			emit("str %s, [%s, #0]", rightOpRegName, elementAddressRegName);

			leftOp->Reg_No = rightOp->Reg_No;
		}

		free_reg(INT_REG, elementAddressRegIndex);
	}
}

void genTest(AST_NODE *testNode)
{
	//if (testNode->nodeType == STMT_NODE)
	//{
		//if (Stmt_Kind(testNode) == ASSIGN_STMT)
		//{
		//	genAssign(testNode);
		//}
		//else if (Stmt_Kind(testNode) == FUNCTION_CALL_STMT)
		//{
		//	genFuncCall(testNode);
		//}
	//}
	//else
	//{
		genExpr(testNode);
	//}
}

void genWhile(AST_NODE *whileNode)
{
	AST_NODE *cond = whileNode->child;

	int constantZeroLabelNumber = -1;
	if (cond->dataType == FLOAT_TYPE)
	{
		float zero = 0.0f;
		constantZeroLabelNumber = genConstant(FLOATC, &zero);
	}

	int while_label = while_label_no++;
	emit("_whileTestLabel_%d:", while_label);

	genTest(cond);

	if (cond->dataType == INT_TYPE)
	{
		char *boolRegName = NULL;
		allocate_reg(INT_REG, cond->Reg_No, 1, 0, &boolRegName);
		emit("cmp %s, #0", boolRegName);
		emit("beq _whileExitLabel_%d", while_label);
		free_reg(INT_REG, cond->Reg_No);
	}
	else if (cond->dataType == FLOAT_TYPE)
	{
		emit("ldr %s, _CONSTANT_%d", float_reg_rest[0], constantZeroLabelNumber);
		char *boolRegName = NULL;
		allocate_reg(FLOAT_REG, cond->Reg_No, 1, 1, &boolRegName);
		emit("fcmp %s, %s\n", boolRegName, float_reg_rest[0]);
		emit("beq _whileExitLabel_%d", while_label);
		free_reg(FLOAT_REG, cond->Reg_No);
	}

	AST_NODE *bodyNode = cond->rightSibling;
	gen_stmt(bodyNode);

	emit("b _whileTestLabel_%d", while_label);
	emit("_whileExitLabel_%d:", while_label);
}

void genFor(AST_NODE *forNode)
{
	int for_label = for_label_no++;
	AST_NODE *initNode, *condNode, *incrNode, *loopNode;
	initNode = forNode->child;
	condNode = initNode->rightSibling;
	incrNode = condNode->rightSibling;
	loopNode = incrNode->rightSibling;
	emit("_forStart_%d:", for_label);
	if (initNode->nodeType == NONEMPTY_ASSIGN_EXPR_LIST_NODE)
	{
		AST_NODE *assignNode = initNode->child;
		for (; assignNode != NULL; assignNode = assignNode->rightSibling)
		{
			genAssign(assignNode);
		}
	}
	emit("_forLoop_%d:", for_label);
	if (condNode->nodeType == NONEMPTY_RELOP_EXPR_LIST_NODE)
	{
		AST_NODE *exprNode = condNode->child;
		AST_NODE *last_cond;
		int cond;
		for (; exprNode != NULL; exprNode = exprNode->rightSibling)
		{
			last_cond = exprNode;
			genExprRelated(exprNode);
		}
		if (last_cond->dataType == INT_TYPE)
		{
			emit("cmp w%d, #0", cond);
		}
		else
		{
			emit("fcmp s%d, #0", cond);
		}
		free_reg(INT_REG, cond);
		emit("beq _forExit_%d", for_label);
	}
	if (loopNode->nodeType == STMT_NODE)
	{
		gen_stmt(loopNode);
	}
	else if (loopNode->nodeType == BLOCK_NODE)
	{
		gen_block(loopNode);
	}
	else if (loopNode->nodeType == NUL_NODE)
	{
		// do nothing
	}
	if (incrNode->nodeType == NONEMPTY_ASSIGN_EXPR_LIST_NODE)
	{
		AST_NODE *assignNode = incrNode->child;
		for (; assignNode != NULL; assignNode = assignNode->rightSibling)
		{
			if (assignNode->nodeType == STMT_NODE)
			{
				genAssign(assignNode);
			}
			else if (assignNode->nodeType == EXPR_NODE)
			{
				genExpr(assignNode);
			}
		}
	}
	emit("b _forLoop_%d", for_label);
	emit("_forExit_%d:", for_label);
}

void genIf(AST_NODE *ifNode)
{
	AST_NODE *condNode = ifNode->child;
	AST_NODE *thenNode = condNode->rightSibling;
	AST_NODE *elseNode = thenNode->rightSibling;
	int if_label = if_label_no++;

	emit("_IF_%d:", if_label);

	genTest(condNode);

	char *condRegName = NULL;
	allocate_reg(INT_REG, condNode->Reg_No, 1, 0, &condRegName);
	if (condNode->dataType == FLOAT_TYPE)
		emit("fcvtzs w%s, s%s", condRegName + 1, condRegName + 1);

	emit("cmp %s, #0", condRegName);
	emit("beq _elseLabel_%d", if_label);
	free_reg(INT_REG, condNode->Reg_No);
	emit("beq _elseLabel_%d", if_label);

	gen_stmt(thenNode);

	emit("b _ifExitLabel_%d", if_label);
	emit("_elseLabel_%d:", if_label);

	gen_stmt(elseNode);

	emit("_ifExitLabel_%d:", if_label);
}

void genReturn(AST_NODE *returnStmtNode)
{
	AST_NODE *returnVal = returnStmtNode->child;
	if (returnVal->nodeType != NUL_NODE)
	{
		genExprRelated(returnVal);

		char *returnValRegName = NULL;
		if (returnVal->dataType == INT_TYPE)
		{
			allocate_reg(INT_REG, returnVal->Reg_No, 1, 0, &returnValRegName);
			emit("mov w0, %s", returnValRegName);
			free_reg(INT_REG, returnVal->Reg_No);
		}
		else if (returnVal->dataType == FLOAT_TYPE)
		{
			allocate_reg(FLOAT_REG, returnVal->Reg_No, 1, 0, &returnValRegName);
			emit("fmov s0, %s", returnValRegName);
			free_reg(FLOAT_REG, returnVal->Reg_No);
		}
	}
	emit("b _end_%s", currentFuncName);
}

void gen_stmt(AST_NODE *stmtNode)
{
	if (stmtNode->nodeType == NUL_NODE)
	{
		return;
	}
	else if (stmtNode->nodeType == BLOCK_NODE)
	{
		genBlock(stmtNode);
	}
	else
	{
		switch (Stmt_Kind(stmtNode))
		{
		case ASSIGN_STMT: // 1)	Assignment	statements
			genAssign(stmtNode);
			if (stmtNode->child->dataType == INT_TYPE)
			{
				free_reg(INT_REG, stmtNode->child->Reg_No);
			}
			else if (stmtNode->child->dataType == FLOAT_TYPE)
			{
				free_reg(FLOAT_REG, stmtNode->child->Reg_No);
			}
			break;
		case WHILE_STMT: // 3)	Control	statements:	while,	if-then-else
			genWhile(stmtNode);
			break;
		case IF_STMT: // 3)	Control	statements:	while,	if-then-else
			genIf(stmtNode);
			break;
		case FOR_STMT: // 9)	For	loops
			genFor(stmtNode);
			break;
		case FUNCTION_CALL_STMT:
			genFuncCall(stmtNode);
			if (stmtNode->Reg_No != -1)
			{
				if (stmtNode->dataType == INT_TYPE)
				{
					free_reg(INT_REG, stmtNode->Reg_No);
				}
				else if (stmtNode->dataType == FLOAT_TYPE)
				{
					free_reg(FLOAT_REG, stmtNode->Reg_No);
				}
			}
			break;
		case RETURN_STMT:
			genReturn(stmtNode);
			break;
		default:
			printf("unknown stmt kind\n");
		}
	}
}

void gen_block(AST_NODE *node)
{
	AST_NODE *traverseChildren = node->child;
	switch (node->nodeType)
	{
	case VARIABLE_DECL_LIST_NODE:
		break;
	case STMT_LIST_NODE:
		while (traverseChildren)
		{
			gen_stmt(traverseChildren);
			traverseChildren = traverseChildren->rightSibling;
		}
		break;
	case NONEMPTY_ASSIGN_EXPR_LIST_NODE:
		while (traverseChildren)
		{
			genTest(traverseChildren);
			if (traverseChildren->rightSibling)
			{
				if (traverseChildren->dataType == INT_TYPE)
				{
					free_reg(INT_REG, traverseChildren->Reg_No);
				}
				else if (traverseChildren->dataType == FLOAT_TYPE)
				{
					free_reg(FLOAT_REG, traverseChildren->Reg_No);
				}
			}
			traverseChildren = traverseChildren->rightSibling;
		}
		node->Reg_No = traverseChildren->Reg_No;
		break;
	case NONEMPTY_RELOP_EXPR_LIST_NODE:
		while (traverseChildren)
		{
			genExprRelated(traverseChildren);
			if (traverseChildren->rightSibling)
			{
				if (traverseChildren->dataType == INT_TYPE)
				{
					free_reg(INT_REG, traverseChildren->Reg_No);
				}
				else if (traverseChildren->dataType == FLOAT_TYPE)
				{
					free_reg(FLOAT_REG, traverseChildren->Reg_No);
				}
			}
			traverseChildren = traverseChildren->rightSibling;
		}
		node->Reg_No = traverseChildren->Reg_No;
		break;
	case NUL_NODE:
		break;
	default:
		printf("unknown block\n");
		node->dataType = ERROR_TYPE;
		break;
	}
}

int getVariableSize(TypeDescriptor *typeDescriptor)
{
	if (typeDescriptor->kind == SCALAR_TYPE_DESCRIPTOR)
	{
		//INT_TYPE and FLOAT_TYPE
		return 4;
	}
	else if (typeDescriptor->kind == ARRAY_TYPE_DESCRIPTOR)
	{
		ArrayProperties *arrayPropertiesPtr = &(typeDescriptor->properties.arrayProperties);

		int arrayElementCount = 1;
		int index = 0;
		for (index = 0; index < arrayPropertiesPtr->dimension; ++index)
		{
			arrayElementCount *= arrayPropertiesPtr->sizeInEachDimension[index];
		}

		return arrayElementCount * 4;
	}
	else
	{
		printf("error counting the size of node\n");
		return -1;
	}
}

void generateCode(AST_NODE *root)
{
	init_regtab();
	output = fopen("output.s", "w");
	if (!output)
	{
		printf("error opening output.s!\n");
		exit(1);
	}
	AST_NODE *traverse = root->child;

	while (traverse)
	{
		if (traverse->nodeType == VARIABLE_DECL_LIST_NODE)
		{
			emit(".data");
			genGlobalVarDecl(traverse);
			emit(".text");
		}
		else if (traverse->nodeType == DECLARATION_NODE)
		{
			genFunctionDecl(traverse);
		}
		traverse = traverse->rightSibling;
	}

	fclose(output);
}