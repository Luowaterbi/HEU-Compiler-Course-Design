#include "NFAToDFA.h"
#include "RegexpToPost.h"
#include "PostToNFA.h"
#include "NFAStateStack.h"
#include "NFAFragmentStack.h"
#include "OutputResult.h"
#include <stdlib.h>
#include <string.h>

NFAFragmentStack FragmentStack; // 栈。用于储存 NFA 片段
NFAStateStack StateStack;		// 栈。用于储存 NFA 状态

const char VoidTrans = '$'; // 表示空转换


//char* regexp = "a(a|1)*";			 // 例 1
// char* regexp = "(aa|b)*a(a|bb)*"; // 例 2
 char* regexp = "(a|b)*a(a|b)?"; 	 // 例 3

char regexp_ci[256];

int main(int argc, char **argv)
{
	char *post;
	DFA* dfa = (DFA*)malloc(sizeof(DFA));
	dfa->length = 0;
	
	//
	// 初始化栈
	//
	InitNFAFragmentStack(&FragmentStack);
	
	// 在 CP Lab中执行程序时不会使用下面宏定义包含的代码，提交作业后，在线上运行流水线时会包含宏定义这部分代码。
	// 下面宏定义代码会在运行流水线时从input.txt文件中读取正则表达式，进行算例的验证。
	// 其中，input1.txt ～ input3.txt文件中包含的正则表达式与例1 ～ 例3的正则表达式是对应的。	
#ifdef CODECODE_CI
	scanf("%255s", regexp_ci);
	regexp = regexp_ci;  	 
#endif	
	
	//
	// 调用 re2post 函数将正则表达式字符串转换成解析树的后序遍历序列
	//
	post = re2post(regexp);
	
	//
	// 调用 post2dfa 函数将解析树的后序遍历序列转换为 DFA 
	//
	dfa = post2dfa(dfa, post);
	
	//
	// 将 DFA 打印输出
	//
	OutputResult(dfa);

	FreeDFA(dfa);
	FreeNFA();
	return 0;
}

/*
功能：
	创建一个 DFA 状态的转换。
	
参数：
	TransformChar -- 转换符号。
	NFAStateArray -- NFA 状态指针数组。
	Count -- 数组元素个数。	
	  
返回值：
	 Transform 结构体指针。
*/
Transform* CreateDFATransform(char TransformChar, NFAState** NFAStateArray, int Count)
{
	int i;
	Transform* pTransform = (Transform*)malloc(sizeof(Transform));
	
	for (i=0; i<Count; i++)
	{
		pTransform->NFAlist[i] = NFAStateArray[i];
	}
	
	pTransform->NFAStateCount = Count;	
	pTransform->TransformChar = TransformChar;
	pTransform->DFAStateIndex = -1;
	pTransform->NextTrans = NULL;
	
	return pTransform;
}

/*
功能：
	创建一个 DFA 状态。
	
参数：
	pTransform -- DFA 状态转换指针。	
	  
返回值：
	 DFAState 结构体指针。
*/
DFAState* CreateDFAState(Transform* pTransform)
{
	int i;
	DFAState* pDFAState = (DFAState*)malloc(sizeof(DFAState));
	
	for (i=0; i<pTransform->NFAStateCount; i++)
	{
		pDFAState->NFAlist[i] = pTransform->NFAlist[i];
	}

	pDFAState->NFAStateCount = pTransform->NFAStateCount;
	pDFAState->firstTran = NULL;

	return pDFAState;
}

/*
功能：
	判断一个转换中的 NFA 状态集合是否为某一个 DFA 状态中 NFA 状态集合的子集。
	
参数：
	pDFA -- DFA 指针。
	pTransform -- DFA 状态转换指针。	
	  
返回值：
	 如果存在返回 DFA 状态下标，不存在返回 -1。
*/
int NFAStateIsSubset(DFA* pDFA, Transform* pTransform)
{
	for(int i=0;i<pDFA->length;i++){
		int pTmpCount=0;
		DFAState *pTmpDFA=pDFA->DFAlist[i];
		for(int k=0;k<pTransform->NFAStateCount;k++)
			for(int j=0;j<pTmpDFA->NFAStateCount;j++){
				if(pTmpDFA->NFAlist[j]==pTransform->NFAlist[k]){
					++pTmpCount;
					break;
				}
		}
		if(pTmpCount==pTransform->NFAStateCount) return i;
	}
	return -1;
}

/*
功能：
	判断某个 DFA 状态的转换链表中是否已经存在一个字符的转换。
	
参数：
	pDFAState -- DFAState 指针。
	TransformChar -- 转换标识符。
	  
返回值：
	 Transform 结构体指针。
*/
Transform* IsTransformExist(DFAState* pDFAState, char TransformChar)
{
	Transform *pTmpTransform=pDFAState->firstTran;
	while(pTmpTransform){
		if(pTmpTransform->TransformChar==TransformChar) return pTmpTransform;  
		pTmpTransform=pTmpTransform->NextTrans;
	}
	return NULL;
}

/*
功能：
	将一个 NFA 集合合并到一个 DFA 转换中的 NFA 集合中。
	注意，合并后的 NFA 集合中不应有重复的 NFA 状态。
	
参数：
	NFAStateArray -- NFA 状态指针数组，即待加入的 NFA 集合。
	Count -- 待加入的 NFA 集合中元素个数。
	pTransform -- 转换指针。
*/
void AddNFAStateArrayToTransform(NFAState** NFAStateArray, int Count, Transform* pTransform)
{
	for(int i=0;i<Count;i++){
		int Exist=0;
		for(int j=0;j<pTransform->NFAStateCount;j++)
			if(pTransform->NFAlist[j]==NFAStateArray[i]){
				Exist=1;
				break;
			}
		if(!Exist) pTransform->NFAlist[pTransform->NFAStateCount++]=NFAStateArray[i];
	}
	return ;
}

/*
功能：
	使用二叉树的先序遍历算法求一个 NFA 状态的ε-闭包。
	
参数：
	State -- NFA 状态指针。从此 NFA 状态开始求ε-闭包。
	StateArray -- NFA 状态指针数组。用于返回ε-闭包。
	Count -- 元素个数。	用于返回ε-闭包中 NFA 状态的个数。
*/
void DFS(NFAState* State, NFAState** StateArray, int* Count){
	StateArray[(*Count)++]=State;
	if(State->Transform!=VoidTrans) return ;
	if(State->Next1) DFS(State->Next1,StateArray,Count);
	if(State->Next2) DFS(State->Next2,StateArray,Count);
}
void Closure(NFAState* State, NFAState** StateArray, int* Count)
{
	DFS(State,StateArray,Count);
}

/*
功能：
	将解析树的后序遍历序列转换为 DFA。

参数：
	pDFA -- DFA 指针。
	postfix -- 正则表达式的解析树后序遍历序列。
	  
返回值：
	DFA 指针。
*/
NFAState* Start = NULL;
DFA* post2dfa(DFA* pDFA, char *postfix)
{
	int i, j;								// 游标
	Transform* pTFCursor;  					// 转换指针
	NFAState* NFAStateArray[MAX_STATE_NUM]; // NFA 状态指针数组。用于保存ε-闭包
	int Count = 0;							// ε-闭包中元素个数
	
	//
	// 调用 post2nfa 函数将解析树的后序遍历序列转换为 NFA 并返回开始状态
	//
	Start = post2nfa(postfix);
	
	//
	// 调用 Closure 函数构造开始状态的ε-闭包
	//
	Closure(Start, NFAStateArray, &Count);

	// 调用 CreateDFATransform 函数创建一个转换(忽略转换字符)，
	// 然后，调用 CreateDFAState 函数，利用刚刚创建的转换新建一个 DFA 状态
	Transform* pTransform = CreateDFATransform('\0', NFAStateArray, Count);
	DFAState* pDFAState = CreateDFAState(pTransform);

	// 将 DFA 状态加入到 DFA 状态线性表中
	pDFA->DFAlist[pDFA->length++] = pDFAState;

	// 遍历线性表中所有 DFA 状态
	for(i=0; i<pDFA->length; i++)
	{
		// 遍历第 i 个 DFA 状态中的所有 NFA 状态
		for(j=0; j<pDFA->DFAlist[i]->NFAStateCount; j++)
		{
			NFAState* NFAStateTemp = pDFA->DFAlist[i]->NFAlist[j];

			// 如果 NFA 状态是接受状态或者转换是空转换，就跳过此 NFA 状态
			if(NFAStateTemp->Transform == VoidTrans || NFAStateTemp->AcceptFlag == 1)
				continue;

			// 调用 Closure 函数构造 NFA 状态的ε-闭包
			Count=0;
			Closure(NFAStateTemp->Next1, NFAStateArray, &Count);

			// 调用 IsTransfromExist 函数判断当前 DFA 状态的转换链表中是否已经存在该 NFA 状态的转换
			pTransform = IsTransformExist(pDFA->DFAlist[i], NFAStateTemp->Transform);
			if(pTransform == NULL)
			{
				// 不存在，调用 CreateDFATransform 函数创建一个转换，并将这个转换插入到转换链表的开始位置
				pTransform=CreateDFATransform(NFAStateTemp->Transform,NFAStateArray,Count);
				pTransform->NextTrans=pDFA->DFAlist[i]->firstTran;
				pDFA->DFAlist[i]->firstTran=pTransform;
			}
			else
			{
				//存在，调用 AddNFAStateArrayToTransform 函数把ε-闭包合并到已存在的转换中
				AddNFAStateArrayToTransform(NFAStateArray,Count,pTransform);
			}
		}

		// 遍历 DFA 状态的转换链表，根据每个转换创建对应的 DFA 状态
		for(pTFCursor = pDFA->DFAlist[i]->firstTran; pTFCursor != NULL; pTFCursor = pTFCursor->NextTrans)
		{
			// 调用 NFAStateIsSubset 函数判断转换中的 NFA 状态集合是否为某一个 DFA 状态中 NFA 状态集合的子集
			int Index = NFAStateIsSubset(pDFA, pTFCursor);
			if(Index == -1)
			{
				// 不是子集，调用 CreateDFAState 函数创建一个新的 DFA 状态并加入 DFA 线性表中
				// 将转换的 DFAStateIndex 赋值为新加入的 DFA 状态的下标
				pDFAState=CreateDFAState(pTFCursor);
				pTFCursor->DFAStateIndex=pDFA->length;
				pDFA->DFAlist[pDFA->length++] = pDFAState;
				
			}
			else
			{
				// 是子集，将转换的 DFAStateIndex 赋值为 Index
				pTFCursor->DFAStateIndex=Index;
			}
		}
	}

	return pDFA;
}

void FreeDFA(DFA* pDFA){
	for(int i=0;i<pDFA->length;i++){
		Transform* nxt;
		for(Transform* j=pDFA->DFAlist[i]->firstTran;j;j=nxt){
			nxt=j->NextTrans;
			free(j);
		}
		free(pDFA->DFAlist[i]);
	}
	free(pDFA);
}