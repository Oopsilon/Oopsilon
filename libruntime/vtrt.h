#ifndef VTRT_H_
#define VTRT_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VT_tagBits 3
#define VT_tagMask 7
#define VT_tag(x) (((intptr_t)x) & VT_tagMask)
#define VT_isPtr(x) (!VT_tag (x))
#define VT_isSmi(x) (VT_tag (x) == 1)
#define VT_intValue(x) (((intptr_t)x) >> VT_tagBits)
#define VTRT_MAKESMI(iVal) ((void *) (((iVal) << VT_tagBits) | 1))

typedef uintptr_t vtrt_smi_t;

struct vtrt_objectHeader {

};

typedef struct vtrt_objectHeader * vtrt_memoop_t;

typedef union {
	vtrt_memoop_t ptr;
	vtrt_smi_t value;
} oop;

typedef oop process_oop;

typedef oop (*vtrt_method_fn_t)(void * __sender, oop __self,...);

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
	oop ref;
};

struct vtrt_classTemplate {
	/* fully-qualified name */
        const char *name;
	/* fully-qualified superclass' name */
	const char *superName;
        struct vtrt_methodArray *instanceMethods;
        struct vtrt_methodArray *classMethods;
        struct vtrt_symbolReference *symbolReferences;
	size_t nInstanceMethods;
	size_t nClassMethods;
	size_t nSymbolReferences;
	size_t instanceSize;
	size_t classSize;
};

/*!
 * @} (templates)
 */


#define __VTRT_CONTEXT_MEMBERS \
	Oop self;

enum vtrt_contextFlags {
        kContextShouldReturn = 1,
};

void vtrt_registerClass(const char *name, struct vtrt_classTemplate *templ);
int vtrt_main(int argc, char *argv[]);

vtrt_memoop_t allocOopsObj(size_t nOops);
oop (*msgLookup(oop receiver, oop selector))(void * __sender, oop __self,...);
oop vtrt_return(volatile void * context, oop value);
bool vtrt_isTrue(oop value);
oop makeSMI(uintptr_t value);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VTRT_H_ */
