/* peryan runtime */
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

extern void PeryanMain();

/* Main function (will be separated to each platform) */
int main(int argc, char *argv[])
{
	PeryanMain();
	return 0;
}

/* In case of debugging */
 void printNum(int num) {
	printf("%d\n", num);
	return;
}

/* Begin implementation of built-in String */

struct String {
	int length;
	int capacity;
	char *str;
};

void *PRMalloc(unsigned int size)
{
	return malloc(size);
}

void *PRFree(void *ptr)
{
	return PRFree(ptr);
}

void *PRRealloc(void *ptr, int size)
{
	return realloc(ptr, size);
}

struct String *PRStringConstructorCStr(char *cStr)
{
	struct String *res = NULL;

	res = PRMalloc(sizeof(struct String));

	res->length = strlen(cStr);

	res->capacity = res->length + 1;

	res->str = PRMalloc(res->capacity);
	strcpy(res->str, cStr);

	return res;
}

struct String *PRStringConstructorInt(int num)
{
	char *str[20];
	sprintf((char *)str, "%d", num);
	return PRStringConstructorCStr((char *)str);
}

struct String *PRStringConcatenate(struct String *lhs, struct String *rhs)
{
	struct String *res = NULL;

	res = PRMalloc(sizeof(struct String));

	res->length = lhs->length + rhs->length;
	res->capacity = res->length + 1;

	res->str = PRMalloc(res->capacity);
	strcpy(res->str, lhs->str);
	strcpy(res->str + lhs->length, rhs->str);

	return res;
}

void PRStringDestructor(struct String *str)
{
	PRFree(str->str);
	PRFree(str);
}

int strlen__(struct String *str)
{
	return str->length;
}

/* End implementation of built-in String */

/*
 * Array is Peryan's sole polymorphic type so that far closer to type system itself,
 * so it should be implemented in the code generator
 *
 */

void mes(struct String *str)
{
	printf("%s\n", str->str);
	return;
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

int PRIntConstructor(struct String *str)
{
	int res;
	sscanf(str->str, "%d", &res);
	return res;
}
