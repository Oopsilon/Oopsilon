/*!
 * The *Desc objects describe the layout of the von Neumann space of an object.
 */

#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

#include "vtrt.h"

#define info(...) printf("Runtime: " __VA_ARGS__)

struct NoDesc;
struct MemDesc;
struct ArrayDesc;
struct ClassDesc;
struct MethodDesc;

template <class T> class OopRef;
template <class DescT> class ObjectHeader;

/* clang-format off */
typedef OopRef <NoDesc>         Oop;
typedef OopRef <NoDesc>         SmiOop;
typedef OopRef <MemDesc>        MemOop;
typedef OopRef <ArrayDesc>      ArrayOop;
typedef OopRef <ClassDesc>      ClassOop;
typedef OopRef <MethodDesc>     MethodOop;
/* clang-format on */

template <class T> class OopRef {
    public:
	// friend class OopRef<OopDesc>;

	enum Tag {
		kPtr = 0,
		kSmi = 1,
	};

	ObjectHeader<T> *m_ptr;

    public:
	typedef ObjectHeader<T> PtrType;

	inline OopRef()
	    : m_ptr(NULL) {};
	inline OopRef(int64_t smi)
	    : m_ptr((ObjectHeader<T> *)VTRT_MAKESMI(smi)) {};
	inline OopRef(void *ptr)
	    : m_ptr((ObjectHeader<T> *)ptr) {};

	static inline OopRef nil() { return OopRef(); }

	inline bool isPtr() const { return VT_isPtr(m_ptr); }
	inline bool isSmi() const { return VT_isSmi(m_ptr); }
	inline bool isNil() const { return m_ptr == 0; }
	inline int64_t smi() const { return VT_intValue(m_ptr); }
	template <typename OT> inline OT &as()
	{
		return reinterpret_cast<OT &>(*this);
	}

	inline ClassOop isa();

	void print(size_t in);

	inline uint32_t hashCode()
	{
		return isSmi() ? smi() : as<MemOop>()->hashCode();
	}

	template <typename OT> inline bool operator==(const OT &other)
	{
		return !operator!=(other);
	}
	template <typename OT> inline bool operator!=(const OT &other)
	{
		return other.m_ptr != m_ptr;
	}
	ObjectHeader<T> *operator->() const { return m_ptr; }
	inline ObjectHeader<T> &operator*() const { return *m_ptr; }
	inline operator Oop() const { return m_ptr; }
};

template <class DescT> struct ObjectHeader {
	ClassOop isa;
	DescT *vns;
};

struct MemDesc {
	uintptr_t size;
	enum {
		kBytes,
		kOops,
	} kind : 8;

        union {
                Oop oops[0];
                uint8_t bytes[0];
        };
};

struct ClassDesc : public MemDesc {
	/* keep in sync with libstern/Object.st class vars */
	static const int instanceSize = 3;

	static ClassOop alloc() { return allocOopsObj(instanceSize); }

	ClassOop m_superclass;
	ArrayOop m_methodArray;
	SmiOop m_instanceSize;
};

struct ArrayDesc : public MemDesc {
        static ArrayOop create(size_t nSlots);
};


/*
 * sync libstkern/Method.st
 */
struct MethodDesc : public MemDesc {
        vtrt_method_fn_t implementation;

        static MethodOop create( vtrt_method_fn_t impl);
};

template <class T>
T
allocOopsObj(size_t nOops)
{
	ObjectHeader<MemDesc> *ote = new ObjectHeader<MemDesc>;
	if (nOops != 0) {
		ote->vns = (MemDesc *)malloc(
		    sizeof(MemDesc) + nOops * sizeof(Oop));
		ote->vns->size = nOops;
	}
	return T(ote);
}

vtrt_memoop_t
allocOopsObj(size_t nOops)
{
	return (vtrt_memoop_t)allocOopsObj<MemOop>(nOops).m_ptr;
}

struct ClassMapEntry {
	struct vtrt_classTemplate *templ;
	ClassOop cls;
	ClassOop metacls;
};

std::map<std::string, ClassMapEntry> classes;

template <class T> size_t sizeOfInstance()
{
        return (sizeof(T) - sizeof(MemDesc)) / sizeof(Oop);
}

ClassOop findClass(std::string name)
{
        return classes[name].cls;
}

ArrayOop ArrayDesc::create(size_t nSlots)
{
        ArrayOop array = allocOopsObj(nSlots);
        array->isa = findClass("Array");
        return array;
}


MethodOop MethodDesc::create(vtrt_method_fn_t  impl)
{
        MethodOop meth = allocOopsObj(sizeOfInstance<MethodDesc>());
        meth->isa = findClass("Method");
        meth->vns->implementation = impl;
        return meth;
}

void
vtrt_registerClass(const char *name, struct vtrt_classTemplate *templ)
{
	info("Registering class %s (subclasses %s)\n", name, templ->superName);

	ClassOop cls = allocOopsObj(templ->classSize),
	metacls = ClassDesc::alloc();
	metacls->vns->m_instanceSize = templ->classSize;
	cls->vns->m_instanceSize = templ->instanceSize;
        cls->isa = metacls;
	classes[name] = { templ, cls, metacls };

        metacls->vns->m_methodArray = ArrayDesc::create(templ->nClassMethods * 2);
        for (size_t i = 0; i < templ->nClassMethods; i++) {

        }
}

int
vtrt_main(int argc, char *argv[])
{
        /* link up the classes */
	for (auto &entry : classes) {
                auto & superName = entry.second.templ->superName;
                auto &cls = entry.second.cls;
                auto & metacls = entry.second.metacls;

                if (strcmp(superName, "nil") == 0) {
                        /*
                         * Object metaclass isa object metaclass,
                         * and inherits from object
                         */
                        metacls->isa = metacls;
                        metacls->vns->m_superclass = cls;
                        continue;
                }

                /* superclass not nil - need to find the superclass */

		auto super = classes.find(superName);
		if (super == classes.end()) {
			throw std::runtime_error("Class " + entry.first +
			    " specifies superclass " +
			    entry.second.templ->name +
			    ", which was not found.\n");
		}

                metacls->isa = classes["Object"].metacls;
                metacls->vns->m_superclass = super->second.metacls;
                /* cls->isa was set to metacls during registration */
                cls->vns->m_superclass = super->second.cls;
	}
	return 0;
}
