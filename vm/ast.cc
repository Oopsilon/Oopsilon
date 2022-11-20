#include <iostream>

#include "ast.hh"

namespace AST {

void
Visitor::visitClass(ClassNode *node)
{
	for (auto &method : node->m_instanceMethods)
		this->visitMethod(method);
	for (auto &method : node->m_classMethods)
		this->visitMethod(method);
}

void
Visitor::visitMethod(MethodNode *node)
{
	for (auto &parameter : node->m_parameters)
		visitParameterDecl(parameter);
	for (auto &local : node->m_locals)
		visitLocalDecl(local);
	for (auto & stmt: node->m_statements)
		stmt->accept(*this);
}

void
Visitor::visitReturnStmt(ReturnStmtNode *node)
{
	node->expr->accept(*this);
}

void
Visitor::visitBlockLocalReturn(ExprNode *node)
{
	node->accept(*this);
}

void
Visitor::visitExprStmt(ExprStmtNode *node)
{
	node->expr->accept(*this);
}

void
Visitor::visitBlockExpr(BlockExprNode *node)
{
	for (auto &parameter : node->m_args)
		visitParameterDecl(parameter);
	for (auto &local : node->m_locals)
		visitLocalDecl(local);
	for (auto &stmt : node->m_stmts) {
		AST::ExprStmtNode *expr;

		/*
		 * if it's the last statement, turn it into a local return
		 */
		if (stmt == node->m_stmts.back() &&
		    (expr = dynamic_cast<AST::ExprStmtNode *>(stmt)) != NULL) {
			/* last statement becomes the expression
			 * returned */
			this->visitBlockLocalReturn(expr->expr);
		} else
			stmt->accept(*this);
	}
}

void
Visitor::visitInlinedBlockExpr(BlockExprNode *node)
{
	this->visitBlockExpr(node);
}


void Visitor::visitMessageExpr(MessageExprNode *node)
{
	node->receiver->accept(*this);
        for (auto & arg: node->args) {
                arg->accept(*this);
        }
}

void
Visitor::visitAssignExpr(AssignExprNode *node)
{
	node->left->accept(*this);
	node->right->accept(*this);
}

void
BlockExprNode::print(int in)
{
	std::cout << "<block>\n";

	std::cout << blanks(in + 1) << "<params>\n";
	for (auto &e : m_args)
		std::cout << blanks(in + 2) << "<param:" << e.name << " />\n";
	std::cout << blanks(in + 1) << "</params>\n";

	std::cout << blanks(in + 1) << "<statements>\n";
	for (auto e : m_stmts) {
		std::cout << blanks(in + 1) << "<statement>\n";
		e->print(in + 2);
		std::cout << blanks(in + 1) << "</statement>\n";
	}
	std::cout << blanks(in + 1) << "</statements>\n";

	std::cout << "</block>\n";
}

bool
BlockExprNode::isSelf()
{
	return m_stmts.size() == 1 && 0; // stmts.front().isSelf();
}

void
ExprStmtNode::print(int in)
{
	std::cout << blanks(in) << "<exprstmt>\n";
	expr->print(in + 1);
	std::cout << blanks(in) << "</exprstmt>\n";
}

void
ReturnStmtNode::print(int in)
{
	std::cout << blanks(in) << "<returnstmt> ";
	expr->print(in + 1);
	std::cout << blanks(in) << "</returnstmt>";
}

void
ClassNode::addMethods(std::vector<MethodNode *> meths)
{
	for (auto m : meths)
		if (m->m_isClassMethod)
			m_classMethods.push_back(m);
		else
			m_instanceMethods.push_back(m);
}

}
