#ifndef ANALYSE_H_
#define ANALYSE_H_

#include <stack>

#include "ast.hh"

struct Scope;

struct Variable {
	std::string name;
	Scope *scope;
	enum Kind {
		kArgument,
		kLocal,
		kHeapvar,
		kInlinedBlockLocal,
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

class Scope {
    public:
	enum Kind {
		kMethod,
		kBlock,
		kOptimisedBlock,
	} kind;
	Scope *outerScope;

	Scope(Scope *outerScope, Kind kind)
	    : outerScope(outerScope)
	    , kind(kind) {};

	virtual void addLocal(std::string name) { throw 0; }
	virtual void addArg(std::string name) { throw 0; }
	virtual void addInlinedBlockLocal(std::string name) { throw 0; }
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
 * Method or block.
 */
class CodeScope : public Scope {
    public:
	std::vector<Variable> arguments, locals, heapvars;
	/* variables from earlier scopes which must be copied in this scope */
	std::vector<Variable *> copyingVars;
	/* scopes whose heapvars must be passed to this scope */
	std::vector<Scope*> usingHeapvarsFrom;

	void addLocal(std::string name);
	void addArg(std::string name);
	void addInlinedBlockLocal(std::string name);
	void moveRemotelyAccessedToHeapvars();

	Variable *lookup(std::string name, bool forWrite = false,
	    bool remoteAccess = false);

	CodeScope(Scope *outerScope, Kind kind)
	    : Scope(outerScope, kind) {};
};

class AnalysisVisitor : public AST::Visitor {
	std::stack<Scope *> scopeStack;

	// void visitClass(AST::ClassNode *node);
	void visitMethod(AST::MethodNode *node);

	void visitParameterDecl(AST::VarDecl &node);
	void visitLocalDecl(AST::VarDecl &node);

	//  void visitReturnStmt(AST::ReturnStmtNode *node);
	//  void visitExprStmt(AST::ExprStmtNode *node);
	void visitBlockExpr(AST::BlockExprNode *node);
	void visitInlinedBlockExpr(AST::BlockExprNode *node);
	//  void visitCascadeExpr(AST::CascadeExprNode *node);
	//  void visitMessageExpr(AST::MessageExprNode *node);
	void visitAssignExpr(AST::AssignExprNode *node);
	void visitIdentExpr(AST::IdentExprNode *node);
	//  void visitIntExpr(AST::IntExprNode *node);
};

class ClosureAnalysisVisitor : public AST::Visitor {
	/* current scope */
	Scope *scope;

	void visitMethod(AST::MethodNode *node);

	void visitParameterDecl(AST::VarDecl &node);
	void visitLocalDecl(AST::VarDecl &node);

	void visitBlockExpr(AST::BlockExprNode *node);
	void visitInlinedBlockExpr(AST::BlockExprNode *node);
	void visitIdentExpr(AST::IdentExprNode *node);
};

#endif /* ANALYSE_H_ */
