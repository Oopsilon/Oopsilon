#ifndef GENERATE_H_
#define GENERATE_H_

#include <map>
#include <sstream>
#include <filesystem>
#include <stack>

#include "analyse.hh"
#include "ast.hh"

/*!
 * Generates code for a single class (in future: or namespace).
 */
class CodeGeneratorVisitor : public AST::Visitor {
	std::stack<std::stringstream> funStack;
	std::vector<std::string> funcs;
	std::stringstream types;

	std::stringstream translationUnitOut;

	std::stack<Scope *> scope;

	/*!
	 * @name per-translation unit
	 * @{
	 */
	/*!
	 * Symbol name vector.
	 *
	 * For ahead-of-time C code generation, every translation unit (we emit
	 * one per class) gets a static array of:
	 *   static struct symbolReference {
	 *	const char *name;
	 *	SymbolOop ref; 
	 *    } __symbolReferences[n];
	 * and references to symbols in code are of the form:
	 *	__symbolReferences[i]
	 * where n is the size of the symbolNames vector and i is the index in
	 * that array at which a particular string is found corresponding to the
	 * symbol.
	 *
	 * The initialisation function for the translation unit resolves each
	 * string to a symbol reference.
	 */
	std::vector<std::string> symbolNames;

	/*!
	 * Generate a reference to the Symbol for a given string.
	 */
	std::string genSymbolReference(std::string string);
	/*!
	 * @} 
	 */

	/* we map code scope pointers to unique names */
	std::map<Scope *, std::string> scopeNames;

	std::string blockName(AST::BlockExprNode *node);
	/*
	 * A name for a scope. 
	 * TODO: Refactor this away? We can just call a method's scope "method"
	 * and a block's scope by its number.
	 */
	std::string &nameForScope(Scope *scope);
	std::string heapvarsNameForScope(Scope *scope);
	/*
	 * Generate the my-heapvars struct declaration for a scope.
	 */
	void genHeapvarsType(CodeScope *scope);
	/*
	 * Generate the context struct declaration for a scope.
	 */
	void genContextType(std::string structName, CodeScope *scope);
	/*
	 * Generate context creation and setup, including scope's my-heapvars
	 * allocation.
	 */
	void genContextCreation(CodeScope *scope, std::string structName,
	    std::stringstream &stream);
	/*
	 * Generate code to move remotely-written arguments to heapvars.
	 */
	void genMoveArgumentsToHeapvars(CodeScope *scope,
	    std::stringstream &stream);
	/*
	 * Generate code to access some variable.
	 */
	void emitVariableAccess(Scope *scope, Variable *var,
	    std::stringstream &stream, bool elideThisContext = false);

	std::stringstream &fun() { return funStack.top(); };

	//void visitClass(AST::ClassNode *node);
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

	public:
	CodeGeneratorVisitor(std::filesystem::path outputDirectory, AST::ClassNode *klass);
};

#endif /* GENERATE_H_ */
