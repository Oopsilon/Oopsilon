#include <iostream>

#include "ast.hh"
#include "generate.hh"

void
CodeGeneratorVisitor::visitClass(AST::ClassNode *node)
{
	std::cout << "Visiting class called " << node->m_name << "\n";
        AST::Visitor::visitClass(node);

}

void
CodeGeneratorVisitor::visitMethod(AST::MethodNode *node)
{
	std::cout << "Visiting method called " << node->m_selector << "\n";
        AST::Visitor::visitMethod(node);
}

void
CodeGeneratorVisitor::visitReturnStmt(AST::ReturnStmtNode *node)
{
	std::cout << "Visiting return stmt\n";
        AST::Visitor::visitReturnStmt(node);
}

void CodeGeneratorVisitor::visitBlockLocalReturn(AST::ExprNode *node)
{
	std::cout << "Visiting block local return stmt\n";
        AST::Visitor::visitBlockLocalReturn(node);
}


void
CodeGeneratorVisitor::visitBlockExpr(AST::BlockExprNode *node)
{
	std::cout << "Visiting block expression:\n";
        AST::Visitor::visitBlockExpr(node);
}

void
CodeGeneratorVisitor::visitMessageExpr(AST::MessageExprNode *node)
{
        std::cout <<"Visiting message expression #" << node->selector << "\n";
        AST::Visitor::visitMessageExpr(node);

}

void CodeGeneratorVisitor::visitIdentExpr(AST::IdentExprNode *node)
{
        std::cout << "Visiting identifier expression <" << node->id << ">\n";
}

void
CodeGeneratorVisitor::visitIntExpr(AST::IntExprNode *node)
{
	std::cout << "Visiting integer literal " << node->num << "\n";
}
