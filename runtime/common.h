#include <math.h>
#include <assert.h>

/* aims it will be the last dependency on C standard library */
#include <stdarg.h>

extern void PeryanMain();

void *PRMalloc(unsigned int size);
void PRFree(void *ptr);
void *PRRealloc(void *ptr, int size);

void AbortWithErrorMessage(const char *format, ...);

int stat = 0;

/* Begin implementation of built-in String */

struct String {
	int length;
	int capacity;
	char *str;
};

struct String *PRStringConstructorCStr(char *cStr)
{
	int i = 0;
	struct String *res = NULL;
	DBG_PRINT(+, PRStringConstructorCStr);

	res = PRMalloc(sizeof(struct String));
	if (res == NULL)
		AbortWithErrorMessage("runtime error: failed to allocate memory");

	for (res->length = 0; cStr[res->length] != 0; )
		res->length++;

	res->capacity = res->length + 1;

	res->str = PRMalloc(res->capacity);
	if (res->str == NULL)
		AbortWithErrorMessage("runtime error: failed to allocate memory");

	for (i = 0; (res->str[i] = cStr[i]) != 0; ++i) ;

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
	int i = 0;
	struct String *res = NULL;
	DBG_PRINT(+, PRStringConcatenate);

	res = PRMalloc(sizeof(struct String));
	if (res == NULL)
		AbortWithErrorMessage("runtime error: failed to allocate memory");

	res->length = lhs->length + rhs->length;
	res->capacity = res->length + 1;

	res->str = PRMalloc(res->capacity);
	if (res->str == NULL)
		AbortWithErrorMessage("runtime error: failed to allocate memory");

	for (i = 0; (res->str[i] = lhs->str[i]) != 0; ++i) ;
	for (i = 0; (res->str[i + lhs->length] = rhs->str[i]) != 0; ++i) ;

	DBG_PRINT(-, PRStringConcatenate);
	return res;
}

void PRStringAppendCStr(struct String *lhs, const char *rhs) {
	int rhsLength = 0;
	int i = 0;

	DBG_PRINT(+, PRStringAppendCStr);

	for (rhsLength = 0; rhs[rhsLength] != 0; )
		rhsLength++;

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
	int sameLen = 0;
	while (lhs->str[sameLen] != 0 && lhs->str[sameLen] == rhs->str[sameLen])
		++sameLen;

	if (lhs->str[sameLen] == rhs->str[sameLen]) {
		return 0;
	} else if (lhs->str[sameLen] > rhs->str[sameLen]) {
		return 1;
	} else { // lhs->str[sameLen] < rhs->str[sameLen])
		return -1;
	}
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
	if (res == NULL)
		AbortWithErrorMessage("runtime error: failed to allocate memory");

	res->length = length;
	res->capacity = res->length + 1;

	res->str = PRMalloc(res->capacity);
	if (res->str == NULL)
		AbortWithErrorMessage("runtime error: failed to allocate memory");
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
		AbortWithErrorMessage("runtime error: no buffer selected");
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
		AbortWithErrorMessage("runtime error: unknown noteinfo option %d", option);
		exit(-1);
	}
}

void noteget(struct String** res, int idx)
{
	int begin = -1, end = -1, cnt = 0, i = 0;
	struct String tmp;

	DBG_PRINT(+, noteget);
	if (noteTarget_ == NULL) {
		AbortWithErrorMessage("runtime error: no buffer selected");
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
		AbortWithErrorMessage("runtime error: invalid index %d for noteget", idx);
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

