/* peryan runtime */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define DBG_PRINT(TYPE, FUNC_NAME) printf("%s%s\n", #TYPE, #FUNC_NAME)
#define DBG_PRINT(TYPE, FUNC_NAME)

#include "common.h"

extern void PeryanMain();

/* Main function (will be separated to each platform) */
int main(int argc, char *argv[])
{
	DBG_PRINT(+, main);
	PeryanMain();
	DBG_PRINT(-, main);
	return 0;
}

/* In case of debugging */
 void printNum(int num) {
	DBG_PRINT(+, printNum);
	printf("%d\n", num);
	DBG_PRINT(-, printNum);
	return;
}

void *PRMalloc(unsigned int size)
{
	DBG_PRINT(+-, PRMalloc);
	return malloc(size);
}

void PRFree(void *ptr)
{
	DBG_PRINT(+, PRMalloc);
	free(ptr);
	DBG_PRINT(-, PRMalloc);
	return;
}

void *PRRealloc(void *ptr, int size)
{
	DBG_PRINT(+-, PRRealloc);
	return realloc(ptr, size);
}

/*
 * Array is Peryan's sole polymorphic type so that far closer to type system itself,
 * so it should be implemented in the code generator
 *
 */

void mes(struct String *str)
{
	DBG_PRINT(+, mes);
	printf("%s\n", str->str);
	DBG_PRINT(-, mes);
	return;
}

int PRStringCompare(struct String *lhs, struct String *rhs)
{
	return strcmp(lhs->str, rhs->str);
}

int PRIntConstructor(struct String *str)
{
	int res;
	DBG_PRINT(+, PRIntConstructor);
	sscanf(str->str, "%d", &res);
	DBG_PRINT(-, PRIntConstructor);
	return res;
}

int exec(struct String *str)
{
	DBG_PRINT(+, exec);
	int i = 0, res = 0;
	struct String *tmp = PRStringConstructorCStr(str->str);

	// TODO: dirty hack to make tester run on both Unix and Windows
#ifndef _WIN32
	for (i = 0; i < tmp->length; ++i) {
		if (tmp->str[i] == ' ')
			break;

		if (tmp->str[i] == '\\')
			tmp->str[i] = '/';
	}
#endif

	res = system(tmp->str);
	PRStringDestructor(tmp);

	DBG_PRINT(-, exec);
	return res;
}

void dirlist(struct String **res, struct String *mask, int mode)
{
	// TODO: support mode option
	// TODO: support current directory

	struct String *cmd = NULL;
	FILE *fp = NULL;
	char tmp[512];

	DBG_PRINT(+, dirlist);

	cmd = PRStringConstructorCStr("find ");
	PRStringAppend(cmd, mask);
	PRStringAppendCStr(cmd, " -maxdepth 0");

	fp = popen(cmd->str, "r");

	if (fp == NULL) {
		fprintf(stderr, "runtime error: cannot open the directory list of %s\n", mask->str);
		exit(-1);
	}

	PRStringDestructor(cmd);

	PRStringDestructor(*res);
	*res = PRStringConstructorVoid();

	while (1) {
		if (fgets(tmp, sizeof(tmp) / sizeof(tmp[0]), fp) == NULL)
			break;

		PRStringAppendCStr(*res, tmp);
	}

	pclose(fp);

	DBG_PRINT(-, dirlist);
	return;
}

void noteload(struct String *fileName)
{
	FILE *fp = NULL;
	char tmp[512];

	if (noteTarget_ == NULL) {
		fprintf(stderr, "runtime error: no buffer selected\n");
		exit(-1);
	}

	PRStringDestructor(*noteTarget_);
	*noteTarget_ = PRStringConstructorVoid();

	fp = fopen(fileName->str, "r");
	if (fp == NULL) {
		fprintf(stderr, "runtime error: cannot open the file %s\n", fileName->str);
		exit(-1);
	}

	while (1) {
		if (fgets(tmp, sizeof(tmp) / sizeof(tmp[0]), fp) == NULL)
			break;

		PRStringAppendCStr(*noteTarget_, tmp);
	}

	fclose(fp);
	return;
}

