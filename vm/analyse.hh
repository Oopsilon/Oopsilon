/*!
 * Analysis leading to code generation. (there will probably be a separate
 * analyser for type checking?)
 */

#ifndef ANALYSE_H_
#define ANALYSE_H_

#include <stack>

#include "ast.hh"

class Scope;
class InstanceScope;

struct Variable {
	std::string name;
	Scope *scope;
	enum Kind {
		kArgument,
		kLocal,
		kHeapvar,
		kInlinedBlockLocal,

		kInstanceVariable,

		kNamespaceMember,

		/* pseudos */
		kSelf,
	} kind;

	/*
	 * for local/argument, read remoetly?
	 * (i.e. at least one unoptimised block context in between)?
	 */
	enum {
		kNone,
		kReadRemotely,
		kWrittenRemotely,
	} remoteAccess = kNone;

	/* for kind = kInlinedBlockLocal, its true variable */
	Variable *real = NULL;

	void markRemoteAccess(bool isRemotelyAccessed, bool isWritten);
};

struct InstanceVariable : public Variable {
	/* index, 0-based */
	size_t index;

	InstanceVariable(std::string aName, size_t index, InstanceScope *aScope)
	    : index(index)
	{
		kind = kInstanceVariable;
		name = aName;
		scope = (Scope *)aScope;
	};
};

class NamespaceMemberVariable : public Variable {
	/*
	 * Node instantiating it, if applicable. Either a ClassNode or a
	 * SharedVariableDeclNode.
	 */
	AST::Node *node;

    public:
	NamespaceMemberVariable(std::string aName, AST::Node *node)
	    : node(node)
	{
		kind = kNamespaceMember;
		name = aName;
		kind = kNamespaceMember;
	}

	AST::ClassNode *klass() { return dynamic_cast<AST::ClassNode *>(node); }
	bool isClass() { return klass() != NULL; };
};

class Scope {
    public:
	enum Kind {
		kMethod,
		kBlock,
		kOptimisedBlock,
		kClass,
		kNamespace,
	} kind;
	Scope *lexicalOuter;

	Scope(Scope *outerScope, Kind kind)
	    : lexicalOuter(outerScope)
	    , kind(kind) {};

	/* only for kMethod scopes. whether a heap context is needed. */
	bool needsHeapContext = false;

	virtual void addLocal(std::string name) { throw 0; }
	virtual void addArg(std::string name) { throw 0; }
	virtual void addInlinedBlockLocal(std::string name, bool isArg = false)
	{
		throw 0;
	}
	/* remoteAccess should only be set by Scopes themselves. */
	virtual Variable *lookup(std::string name, bool forWrite = false,
	    bool remoteAccess = false);

	/*
	 * return the real code scope (method or non-inlined block) this scope
	 * is ultimately associated with
	 */
	virtual CodeScope *realScope();
	bool inOptimisedBlock() { return kind == kOptimisedBlock; }
};

/*
 * Instance scope. There's two per class: one is the scope of instance methods,
 * the other of class methods.
 */
class InstanceScope : public Scope {
    public:
	AST::ClassNode *cls;
	Variable selfVar;
	std::vector<InstanceVariable> instanceVars;

	InstanceScope(AST::ClassNode *cls)
	    : cls(cls)
	    , Scope(NULL, kClass)
	{
		// TODO(ASAP): refactor
		selfVar.name = "self";
		selfVar.kind = Variable::kSelf;
		selfVar.scope = this;
	};

	void addIvar(std::string name, int index);
	Variable *lookup(std::string name, bool forWrite = false,
	    bool remoteAccess = false);
};

/*
 * A namespace scope. Namespaces serve the purpose of both
 */
class NamespaceScope : public Scope {
	std::string name;
	NamespaceScope *parent;
	std::vector<NamespaceMemberVariable> members;

    public:
	NamespaceScope(std::string name, NamespaceScope *parent)
	    : name(name)
	    , parent(parent)
	    , Scope(NULL, kNamespace) {};

	void addClass(AST::ClassNode *klass);
	NamespaceMemberVariable *lookup(std::string name, bool forWrite = false,
	    bool remoteAccess = false);
};

/*
 * Method or block.
 */
class CodeScope : public Scope {
    public:
	std::vector<Variable> arguments, locals, heapvars;
	/* variables from earlier scopes which must be copied in this scope */
	std::vector<Variable *> copyingVars;
	/* scopes whose heapvars must be passed to this scope */
	std::vector<Scope *> usingHeapvarsFrom;
	/* if this is a kMethod scope, the class in which the method is found */
	AST::ClassNode *klass;

	/* Scope name, used for e.g. struct declarations in the C generator. */
	std::string name;

	void addLocal(std::string name);
	void addArg(std::string name);
	void addInlinedBlockLocal(std::string name, bool isArg);
	void moveRemotelyAccessedToHeapvars();

	Variable *lookup(std::string name, bool forWrite = false,
	    bool remoteAccess = false);

	/*
	 * Create a new CodeScope; if it is a method scope, then klass should be
	 * given the defining class.
	 */
	CodeScope(Scope *outerScope, Kind kind, AST::ClassNode *klass = NULL)
	    : Scope(outerScope, kind) {};
};

class RegistrarVisitor : public AST::Visitor {
	void visitClass(AST::ClassNode *node);
};

class LinkupVisitor : public AST::Visitor {
	void visitClass(AST::ClassNode *node);
};

class AnalysisVisitor : public AST::Visitor {
	AST::ClassNode *methodClass;
	AST::MethodNode *method;
	std::stack<Scope *> scopeStack;

	void visitClass(AST::ClassNode *node);
	void visitMethod(AST::MethodNode *node);

	void visitParameterDecl(AST::VarDecl &node);
	void visitLocalDecl(AST::VarDecl &node);

	//  void visitReturnStmt(AST::ReturnStmtNode *node);
	//  void visitExprStmt(AST::ExprStmtNode *node);
	void visitBlockExpr(AST::BlockExprNode *node);
	void visitInlinedBlockExpr(AST::BlockExprNode *node);
	//  void visitCascadeExpr(AST::CascadeExprNode *node);
	void visitMessageExpr(AST::MessageExprNode *node);
	void visitAssignExpr(AST::AssignExprNode *node);
	void visitIdentExpr(AST::IdentExprNode *node);
	//  void visitIntExpr(AST::IntExprNode *node);
};

class ClosureAnalysisVisitor : public AST::Visitor {
	/* current scope */
	Scope *scope;

	void visitMethod(AST::MethodNode *node);

	void visitReturnStmt(AST::ReturnStmtNode *node);

	void visitBlockExpr(AST::BlockExprNode *node);
	void visitInlinedBlockExpr(AST::BlockExprNode *node);
	void visitIdentExpr(AST::IdentExprNode *node);
};

#endif /* ANALYSE_H_ */
