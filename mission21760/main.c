#include "RemoveLeftRecursion.h"
#include <string.h>
#include <stdlib.h>

const char* VoidSymbol = "$"; // "ε"
const char* Postfix = "'";

char rule_table_ci[20][256];
char ruleNameArr[20][64];

int main(int argc, char* argv[])
{
	//
	// 调用 InitRules 函数初始化文法
	//
	
#ifdef CODECODE_CI
	Rule* pHead = InitRules_CI();  	// 此行代码在线上流水线运行
#else
	Rule* pHead = InitRules();		// 此行代码在 CP Lab 中运行
#endif

	//
	// 输出消除左递归之前的文法
	//
	printf("Before Remove Left Recursion:\n");
	PrintRule(pHead);

	//
	// 调用 RemoveLeftRecursion 函数消除文法中的左递归
	//
	RemoveLeftRecursion(pHead);
	
	//
	// 输出消除左递归之后的文法
	//
	printf("\nAfter Remove Left Recursion:\n");
	PrintRule(pHead);
	
	FreeRule(pHead);
	return 0;
}

/*
功能：
	判断当前 Rule 中的一个 Symbol 是否需要被替换。
	如果 Symbol 是一个非终结符，且 Symbol 对应的
	Rule 在当前 Rule 之前，就需要被替换。

参数：
	pCurRule -- 当前 Rule 的指针。
	pSymbol -- Symbol 指针。
	
返回值：
	需要替换返回 1。
	不需要替换返回 0。
*/
int SymbolNeedReplace(const Rule* pHead, const Rule* pCurRule, const RuleSymbol* pSymbol)
{
	if(pSymbol->isToken==1) return 0;
	Rule* pTmpRule=pHead;
	while(pTmpRule!=pCurRule){
		if(pTmpRule==pSymbol->pRule) return 1;
		pTmpRule=pTmpRule->pNextRule;
	}
	return 0;
}

/*
功能：
	拷贝一个 Symbol。

参数：
	pSymbolTemplate -- 需要被拷贝的 Symbol 指针。
	  
返回值：
	拷贝获得的新 Symbol 的指针。
*/
RuleSymbol* CopySymbol(const RuleSymbol* pSymbolTemplate)
{
	RuleSymbol* pNewSymbol=CreateSymbol();
	pNewSymbol->pNextSymbol=pSymbolTemplate->pNextSymbol;
	pNewSymbol->pOther=pSymbolTemplate->pOther;
	pNewSymbol->isToken=pSymbolTemplate->isToken;
	strcpy(pNewSymbol->TokenName,pSymbolTemplate->TokenName);
	pNewSymbol->pRule=pSymbolTemplate->pRule;
	return pNewSymbol;
}

/*
功能：
	拷贝一个 Select。

参数：
	pSelectTemplate -- 需要被拷贝的 Select 指针。
	  
返回值：
	拷贝获得的新 Select 的指针。
*/
RuleSymbol* CopySelect(const RuleSymbol* pSelectTemplate)
{
	RuleSymbol* pNewSymbol=CopySymbol(pSelectTemplate);
	RuleSymbol* pCurSymbol=pNewSymbol;
	while(pCurSymbol->pNextSymbol){
		pCurSymbol->pNextSymbol=CopySymbol(pCurSymbol->pNextSymbol);
		pCurSymbol=pCurSymbol->pNextSymbol;
	}
	return pNewSymbol;
}

/*
功能：
	替换一个 Select 的第一个 Symbol。

参数：
	pSelectTemplate -- 需要被替换的 Select 指针。
	  
返回值：
	替换后获得的新 Select 的指针。
	注意，替换后可能会有一个新的 Select，
	也可能会有多个 Select 链接在一起。
*/
RuleSymbol* ReplaceSelect(const RuleSymbol* pSelectTemplate)
{
	RuleSymbol* pNewSelect=CopySelect(pSelectTemplate->pRule->pFirstSymbol);
	RuleSymbol* pCurSelect=pNewSelect;
	while(pCurSelect->pOther){
		pCurSelect->pOther=CopySelect(pCurSelect->pOther);
		RuleSymbol* pRestSelect=CopySelect(pSelectTemplate->pNextSymbol);
		AddSymbolToSelect(pCurSelect,pRestSelect);
		pCurSelect=pCurSelect->pOther;
	}
	RuleSymbol* pRestSelect=CopySelect(pSelectTemplate->pNextSymbol);
	AddSymbolToSelect(pCurSelect,pRestSelect);
	pCurSelect->pOther=pSelectTemplate->pOther;
	return pNewSelect;
}

/*
功能：
	释放一个 Select 的内存。

参数：
	pSelect -- 需要释放的 Select 的指针。
*/
void FreeSelect(RuleSymbol* pSelect)
{
	if(pSelect==NULL) return ;
    FreeSelect(pSelect->pNextSymbol);
	free(pSelect);
    return ;
}

/*
功能：
	释放所有 Rule 的内存。

参数：
	pRule -- 所有 Rule 的头指针。
*/
void FreeRule(Rule* pRule)
{
	if(pRule==NULL) return ;
    FreeRule(pRule->pNextRule);
	RuleSymbol *pCurSelect=pRule->pFirstSymbol,*pNxtSelect;
	while(pCurSelect){
		pNxtSelect=pCurSelect->pOther;
		FreeSelect(pCurSelect);
		pCurSelect=pNxtSelect;
	}
	free(pRule);
    return ;
}

/*
功能：
	判断一条 Rule 是否存在左递归。

参数：
	prRule -- Rule 指针。
	  
返回值：
	存在返回 1。
	不存在返回 0。
*/
int RuleHasLeftRecursion(Rule* pRule)
{

	RuleSymbol *pSelect=pRule->pFirstSymbol;
	while(pSelect){
		if(pSelect->isToken==0&&pSelect->pRule==pRule) return 1;
		pSelect=pSelect->pOther;
	}
	return 0;
}

/*
功能：
	将一个 Symbol 添加到 Select 的末尾。

参数：
	pSelect -- Select 指针。
	pNewSymbol -- Symbol 指针。
*/
void AddSymbolToSelect(RuleSymbol* pSelect, RuleSymbol* pNewSymbol)
{
	while(pSelect->pNextSymbol!=NULL) pSelect=pSelect->pNextSymbol;
	pSelect->pNextSymbol=pNewSymbol;
	return ;
}

/*
功能：
	将一个 Select 加入到文法末尾，当 Select 为 NULL 时就将一个ε终结符加入到文法末尾。

参数：
	pRule -- 文法指针。
	pNewSelect -- Select 指针。
*/
void AddSelectToRule(Rule* pRule, RuleSymbol* pNewSelect)
{

	RuleSymbol** pTmpSelect=&(pRule->pFirstSymbol);
    while((*pTmpSelect)!=NULL) pTmpSelect=&((*pTmpSelect)->pOther);
    (*pTmpSelect)=pNewSelect;
    return ;
}

/*
功能：
	消除左递归。

参数：
	pHead -- 文法链表的头指针。
*/
void RemoveLeftRecursion(Rule* pHead)
{
	Rule* pRule;				// Rule 游标
	RuleSymbol* pSelect; 		// Select 游标
	Rule* pNewRule;			  	// Rule 指针
	int isChange;				// Rule 是否被替换的标记
	RuleSymbol **pSelectPrePtr; // Symbol 指针的指针
	
	for(pRule = pHead; pRule != NULL; pRule = pRule->pNextRule)
	{
		//
		// 替换
		//
		do
		{
			isChange = 0;

			// 在 Rule 的所有 Select 中查找是否需要替换
			for(pSelect = pRule->pFirstSymbol, pSelectPrePtr = &pRule->pFirstSymbol;
				pSelect != NULL;
				pSelectPrePtr = &pSelect->pOther, pSelect = pSelect->pOther)
			{
				if(SymbolNeedReplace(pHead, pRule, pSelect)) // 判断 Select 的第一个 Symbol 是否需要替换
				{
					isChange = 1;

					// 调用 ReplaceSelect 函数，替换 Select 的第一个 Symbol 后得到新的 Selects
					RuleSymbol* pNewSelects = ReplaceSelect(pSelect);

					// 使用新的 Selects 替换原有的 Select，并调用 FreeSelect 函数释放原有的 Select 内存
					*pSelectPrePtr=pNewSelects;			
					FreeSelect(pSelect);
					break;
				}

				if(isChange)
					break;
			}
		}while(isChange);

		// 忽略没有左递归的 Rule;
		if(!RuleHasLeftRecursion(pRule))
			continue;

		//
		// 消除左递归
		//
		pNewRule = CreateRule(pRule->RuleName); // 创建新 Rule
		strcat(pNewRule->RuleName, Postfix);

		pSelect = pRule->pFirstSymbol; // 初始化 Select 游标
		pSelectPrePtr = &pRule->pFirstSymbol;
		while(pSelect != NULL) // 循环处理所有的 Select
		{
			if(0 == pSelect->isToken && pSelect->pRule == pRule) // Select 存在左递归
			{
				// 移除包含左递归的 Select，将其转换为右递归后添加到新 Rule 的末尾，并移动游标
				
				RuleSymbol* pNxtSymbol=pSelect->pNextSymbol;
				RuleSymbol* pNewSymbol=CreateSymbol();
				pNewSymbol->isToken=0;
				pNewSymbol->pRule=pNewRule;
				AddSymbolToSelect(pNxtSymbol,pNewSymbol);
				AddSelectToRule(pNewRule,pNxtSymbol);
				RuleSymbol* pFreeSymbol=pSelect;
				pSelect=pSelect->pOther;
				(*pSelectPrePtr)=pSelect;
				free(pFreeSymbol);
				
			}
			else // Select 不存在左递归
			{
				// 在没有左递归的 Select 末尾添加指向新 Rule 的非终结符，并移动游标
				
				RuleSymbol* pNewSymbol=CreateSymbol();
				pNewSymbol->isToken=0;
				pNewSymbol->pRule=pNewRule;
				AddSymbolToSelect(pSelect,pNewSymbol);
				pSelectPrePtr=&pSelect->pOther;
				pSelect=pSelect->pOther;
			}
		}

		RuleSymbol *pNewSymbol=CreateSymbol();
		pNewSymbol->isToken=1;
		strcpy(pNewSymbol->TokenName,VoidSymbol);
		AddSelectToRule(pNewRule,pNewSymbol);
		pNewRule->pNextRule=pRule->pNextRule;
		pRule->pNextRule=pNewRule;
		pRule = pNewRule;
	}
}

/*
功能：
	使用给定的数据初始化文法链表

返回值：
	Rule 指针
*/
typedef struct _SYMBOL
{
	int isToken;
	char Name[MAX_STR_LENGTH];
}SYMBOL;

typedef struct _RULE_ENTRY
{
	char RuleName[MAX_STR_LENGTH];
	SYMBOL Selects[64][64];
}RULE_ENTRY;

static const RULE_ENTRY rule_table[] =
{
	/* A -> Ba | Aa | c */
	{ "A", 
			{
				{ { 0, "B" }, { 1, "a"} },
				{ { 0, "A" }, { 1, "a"} },
				{ { 1, "c" } }
			}	
	},

	/* B -> Bb | Ab | d */
	{ "B", 
			{
				{ { 0, "B" }, { 1, "b"} },
				{ { 0, "A" }, { 1, "b"} },
				{ { 1, "d" } }
			}	
	}
};

/*
功能：
	初始化文法链表。
	
返回值：
	文法的头指针。
*/
Rule* InitRules()
{
	Rule *pHead, *pRule;
	RuleSymbol **pSymbolPtr1, **pSymbolPtr2;
	int nRuleCount = sizeof(rule_table) / sizeof(rule_table[0]);
	int i, j, k;

	Rule** pRulePtr = &pHead;
	for (i=0; i<nRuleCount; i++)
	{
		*pRulePtr = CreateRule(rule_table[i].RuleName);
		pRulePtr = &(*pRulePtr)->pNextRule;
	}

	pRule = pHead;
	for (i=0; i<nRuleCount; i++)
	{
		pSymbolPtr1 = &pRule->pFirstSymbol;
		for (j=0; rule_table[i].Selects[j][0].Name[0] != '\0'; j++)
		{
			pSymbolPtr2 = pSymbolPtr1;
			for (k=0; rule_table[i].Selects[j][k].Name[0] != '\0'; k++)
			{
				const SYMBOL* pSymbol = &rule_table[i].Selects[j][k];

				*pSymbolPtr2 = CreateSymbol();
				(*pSymbolPtr2)->isToken = pSymbol->isToken;
				if (1 == pSymbol->isToken)
				{
					strcpy((*pSymbolPtr2)->TokenName, pSymbol->Name);
				}
				else
				{
					(*pSymbolPtr2)->pRule = FindRule(pHead, pSymbol->Name);
					if (NULL == (*pSymbolPtr2)->pRule)
					{
						printf("Init rules error, miss rule \"%s\"\n", pSymbol->Name);
						exit(1);
					}
				}
				
				pSymbolPtr2 = &(*pSymbolPtr2)->pNextSymbol;
			}
			
			pSymbolPtr1 = &(*pSymbolPtr1)->pOther;
		}

		pRule = pRule->pNextRule;
	}

	return pHead;
}

/*
功能：
	初始化文法链表(在执行流水线时调用)。
	
返回值：
	文法的头指针。
*/
Rule* InitRules_CI()
{
	int nRuleCount = 0;
	for (int i = 0; i < 20; i++)
	{
		gets(rule_table_ci[i]);	
		int length = strlen(rule_table_ci[i]);
		if (length == 0)
		{
			break;
		}
		
		for (int j = 0; j < length; j++)
		{
			if (rule_table_ci[i][j] == ' ')
			{
				ruleNameArr[i][j] = '\0';
				break;
			}
			ruleNameArr[i][j]= rule_table_ci[i][j];
		}	  
		
		nRuleCount++;
	}
			
	Rule *pHead, *pRule;
	RuleSymbol **pSymbolPtr1, **pSymbolPtr2;
		
	int i, j, k;

	Rule** pRulePtr = &pHead;
	for (i=0; i<nRuleCount; i++)
	{
		*pRulePtr = CreateRule(ruleNameArr[i]);
		pRulePtr = &(*pRulePtr)->pNextRule;
	}

	pRule = pHead;
	for (i=0; i<nRuleCount; i++)
	{
		pSymbolPtr1 = &pRule->pFirstSymbol;
		
		int start = 0;
		for (int j=0; rule_table_ci[i][j] != '\0'; j++)
		{
			if (rule_table_ci[i][j] == ' '
			 && rule_table_ci[i][j + 1] == '-'
			&& rule_table_ci[i][j + 2] == '>' 
			&& rule_table_ci[i][j + 3] == ' ')
			{
				start = j + 4;
				break;
			}
		}
			
		for (k = start; rule_table_ci[i][k] != '\0'; k++)
		{
			if (rule_table_ci[i][k] == '|')
			{
				pSymbolPtr1 = &(*pSymbolPtr1)->pOther;
				pSymbolPtr2 = pSymbolPtr1;
				continue;
			}
			if (rule_table_ci[i][k] == ' ')
			{
				continue;
			}
			if (k == start)
			{
				pSymbolPtr2 = pSymbolPtr1;
			}

			*pSymbolPtr2 = CreateSymbol();
			
			char tokenName[MAX_STR_LENGTH] = {};
			tokenName[0] = rule_table_ci[i][k];
			tokenName[1] = '\0';
			(*pSymbolPtr2)->isToken = 1;
			for (int m = 0; m < nRuleCount; m++)
			{
				if (strcmp(tokenName, ruleNameArr[m]) == 0)
				{
					(*pSymbolPtr2)->isToken = 0;
					(*pSymbolPtr2)->pRule = FindRule(pHead, tokenName);
					if (NULL == (*pSymbolPtr2)->pRule)
					{
						printf("Init rules error, miss rule \"%s\"\n", tokenName);
						exit(1);
					}
				}
			}
			if ((*pSymbolPtr2)->isToken == 1)
			{
				strcpy((*pSymbolPtr2)->TokenName, tokenName);
			}
			
			pSymbolPtr2 = &(*pSymbolPtr2)->pNextSymbol;
			
		}
			
		pRule = pRule->pNextRule;
	}

	return pHead;
}

/*
功能：
	创建一个新的 Rule。

参数：
	pRuleName -- 文法的名字。
	
返回值：
	Rule 指针
*/
Rule* CreateRule(const char* pRuleName)
{
	Rule* pRule = (Rule*)malloc(sizeof(Rule));

	strcpy(pRule->RuleName, pRuleName);
	pRule->pFirstSymbol = NULL;
	pRule->pNextRule = NULL;

	return pRule;
}

/*
功能：
	创建一个新的 Symbol。
	
返回值：
	RuleSymbol 指针	
*/
RuleSymbol* CreateSymbol()
{
	RuleSymbol* pSymbol = (RuleSymbol*)malloc(sizeof(RuleSymbol));

	pSymbol->pNextSymbol = NULL;
	pSymbol->pOther = NULL;
	pSymbol->isToken = -1;
	pSymbol->TokenName[0] = '\0';
	pSymbol->pRule = NULL;

	return pSymbol;
}

/*
功能：
	根据 RuleName 在文法链表中查找名字相同的文法。

参数：
	pHead -- 文法的头指针。
	RuleName -- 文法的名字。
	
返回值：
	Rule 指针	
*/
Rule* FindRule(Rule* pHead, const char* RuleName)
{
	Rule* pRule;
	for (pRule = pHead; pRule != NULL; pRule = pRule->pNextRule)
	{
		if (0 == strcmp(pRule->RuleName, RuleName))
		{
			break;
		}
	}
	
	return pRule;
}	

/*
功能：
	输出文法。

参数：
	pHead -- 文法的头指针。
*/
void PrintRule(Rule* pHead)
{
	
	Rule* pTmpRule=pHead;
    while(pTmpRule!=NULL){
        printf("%s->",pTmpRule->RuleName);
        RuleSymbol *pTmpSelect=pTmpRule->pFirstSymbol;
        while(pTmpSelect!=NULL){
            RuleSymbol *pTmpSymbol=pTmpSelect;
            while(pTmpSymbol!=NULL){
                printf("%s",pTmpSymbol->isToken?pTmpSymbol->TokenName:pTmpSymbol->pRule->RuleName);
                pTmpSymbol=pTmpSymbol->pNextSymbol;
            }
            pTmpSelect=pTmpSelect->pOther;
            if(pTmpSelect) printf("%s","|");
        }
        printf("\n");
        pTmpRule=pTmpRule->pNextRule;
    }
	return ;
}