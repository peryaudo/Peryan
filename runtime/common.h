#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

void *PRMalloc(unsigned int size);
void PRFree(void *ptr);
void *PRRealloc(void *ptr, int size);

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

int stat = 0;

/* Begin implementation of built-in String */

struct String {
	int length;
	int capacity;
	char *str;
};

struct String *PRStringConstructorCStr(char *cStr)
{
	struct String *res = NULL;
	DBG_PRINT(+, PRStringConstructorCStr);

	res = PRMalloc(sizeof(struct String));

	res->length = strlen(cStr);

	res->capacity = res->length + 1;

	res->str = PRMalloc(res->capacity);
	strcpy(res->str, cStr);

	DBG_PRINT(-, PRStringConstructorCStr);
	return res;
}

struct String *PRStringConstructorVoid()
{
	DBG_PRINT(+-, PRStringConstructorVoid);
	return PRStringConstructorCStr("");
}

struct String *PRStringConstructorInt(int num)
{
	char *str[20];
	DBG_PRINT(+-, PRStringConstructorInt);
	sprintf((char *)str, "%d", num);
	return PRStringConstructorCStr((char *)str);
}

struct String *PRStringConcatenate(struct String *lhs, struct String *rhs)
{
	struct String *res = NULL;
	DBG_PRINT(+, PRStringConcatenate);

	res = PRMalloc(sizeof(struct String));

	res->length = lhs->length + rhs->length;
	res->capacity = res->length + 1;

	res->str = PRMalloc(res->capacity);
	strcpy(res->str, lhs->str);
	strcpy(res->str + lhs->length, rhs->str);

	DBG_PRINT(-, PRStringConcatenate);
	return res;
}

void PRStringAppendCStr(struct String *lhs, const char *rhs) {
	int rhsLength = 0;
	int i = 0;

	DBG_PRINT(+, PRStringAppendCStr);
	rhsLength = strlen(rhs);

	while(!(lhs->length + rhsLength < lhs->capacity)) {
		lhs->capacity *= 2;
	}

	lhs->str = PRRealloc(lhs->str, lhs->capacity);

	for (i = 0; i < rhsLength; ++i) {
		lhs->str[lhs->length + i] = rhs[i];
	}

	lhs->length += rhsLength;

	lhs->str[lhs->length] = 0;

	DBG_PRINT(-, PRStringAppendCStr);
	return;
}

void PRStringAppend(struct String *lhs, struct String *rhs)
{
	int i = 0;
	DBG_PRINT(+, PRStringAppend);

	while (!(lhs->length + rhs->length < lhs->capacity)) {
		lhs->capacity *= 2;
	}

	lhs->str = PRRealloc(lhs->str, lhs->capacity);
	
	for (i = 0; i < rhs->length; ++i) {
		lhs->str[lhs->length + i] = rhs->str[i];
	}

	lhs->length = lhs->length + rhs->length;

	lhs->str[lhs->length] = 0;

	DBG_PRINT(-, PRStringAppend);
	return;
}

void PRStringDestructor(struct String *str)
{
	DBG_PRINT(+, PRStringDestructor);
	PRFree(str->str);
	PRFree(str);
	DBG_PRINT(-, PRStringDestructor);
}

int PRStringCompare(struct String *lhs, struct String *rhs)
{
	return strcmp(lhs->str, rhs->str);
}

/* End implementation of built-in String */

int PRIntConstructor(struct String *str)
{
	int res;
	DBG_PRINT(+, PRIntConstructor);
	sscanf(str->str, "%d", &res);
	DBG_PRINT(-, PRIntConstructor);
	return res;
}

int strlen_(struct String *str)
{
	DBG_PRINT(+-, strlen_);
	return str->length;
}

struct String *strmid(struct String *str, int start, int length)
{
	int i = 0;
	struct String *res = NULL;

	res = PRMalloc(sizeof(struct String));

	res->length = length;
	res->capacity = res->length + 1;

	res->str = PRMalloc(res->capacity);
	for (i = 0; i < length; ++i) {
		res->str[i] = str->str[start + i];
	}
	res->str[length] = 0;

	return res;
}

/* extern instr :: String -> Int -> String -> Int */
int instr(struct String *haystack, int start, struct String *needle)
{
	/* TODO: there's famous faster algorithm called KMP */

	int i = 0, j = 0, res = -1;
	for (i = start; i < haystack->length; ++i) {
		res = i;

		for (j = 0; j < needle->length; ++j) {
			if (i + j >= haystack->length)
				break;

			if (haystack->str[i + j] != needle->str[j]) {
				res = -1;
				break;
			}
		}

		if (res != -1) {
			return res;
		}
	}

	return -1;
}

struct String **noteTarget_ = NULL;

void notesel(struct String** noteTarget)
{
	noteTarget_ = noteTarget;
	return;
}

int noteinfo(int option)
{
	int i = 0;
	int res = 0;

	if (noteTarget_ == NULL) {
		fprintf(stderr, "runtime error: no buffer selected\n");
		exit(-1);
	}

	if (option == 0) {
		res = 0;
		for (i = 0; i < (*noteTarget_)->length; ++i) {
			if ((*noteTarget_)->str[i] == '\r') {
				res++;
				i++;
			} else if ((*noteTarget_)->str[i] == '\n') {
				res++;
			}
		}

		if ((*noteTarget_)->str[(*noteTarget_)->length - 1] != '\n')
			res++;

		return res;
	} else if (option == 1) {
		return (*noteTarget_)->length;
	} else {
		fprintf(stderr, "runtime error: unknown noteinfo option %d\n", option);
		exit(-1);
	}
}

void noteget(struct String** res, int idx)
{
	int begin = -1, end = -1, cnt = 0, i = 0;
	struct String tmp;

	DBG_PRINT(+, noteget);
	if (noteTarget_ == NULL) {
		fprintf(stderr, "runtime error: no buffer selected\n");
		exit(-1);
	}

	cnt = 0;
	for (i = 0; i < (*noteTarget_)->length; ++i) {
		if (cnt == idx && begin == -1)
			begin = i;
		if ((*noteTarget_)->str[i] == '\r') {
			if (cnt == idx && begin != -1)
				end = i;
			cnt++;
			i++;
		} else if ((*noteTarget_)->str[i] == '\n') {
			if (cnt == idx && begin != -1)
				end = i;
			cnt++;
		}
	}

	if (begin == -1) {
		fprintf(stderr, "runtime error: invalid index %d for noteget\n", idx);
		exit(-1);
	}
	if (end == -1)
		end = (*noteTarget_)->length;

	tmp.length = end - begin;
	tmp.capacity = 0;
	tmp.str = (*noteTarget_)->str + begin;

	PRStringDestructor(*res);
	*res = PRStringConstructorVoid();
	PRStringAppend(*res, &tmp);

	DBG_PRINT(-, noteget);
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

/*
 * Array is Peryan's sole polymorphic type so that far closer to type system itself,
 * so it should be implemented in the code generator
 *
 */

int rnd(int maxRange) {
	static unsigned int x = 123456789;
	static unsigned int y = 362436069;
	static unsigned int z = 521288629;
	static unsigned int w = 88675123;
	unsigned int t;

	t = x ^ (x << 11);
	x = y; y = z; z = w;
	w = (w ^ (w >> 19)) ^ (t ^ (t >> 8));
	return w % maxRange;
}

double sqrt_(double x) {
	return sqrt(x);
}

double sin_(double x) {
	return sin(x);
}

double cos_(double x) {
	return cos(x);
}

