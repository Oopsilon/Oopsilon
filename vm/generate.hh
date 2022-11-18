#ifndef GENERATE_H_
#define GENERATE_H_

#include <map>
#include <sstream>
#include <stack>

#include "analyse.hh"
#include "ast.hh"

class CodeGeneratorVisitor : public AST::Visitor {
	std::stack<std::stringstream> funStack;
	std::vector<std::string> funcs;
	std::stringstream types;

	std::stack<Scope *> scope;

	/* we map code scope pointers to unique names */
	std::map<Scope *, std::string> scopeNames;

	std::string &nameForScope(Scope *scope);
	std::string heapvarsNameForScope(Scope *scope);
	void genHeapvarsType(CodeScope *scope);
	void genMoveArgumentsToHeapvars(CodeScope *scope,
	    std::stringstream &stream);

	void emitVariableAccess(Scope *scope, Variable *var,
	    std::stringstream &stream);

	std::stringstream &fun() { return funStack.top(); };

	void visitClass(AST::ClassNode *node);
	void visitMethod(AST::MethodNode *node);
	void visitReturnStmt(AST::ReturnStmtNode *node);
	void visitBlockLocalReturn(AST::ExprNode *node);
	void visitExprStmt(AST::ExprStmtNode *node);
	void visitBlockExpr(AST::BlockExprNode *node);
	void visitInlinedBlockExpr(AST::BlockExprNode *node);
	// void visitCascadeExpr(AST::CascadeExprNode *node);
	void visitMessageExpr(AST::MessageExprNode *node);
	void visitAssignExpr(AST::AssignExprNode *node);
	void visitIdentExpr(AST::IdentExprNode *node);
	void visitIntExpr(AST::IntExprNode *node);
};

#endif /* GENERATE_H_ */
