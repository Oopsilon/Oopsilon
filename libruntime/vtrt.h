#ifndef VTRT_H_
#define VTRT_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uintptr_t SmiOop;

typedef struct {
	int stuff
} MemDesc;

typedef MemDesc * MemOop;

typedef union {
	MemOop ptr;
	SmiOop value;
} Oop;

typedef Oop ProcessOop;

/*!
 * @name templates
 * @{
 */

struct vtrt_methodArray {
	const char *name;
	void(*function);
};

struct vtrt_symbolReference {
	const char *string;
	Oop ref;
};

struct vtrt_classTemplate {
        const char *name;
        struct vtrt_methodArray *classMethods;
        struct vtrt_methodArray *instanceMethods;
        struct vtrt_symbolReference *symbolReferences;
};

/*!
 * @} (templates)
 */


#define __VTRT_CONTEXT_MEMBERS \
	Oop self;

enum vtrt_contextFlags {
        kContextShouldReturn = 1,
};

void allocOopsObj(size_t bytes);
Oop (*msgLookup(Oop receiver, Oop selector))(void * __sender, Oop __self,...);
Oop vtrt_return(volatile void * context, Oop value);
bool vtrt_isTrue(Oop value);
Oop makeSMI(uintptr_t value);

#endif /* VTRT_H_ */
