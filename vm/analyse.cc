#include <algorithm>
#include <cassert>
#include <iostream>
#include <list>
#include <stdexcept>

#include "analyse.hh"
#include "ast.hh"

using compile_error = std::runtime_error;

NamespaceScope rootScope("", NULL);
NamespaceScope smalltalkScope("Smalltalk", &rootScope);

/*
 * registration
 */
void
RegistrarVisitor::visitClass(AST::ClassNode *node)
{
	smalltalkScope.addClass(node);
}

void
NamespaceScope::addClass(AST::ClassNode *klass)
{
	members.emplace_back(NamespaceMemberVariable(klass->m_name, klass));
}

NamespaceMemberVariable *
NamespaceScope::lookup(std::string name, bool forWrite, bool remoteAccess)
{
	for (auto &member : members) {
		if (member.name == name)
			return &member;
	}

	return NULL;
}

/*
 * linkup
 */
void
LinkupVisitor::visitClass(AST::ClassNode *node)
{
	if (node->m_superName == "nil")
		return;

	auto super = smalltalkScope.lookup(node->m_superName);
	if (!super) {
		throw std::runtime_error(
		    "Couldn't find class named " + node->m_superName);
	}

	if (!super->isClass()) {
		throw compile_error("Class " + node->m_name + ": Superclass " +
		    node->m_superName + " is not a class");
	}

	node->m_superClass = super->klass();
}

/*
 * analysis proper
 */

Variable *
Scope::lookup(std::string name, bool forWrite, bool remoteAccess)
{
	return NULL;
}

void
InstanceScope::addIvar(std::string name, int index)
{
	instanceVars.emplace_back(InstanceVariable(name, index, this));
}

Variable *
InstanceScope::lookup(std::string name, bool forWrite, bool remoteAccess)
{
	if (name == "self") {
		return &selfVar;
	}

	for (auto &ivar : instanceVars) {
		if (ivar.name == name)
			return &ivar;
	}

	if (lexicalOuter)
		return lexicalOuter->lookup(name, forWrite, remoteAccess);
	else
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
			if (local.kind == Variable::kInlinedBlockLocal)
				local = *local.real;
			local.markRemoteAccess(remoteAccess, forWrite);
			return &local;
		}
	if (lexicalOuter == NULL) {
		assert(kind == kMethod);
		if (name == "self") {
			return NULL;
		}
		return NULL;
	} else
		return lexicalOuter->lookup(name, forWrite,
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
CodeScope::addInlinedBlockLocal(std::string name, bool isArg)
{
	assert(kind == kOptimisedBlock);
	std::string scopeVariableName = "blockLocal" +
	    std::to_string((uintptr_t)this) + "_" + name;
	realScope()->addLocal(scopeVariableName);
	(isArg ? arguments : locals)
	    .push_back({ name, this, Variable::kInlinedBlockLocal,
		Variable::kNone, realScope()->lookup(scopeVariableName) });
}

void
CodeScope::moveRemotelyAccessedToHeapvars()
{
	for (auto &arg : arguments)
		if (arg.remoteAccess == Variable::kWrittenRemotely) {
			heapvars.push_back(
			    { arg.name, this, Variable::kHeapvar });
		}
	for (auto &local : locals)
		if (local.remoteAccess == Variable::kWrittenRemotely) {
			heapvars.push_back(
			    { local.name, this, Variable::kHeapvar });
		}
}

CodeScope *
Scope::realScope()
{
	CodeScope *cs = dynamic_cast<CodeScope *>(this);
	assert(this != NULL);
	if (kind != kOptimisedBlock)
		return cs;
	else
		return lexicalOuter->realScope();
}

void
AnalysisVisitor::visitClass(AST::ClassNode *node)
{
	int i = 0;
	std::list<AST::VarDecl *> allIvarDecls, allCvarDecls;

	std::cout << "Analyse class " << node->m_name << "\n";

	methodClass = node;
	node->m_classScope = new InstanceScope(node);
	node->m_instanceScope = new InstanceScope(node);

	for (auto klass = node; klass != NULL; klass = klass->m_superClass) {
		for (auto ivarIter = klass->m_instanceVars.rbegin();
		     ivarIter != klass->m_instanceVars.rend(); ivarIter++) {
			allIvarDecls.emplace_front(&*ivarIter);
		}
				for (auto cvarIter = klass->m_classInstanceVars.rbegin();
		     cvarIter != klass->m_classInstanceVars.rend(); cvarIter++) {
			allCvarDecls.emplace_front(&*cvarIter);
		}
	}

	for (auto &ivar : allIvarDecls)
		node->m_instanceScope->addIvar(ivar->name, i++);
	for (auto &cvar : allCvarDecls)
		node->m_classScope->addIvar(cvar->name, i++);


	AST::Visitor::visitClass(node);
}

void
AnalysisVisitor::visitMethod(AST::MethodNode *node)
{
	method = node;
	node->m_class = methodClass;
	node->scope = new CodeScope(node->m_isClassMethod ?
		methodClass->m_classScope :
		methodClass->m_instanceScope,
	    Scope::kMethod);
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
	if (scopeStack.top()->inOptimisedBlock())
		scopeStack.top()->addInlinedBlockLocal(node.name, true);
	else
		scopeStack.top()->addArg(node.name);
}

void
AnalysisVisitor::visitLocalDecl(AST::VarDecl &node)
{
	std::cout << "add local <" << node.name << ">!\n";

	if (scopeStack.top()->inOptimisedBlock())
		scopeStack.top()->addInlinedBlockLocal(node.name);
	else
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
AnalysisVisitor::visitInlinedBlockExpr(AST::BlockExprNode *node)
{
	node->scope = new CodeScope(scopeStack.top(), Scope::kOptimisedBlock);
	scopeStack.push(node->scope);
	AST::Visitor::visitBlockExpr(node);
	scopeStack.pop();
}

void
AnalysisVisitor::visitMessageExpr(AST::MessageExprNode *node)
{
	if (node->selector == "ifTrue:ifFalse:" && node->args[0]->isBlock() &&
	    node->args[1]->isBlock()) {
		node->specialKind = AST::MessageExprNode::kIfTrueIfFalse;
		dynamic_cast<AST::BlockExprNode *>(node->args[0])->isInlined =
		    true;
		dynamic_cast<AST::BlockExprNode *>(node->args[1])->isInlined =
		    true;
	} else if (node->selector == "to:do:" && node->args[1]->isBlock()) {
		node->specialKind = AST::MessageExprNode::kToDo;
		dynamic_cast<AST::BlockExprNode *>(node->args[1])->isInlined =
		    true;
	}
	AST::Visitor::visitMessageExpr(node);
}

void
AnalysisVisitor::visitAssignExpr(AST::AssignExprNode *node)
{
	auto var = scopeStack.top()->lookup(node->left->id, true);
	if (!var) {
		std::cerr << "Reference to undeclared name " << node->left->id
			  << "\n";
		throw 0;
	}
#if 0
	else if (var->kind == Variable::kArgument) {
		std::cerr << "Arguments cannot be assigned to: "
			  << node->left->id << "\n";
		throw 0;
	}
#endif
	node->left->variable = var;
	AST::Visitor::visitAssignExpr(node);
}

void
AnalysisVisitor::visitIdentExpr(AST::IdentExprNode *node)
{
	auto var = scopeStack.top()->lookup(node->id);
	if (!var) {
		// if ()
	}
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
	scope = node->scope->lexicalOuter;
}

void
ClosureAnalysisVisitor::visitReturnStmt(AST::ReturnStmtNode *node)
{
	bool isNLR = false;

	std::cout << "ClosureAnalysisVisitor visiting a return statement.\n";

	if (scope->kind != Scope::kMethod) {
		std::cout << "Checking for NLR.\n";
		auto aScope = scope;
		do {
			std::cout << "aScope kind is: " << aScope->kind << "\n";
			if (aScope->kind == Scope::kBlock)
				isNLR = true;
			else if (aScope->kind == Scope::kMethod && isNLR)
				aScope->needsHeapContext = true;
		} while ((aScope = aScope->lexicalOuter) != NULL);
	}

	node->isNonLocalReturn = isNLR;
	if (isNLR)
		printf("Nonlocalreturn found!\n");
	AST::Visitor::visitReturnStmt(node);
}

void
ClosureAnalysisVisitor::visitBlockExpr(AST::BlockExprNode *node)
{
	std::cout << "Entering block expression.\n";
	scope = node->scope;
	std::cout << "Scope kind: " << scope->kind << "\n";
	node->scope->moveRemotelyAccessedToHeapvars();
	AST::Visitor::visitBlockExpr(node);
	scope = node->scope->lexicalOuter;
	std::cout << "Exiting block expression.\n";
}

void
ClosureAnalysisVisitor::visitInlinedBlockExpr(AST::BlockExprNode *node)
{
	scope = node->scope;
	AST::Visitor::visitBlockExpr(node);
	scope = node->scope->lexicalOuter;
}

void
ClosureAnalysisVisitor::visitIdentExpr(AST::IdentExprNode *node)
{
	std::cout << "ClosureAnalysisisVisitor visiting an ident expression\n";
	if (node->variable->remoteAccess == Variable::kWrittenRemotely) {
		node->variable = node->variable->scope->lookup(node->id);
		assert(node->variable->kind == Variable::kHeapvar);

		/*
		 * now make sure the defining scope's heapvars are carried all
		 * the way up to this scope. factoring candidate??
		 */
		for (auto aScope = scope; aScope != node->variable->scope;
		     aScope = aScope->lexicalOuter) {
			// TODO: refactor!
			CodeScope *codeScope = dynamic_cast<CodeScope *>(
			    aScope);

#if 0
			/* nothing to do for these */
			if (scope->kind == Scope::kOptimisedBlock)
				continue;
#endif

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
		}
	} else if (node->variable->remoteAccess == Variable::kReadRemotely) {
		/*
		 * make sure the defining scope's copieds are carried all
		 * the way up to this scope. factoring candidate??
		 */
		for (auto aScope = scope; aScope != node->variable->scope;
		     aScope = aScope->lexicalOuter) {
			// TODO: refactor!
			CodeScope *codeScope = dynamic_cast<CodeScope *>(
			    aScope);

#if 0
			if (scope->kind == Scope::kOptimisedBlock)
				continue;
#endif

			/*
			 * because the furthest we should be going is up to a
			 * block scope (methods don't 'use' their own heapvars)
			 */
			assert(codeScope != NULL);

			/*
			 * if an intervening scope doesn't use the heapvars, add
			 * them
			 */
			if (std::find(codeScope->copyingVars.begin(),
				codeScope->copyingVars.end(), node->variable) ==
			    codeScope->copyingVars.end()) {
				codeScope->copyingVars.push_back(
				    node->variable);
			}
		}
	}
}
