#include <algorithm>
#include <cassert>
#include <iostream>

#include "analyse.hh"
#include "ast.hh"

Variable *
Scope::lookup(std::string name, bool forWrite, bool remoteAccess)
{
	return NULL;
}

void
Variable::markRemoteAccess(bool isRemotelyAccessed, bool isWritten)
{
	if (isRemotelyAccessed && isWritten && remoteAccess < kWrittenRemotely)
		remoteAccess = kWrittenRemotely;
	else if (isRemotelyAccessed && remoteAccess < Variable::kReadRemotely)
		remoteAccess = Variable::kReadRemotely;
}

Variable *
CodeScope::lookup(std::string name, bool forWrite, bool remoteAccess)
{
	/* heapvars is always searched first */
	for (auto &heapvar : heapvars)
		if (heapvar.name == name) {
			return &heapvar;
		}
	for (auto &arg : arguments)
		if (arg.name == name) {
			arg.markRemoteAccess(remoteAccess, forWrite);
			return &arg;
		}
	for (auto &local : locals)
		if (local.name == name) {
			local.markRemoteAccess(remoteAccess, forWrite);
			return &local;
		}
	if (outerScope == NULL)
		return NULL;
	else
		return outerScope->lookup(name, forWrite,
		    remoteAccess ? remoteAccess : kind == kBlock);
}

void
CodeScope::addArg(std::string name)
{
	arguments.push_back({ name, this, Variable::kArgument });
}

void
CodeScope::addLocal(std::string name)
{
	locals.push_back({ name, this, Variable::kLocal });
}

void
CodeScope::moveRemotelyAccessedToHeapvars()
{
	for (auto &local : locals)
		if (local.remoteAccess != Variable::kNone) {
			heapvars.push_back(
			    { local.name, this, Variable::kHeapvar });
		}
}

void
AnalysisVisitor::visitMethod(AST::MethodNode *node)
{
	node->scope = new CodeScope(NULL, Scope::kMethod);
	scopeStack.push(node->scope);
	AST::Visitor::visitMethod(node);
	for (auto &local : node->scope->locals)
		std::cout << "Local <" << local.name
			  << ">: " << local.remoteAccess << "\n";
	scopeStack.pop();
}

void
AnalysisVisitor::visitParameterDecl(AST::VarDecl &node)
{
	scopeStack.top()->addArg(node.name);
}

void
AnalysisVisitor::visitLocalDecl(AST::VarDecl &node)
{
	std::cout << "add local <" << node.name << ">!\n";

	scopeStack.top()->addLocal(node.name);
}

void
AnalysisVisitor::visitBlockExpr(AST::BlockExprNode *node)
{
	node->scope = new CodeScope(scopeStack.top(), Scope::kBlock);
	scopeStack.push(node->scope);
	AST::Visitor::visitBlockExpr(node);
	scopeStack.pop();
}

void
AnalysisVisitor::visitAssignExpr(AST::AssignExprNode *node)
{
	auto var = scopeStack.top()->lookup(node->left->id, true);
	if (!var) {
		std::cerr << "Reference to undeclared name " << node->left->id
			  << "\n";
		throw 0;
	} else if (var->kind == Variable::kArgument) {
		std::cerr << "Arguments cannot be assigned to: "
			  << node->left->id << "\n";
		throw 0;
	}
	node->left->variable = var;
	AST::Visitor::visitAssignExpr(node);
}

void
AnalysisVisitor::visitIdentExpr(AST::IdentExprNode *node)
{
	auto var = scopeStack.top()->lookup(node->id);
	if (!var) {
		std::cerr << "Reference to undeclared name " << node->id
			  << "\n";
		throw 0;
	}
	node->variable = var;
	AST::Visitor::visitIdentExpr(node);
}

void
ClosureAnalysisVisitor::visitMethod(AST::MethodNode *node)
{
	scope = node->scope;
	node->scope->moveRemotelyAccessedToHeapvars();
	AST::Visitor::visitMethod(node);
	scope = node->scope->outerScope;
}

void
ClosureAnalysisVisitor::visitParameterDecl(AST::VarDecl &node)
{
}

void
ClosureAnalysisVisitor::visitLocalDecl(AST::VarDecl &node)
{
}

void
ClosureAnalysisVisitor::visitBlockExpr(AST::BlockExprNode *node)
{
	scope = node->scope;
	AST::Visitor::visitBlockExpr(node);
	scope = node->scope->outerScope;
}

void
ClosureAnalysisVisitor::visitIdentExpr(AST::IdentExprNode *node)
{
	if (node->variable->remoteAccess == Variable::kWrittenRemotely) {
		node->variable = node->variable->scope->lookup(node->id);
		assert(node->variable->kind == Variable::kHeapvar);

		/*
		 * now make sure the defining scope's heapvars are carried all
		 * the way up to this scope. factoring candidate??
		 */
		for (auto aScope = scope; aScope != node->variable->scope;
		     aScope = aScope->outerScope) {
			// TODO: refactor!
			CodeScope *codeScope = dynamic_cast<CodeScope *>(
			    aScope);

			/*
			 * because the furthest we should be going is up to a
			 * block scope (methods don't 'use' their own heapvars)
			 */
			assert(codeScope != NULL);

			/*
                         * if an intervening scope doesn't use the heapvars, add
			 * them
                         */
			if (std::find(codeScope->usingHeapvarsFrom.begin(),
				codeScope->usingHeapvarsFrom.end(),
				node->variable->scope) ==
			    codeScope->usingHeapvarsFrom.end()) {
				codeScope->usingHeapvarsFrom.push_back(
				    node->variable->scope);
                            }

                            // the usingheapvarsfrom tells us what to pass in
                            // arguments to a block when creating it

                            // we can just give each method/block's own heapvars
                            // its own name,

                            // then we can just load them up
		}
	}
}
