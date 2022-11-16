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
	/* remoteAccess should only be set by Scopes themselves. */
	virtual Variable *lookup(std::string name, bool forWrite = false,
	    bool remoteAccess = false);
};

/*
 * Method or block.
 */
class CodeScope : public Scope {
    public:
	std::vector<Variable> arguments, locals, heapvars;
	std::vector<Scope*> usingHeapvarsFrom;

	void addLocal(std::string name);
	void addArg(std::string name);
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
	void visitIdentExpr(AST::IdentExprNode *node);
};

#endif /* ANALYSE_H_ */
