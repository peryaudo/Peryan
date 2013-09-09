/* peryan runtime */

//#define DBG_PRINT(TYPE, FUNC_NAME) printf("%s%s\n", #TYPE, #FUNC_NAME)
#define DBG_PRINT(TYPE, FUNC_NAME)

#include "common.h"

#include <stdio.h>
#include <stdlib.h>

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


void AbortWithErrorMessage(const char *format, ...) {
	va_list arg;
	va_start(arg, format);
	vfprintf(stderr, format, arg);
	fprintf(stderr, "\n");
	va_end(arg);
	exit(1);
	return;
}

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

void mes(struct String *str)
{
	DBG_PRINT(+, mes);
	printf("%s\n", str->str);
	DBG_PRINT(-, mes);
	return;
}

int exec(struct String *str)
{
	DBG_PRINT(+, exec);
#ifndef _WIN32
	int i = 0;
#endif
	int res = 0;
	struct String *tmp = NULL;

	tmp = PRStringConstructorCStr(str->str);

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
		AbortWithErrorMessage("runtime error: no buffer selected");
		exit(-1);
	}

	PRStringDestructor(*noteTarget_);
	*noteTarget_ = PRStringConstructorVoid();

	fp = fopen(fileName->str, "r");
	if (fp == NULL) {
		AbortWithErrorMessage("runtime error: cannot open the file %s", fileName->str);
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


