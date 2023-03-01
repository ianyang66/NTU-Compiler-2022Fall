#ifndef __GENCODE_H__
#define __GENCODE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "header.h"
#include "symbolTable.h"

#define ID_Name(idNode) (idNode->semantic_value.identifierSemanticValue.identifierName)
#define ID_Kind(idNode) (idNode->semantic_value.identifierSemanticValue.kind)
#define ID_SymTab(idNode) (idNode->semantic_value.identifierSemanticValue.symbolTableEntry)

#define Decl_Kind(declNode) (declNode->semantic_value.declSemanticValue.kind)
#define Stmt_Kind(stmtNode) (stmtNode->semantic_value.stmtSemanticValue.kind)
#define Expr_Kind(exprNode) (exprNode->semantic_value.exprSemanticValue.kind)



#define INT_REG_NUMBER 5
#define INT_REST_REG_NUMBER 2
#define INT_OTHER_REG_NUMBER 11
#define FLOAT_REG_NUMBER 6
#define FLOAT_REST_REG_NUMBER 2

void emit(char *fmt, ...);
void gen_head(char *name);
void gen_prologue(char *name);
void gen_epilogue(char *name);

extern int offset;
extern int largest;

void reset_offset();
void set_offset(SymbolAttribute *attribute);

int isGlobalVar(SymbolTableEntry *symbolTableEntry);

typedef struct MyIntVector
{
	int *data;
	int size;
	int capacity;
} MyIntVector;

MyIntVector *getMyIntVector(int initialCapacity);
void myPushBack(MyIntVector *myIntVector, int val);
void myPopBack(MyIntVector *myIntVector);

extern char *int32_reg[];
extern char *int32_reg_rest[];
extern char *int64_reg[];
extern char *int64_reg_rest[];
extern char *float_reg[];
extern char *float_reg_rest[];

typedef struct int_regtab
{
	int busy[INT_REG_NUMBER];
	int is64[INT_REG_NUMBER];
} int_regtab;

typedef struct float_regtab
{
	int busy[FLOAT_REG_NUMBER];
} float_regtab;

typedef struct pseu_regtab
{
	MyIntVector *busyVector;
} pseu_regtab;

typedef enum reg_type
{
	INT_REG,
	FLOAT_REG
} reg_type;

extern int_regtab IntRegTab;
extern float_regtab FloatRegTab;
extern pseu_regtab PseuRegTab;
extern int PseuTabStartOff;

void init_regtab();

void reset_regtab(int maxLocalVariableOffset);

int get_reg(reg_type RegType);
void free_reg(reg_type RegType, int registerIndex);
void save_reg();
void restore_reg();
int pseu_reg_offset(int pseudoRegisterIndex);

int genConstant(C_type constantType, void *valuePtr);
void genBoolfromFloat(int boolRegIndex, int floatRegIndex);
void allocate_reg(reg_type RegType, int regIndex, int needToBeLoaded, int workRegIndexIfPseudo, char **regName);
void memory_store(reg_type RegType, int regIndex, char *regName);
void genLogical(reg_type RegType, char *instruction, int dstRegIndex, int srcReg1Index, int srcReg2Index);
void genSimple(reg_type RegType, char *instruction, int reg1Index, int reg2Index);
void genTriple(reg_type RegType, char *instruction, int reg1Index, int reg2Index, int reg3Index);
void genImmediate(reg_type RegType, char *instruction, int reg1Index, int reg2Index, void *imm);
void codeGenPrepareRegister(reg_type processorType, int regIndex, int isAddr, int needToBeLoaded, int workRegIndexIfPseudo, char** regName);

void generateCode(AST_NODE *root);
void genGlobalVarDecl(AST_NODE *varaibleDeclListNode);
void genFunctionDecl(AST_NODE *functionDeclNode);

void gen_block(AST_NODE *node);
int getVariableSize(TypeDescriptor *typeDescriptor);
void gen_stmt(AST_NODE *stmtNode);
void genBlock(AST_NODE *blockNode);
void genWhile(AST_NODE *whileNode);
void genFor(AST_NODE *forStmtNode);
void genIf(AST_NODE *ifStmtNode);
void genReturn(AST_NODE *returnStmtNode);
void genTest(AST_NODE *testNode);
void genAssign(AST_NODE *assignmentStmtNode);
void genExprRelated(AST_NODE *exprRelatedNode);
void genExpr(AST_NODE *exprNode);
void genFuncCall(AST_NODE *functionCallNode);
void genVarRef(AST_NODE *idNode);
void genConstRef(AST_NODE *constantNode);
int genArrayAddr(AST_NODE *idNode);

#endif
