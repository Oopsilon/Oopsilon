#ifndef GENERATE_H_
#define GENERATE_H_

#include "ast.hh"

class CodeGeneratorVisitor : public AST::Visitor {
	void visitClass(AST::ClassNode *node);
	void visitMethod(AST::MethodNode *node);
	void visitReturnStmt(AST::ReturnStmtNode *node);
        void visitBlockLocalReturn(AST::ExprNode *node);
	// void visitExprStmt(AST::ExprStmtNode *node);
	void visitBlockExpr(AST::BlockExprNode *node);
	// void visitCascadeExpr(AST::CascadeExprNode *node);
	void visitMessageExpr(AST::MessageExprNode *node);
	// void visitAssignExpr(AST::AssignExprNode *node);
	void visitIdentExpr(AST::IdentExprNode *node);
	void visitIntExpr(AST::IntExprNode *node);
};

#endif /* GENERATE_H_ */
