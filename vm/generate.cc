#include <cassert>
#include <iostream>
#include <sstream>

#include "analyse.hh"
#include "ast.hh"
#include "generate.hh"

void
generateScopeName(Scope *scope, std::string &name)
{
	if (scope->kind == Scope::kMethod)
		name.insert(0, "M");
	else if (scope->kind == Scope::kBlock)
		name.insert(0, "B");
	if (scope->lexicalOuter != NULL)
		return generateScopeName(scope->lexicalOuter, name);
}

bool
useCrossesBlock(Scope *useScope, Scope *defScope)
{
	while (useScope != defScope) {
		/* does this work for optimised blocks??? i think it does? */
		if (useScope->kind == Scope::kBlock)
			return true;
		useScope = useScope->lexicalOuter;
		assert(useScope != NULL);
	}
	return false;
}

std::string
CodeGeneratorVisitor::blockName(AST::BlockExprNode *node)
{
	return "block" + std::to_string(uintptr_t(node));
}

std::string &
CodeGeneratorVisitor::nameForScope(Scope *scope)
{
	if (scopeNames.find(scope) == scopeNames.end()) {
		std::string name;
		generateScopeName(scope, name);
		scopeNames[scope] = std::move(name);
	}
	return scopeNames[scope];
}

std::string
CodeGeneratorVisitor::heapvarsNameForScope(Scope *scope)
{
	return nameForScope(scope) + "_heapvars";
}

void
CodeGeneratorVisitor::genContextType(std::string structName, CodeScope *scope)
{
	types << "struct " << structName << "_context {\n";
	std::string heapVarsName = (scope->heapvars.empty() ?
		"__unused__" :
		heapvarsNameForScope(scope));
	types << "/* my heapvar vector */\n"
	      << "  struct " << heapVarsName << " *" << heapVarsName << ";\n";

	if (scope->kind == Scope::kBlock) {
		/* skip this, we're going to access the blockClosure's stuff
		 * instead */
#if 0
		types << "/* imported heapvar vectors */\n";
		for (auto &scope : scope->usingHeapvarsFrom) {
			types << "  struct " << heapvarsNameForScope(scope)
			      << " *" << heapvarsNameForScope(scope) << ";\n";
		}
		types << "/* copied vars */\n";
		for (auto &var : scope->copyingVars)
			types << "  oop " << nameForScope(var->scope)
			      << var->name << ";\n";
#endif
		types << "  struct " << structName << " *"
		      << "thisBlock;\n";
	}

	types << "/* non-heapvar'd arguments */\n";
	for (auto &var : scope->arguments)
		if (var.remoteAccess != Variable::kWrittenRemotely)
			types << "  oop " << nameForScope(scope) << var.name
			      << ";\n";
	types << "/* non-heapvar'd locals */\n";
	for (auto &var : scope->locals)
		if (var.remoteAccess != Variable::kWrittenRemotely)
			types << "  oop " << nameForScope(scope) << var.name
			      << ";\n";
	types << "};\n\n";
}

void
CodeGeneratorVisitor::genHeapvarsType(CodeScope *scope)
{
	if (!scope->heapvars.empty()) {
		types << "struct " << heapvarsNameForScope(scope) << " {\n";
		for (auto &heapvar : scope->heapvars)
			types << "  oop " << heapvar.name << ";\n";
		types << "};\n\n";
	}
}

void
CodeGeneratorVisitor::genMoveArgumentsToHeapvars(CodeScope *scope,
    std::stringstream &stream)
{
	bool didMoveAny = false;
	for (auto &arg : scope->arguments)
		if (arg.remoteAccess == Variable::kWrittenRemotely) {
			stream << "  " << heapvarsNameForScope(scope) << "->";
			emitVariableAccess(scope, &arg, stream, true);
			stream << " = ";
			emitVariableAccess(scope, &arg, stream, true);
			stream << ";\n";
			didMoveAny = true;
		}
	if (didMoveAny)
		stream << "\n";
}

void
CodeGeneratorVisitor::genContextCreation(CodeScope *scope,
    std::string structName, std::stringstream &stream)
{
	fun() << "  volatile struct " << structName << " *thisContext";
	if (!scope->needsHeapContext) {
		fun() << ";\n  struct " << structName << " __context;\n";
		fun() << "  thisContext = &__context;\n";
	} else {
		fun() << " = allocOopsObj(sizeof(struct " << structName
		      << "));\n";
	}

	if (!scope->heapvars.empty()) {
		fun() << "  struct " << heapvarsNameForScope(scope) << " *"
		      << heapvarsNameForScope(scope);
		fun() << " = allocOopsObj(sizeof(struct "
		      << heapvarsNameForScope(scope) << "));\n";
	}
	fun() << "\n";
}

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

	genHeapvarsType(node->scope);
	genContextType("M", node->scope);

	scope.push(node->scope);
	funStack.push({});

	/* method function signature */
	fun() << "void method()\n{\n";

	/* method body */
#if 0
	fun() << "  oop __retVal = self;\n";
#endif
	genContextCreation(node->scope, "M_context", fun());
	genMoveArgumentsToHeapvars(node->scope, fun());

	if (node->scope->needsHeapContext) {
		fun()
		    << "  if (oops_setjmp(thisContext->returnBuf) != 0) {"
		       "\n    if(thisContext->flags & kContextShouldReturn)"
		       "\n      return oops_return(thisContext, thisContext->retval);"
		       "\n  }\n\n";
	}
#if 0
	} else
		fun() << "  if (0) {\n";

	fun() << "__doReturn:"
		 "\n    // set thread context to sender..."
		 "\n    oops_markContextReturned(thisContext);"
		 "\n    return __retVal;"
		 "\n  }\n\n";
#else

#endif
	fun() << "/* code */\n";
	AST::Visitor::visitMethod(node);

	/* end of method */
	fun() << "}\n";
	funcs.push_back(fun().str());
	funStack.pop();
	scope.pop();

	std::cout
	    << "/*\n * generated by the Valutron Optimizing Compiler\n */\n";
	std::cout << types.str() << "\n";
	for (auto &fun : funcs)
		std::cout << fun << "\n";
}

void
CodeGeneratorVisitor::visitReturnStmt(AST::ReturnStmtNode *node)
{
	std::cout << "Visiting return stmt\n";
	if (node->isNonLocalReturn)
		fun() << "return oops_nonLocalReturn(thisContext, ";
	else
		fun() << "return oops_return(thisContext, ";
	AST::Visitor::visitReturnStmt(node);
	fun() << ");\n";
}

void
CodeGeneratorVisitor::visitBlockLocalReturn(AST::ExprNode *node)
{
	std::cout << "Visiting block local return stmt\n";

	if (!scope.top()->inOptimisedBlock()) {
#if 0
		fun() << "retVal = ";
		AST::Visitor::visitBlockLocalReturn(node);
		fun() << ";\n";
		fun() << "goto __return;\n";
#endif
		/* this is return from a true block */
		fun() << "return oops_return(thisContext, ";
		AST::Visitor::visitBlockLocalReturn(node);
		fun() << ");\n";
	} else {
		/*
		 * emit it as the last expression-statement of the
		 * statement expression
		 */
		AST::Visitor::visitBlockLocalReturn(node);
		fun() << ";\n";
	}
}

void
CodeGeneratorVisitor::visitExprStmt(AST::ExprStmtNode *node)
{
	AST::Visitor::visitExprStmt(node);
	fun() << ";\n";
}

void
CodeGeneratorVisitor::visitBlockExpr(AST::BlockExprNode *node)
{
	std::cout << "Visiting block expression:\n";
	genHeapvarsType(node->scope);

	/* block type */
	types << "struct block" << (uintptr_t)node << "{\n";
	types << "/* imported heapvar vectors */\n";
	// TODO: factoring 1?
	for (auto &scope : node->scope->usingHeapvarsFrom) {
		types << "  struct " << heapvarsNameForScope(scope) << " *"
		      << heapvarsNameForScope(scope) << ";\n";
	}
	types << "/* copied vars */\n";
	for (auto &var : node->scope->copyingVars)
		types << "  oop " << nameForScope(var->scope) << var->name
		      << ";\n";
	types << "};\n\n";

	/* block context type */
	genContextType(blockName(node), node->scope);

	/* the function to make the block */
	types << "makeBlock" << (intptr_t)node << "(oop self";
	/* heapvar vector arguments */
	for (auto &scope : node->scope->usingHeapvarsFrom)
		types << ", struct " << heapvarsNameForScope(scope) << " *"
		      << heapvarsNameForScope(scope);
	/* copied variable parameters */
	for (auto &var : node->scope->copyingVars)
		types << ", oop " << nameForScope(var->scope) << var->name;
	types << ")\n{\n";
	/* body */
	types << "  newBlock = allocOopsObj(sizeof(struct block"
	      << (uintptr_t)node << ") / sizeof(Oop));\n";
	/* heapvar vectors assignment */
	for (auto &scope : node->scope->usingHeapvarsFrom)
		types << "  newBlock->" << heapvarsNameForScope(scope) << " = "
		      << heapvarsNameForScope(scope) << ";\n";
	/* copied variables assignment */
	for (auto &var : node->scope->copyingVars) {
		std::string name = nameForScope(var->scope) + var->name;
		types << "  newBlock->" << name << " = " << name << ";\n";
	}
	types << "  return newBlock;\n";
	types << "}\n\n";

	/* block code */
	funStack.push({});
	scope.push(node->scope);
	fun() << "static void block" << (uintptr_t)node << "()\n{\n";

	genContextCreation(node->scope,
	    "block" + std::to_string((uintptr_t)node), fun());
	genMoveArgumentsToHeapvars(node->scope, fun());

	fun() << "/* code */\n";

	AST::Visitor::visitBlockExpr(node);
	fun() << "}\n";
	funcs.push_back(fun().str());
	scope.pop();
	funStack.pop();

	/* invocation of the makeBlock function */
	fun() << "makeBlock" << (uintptr_t)node << "(self";
	for (auto &aScope : node->scope->usingHeapvarsFrom) {
		fun() << ", ";
		if (useCrossesBlock(scope.top(), aScope))
			fun() << "thisBlock->";
		fun() << heapvarsNameForScope(aScope);
	}
	/* copied variables*/
	for (auto &var : node->scope->copyingVars) {
		fun() << ", ";
		emitVariableAccess(scope.top(), var, fun());
	}
	fun() << ")";
}

void
CodeGeneratorVisitor::visitInlinedBlockExpr(AST::BlockExprNode *node)
{
	std::cout << "Visiting inlined block expression:\n";
	/* block code */
	scope.push(node->scope);
	fun() << "({\n";
	AST::Visitor::visitBlockExpr(node);
	fun() << "})";
	scope.pop();
}

void
CodeGeneratorVisitor::visitMessageExpr(AST::MessageExprNode *node)
{
	std::cout << "Visiting message expression #" << node->selector << "\n";

	if (node->specialKind == AST::MessageExprNode::kIfTrueIfFalse) {
		fun() << "(";
		node->receiver->accept(*this);
		fun() << " ?\n    ";
		node->args[0]->accept(*this);
		fun() << " :\n    ";
		node->args[1]->accept(*this);
		fun() << ")";
		return;
	} else if (node->specialKind == AST::MessageExprNode::kToDo) {
		AST::BlockExprNode *block = dynamic_cast<AST::BlockExprNode *>(
		    node->args[1]);

		assert(block != NULL);

		fun() << "({"
			 "\n\toop __result = nil;"
			 "\n\toop __counter = ";
		node->receiver->accept(*this);
		fun() << ";"
			 "\n\toop __comparator = ";
		node->args[0]->accept(*this);
		fun() << ";"
			 "\n\tfor(; smiIsLessThan(__counter, __comparator); "
			 "__counter = smiAdd(__counter, 1)) {\n\t";

		/* assign the counter's value to the block's first
		 * argument */
		emitVariableAccess(scope.top(), &block->scope->arguments[0],
		    fun());
		fun() << " = __counter;\n\t";

		/* emit block body, assigned to __result */
		fun() << "__result = ";
		block->accept(*this);
		fun() << ";\n\t}"
			 "\n\t__result;"
			 "\n})\n";
		return;
	}

plain:
	fun() << "msgLookup(";
	node->receiver->accept(*this);
	fun() << ", \"" << node->selector << "\")";
	fun() << "(";
	bool first = true;
	for (auto &arg : node->args) {
		if (first)
			first = false;
		else
			fun() << ", ";
		arg->accept(*this);
	}
	fun() << ")";
}

void
CodeGeneratorVisitor::visitAssignExpr(AST::AssignExprNode *node)
{
	node->left->accept(*this);
	fun() << " = ";
	node->right->accept(*this);
}

void
CodeGeneratorVisitor::emitVariableAccess(Scope *scope, Variable *var,
    std::stringstream &stream, bool elideThisContext)
{
	switch (var->kind) {
	case Variable::kArgument:
	case Variable::kLocal:
	case Variable::kHeapvar: {
		if (useCrossesBlock(scope, var->scope))
			stream << "thisBlock->";
		else if (!elideThisContext && var->kind != Variable::kHeapvar)
			/* no need access thru thisContext for own
			 * heapvars */
			stream << "thisContext->";

		if (var->kind == Variable::kHeapvar)
			stream << heapvarsNameForScope(var->scope) << "->";

		stream << nameForScope(var->scope) << var->name;
		return;
	}
	case Variable::kInlinedBlockLocal:
		return emitVariableAccess(scope, var->real, stream);
	}
}

void
CodeGeneratorVisitor::visitIdentExpr(AST::IdentExprNode *node)
{
	std::cout << "Visiting identifier expression <" << node->id << ">\n";
	emitVariableAccess(scope.top(), node->variable, fun());
}

void
CodeGeneratorVisitor::visitIntExpr(AST::IntExprNode *node)
{
	std::cout << "Visiting integer literal " << node->num << "\n";
	fun() << "makeSMI(" << node->num << ")";
}
