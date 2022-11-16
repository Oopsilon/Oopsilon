#ifndef AST_H_
#define AST_H_

#include <cstddef>
#include <string>
#include <typeinfo>
#include <vector>

class CodeScope;
class Variable;

static inline std::string
blanks(size_t n)
{
	return std::string(n, ' ');
}

namespace AST {

/* Details of the position of some source code. */
class Position {
	size_t m_oldLine, m_oldCol, m_oldPos;
	size_t m_line, m_col, m_pos;

    public:
	Position() {};
	Position(size_t oldLine, size_t oldCol, size_t oldPos, size_t line,
	    size_t col, size_t pos)
	    : m_oldLine(oldLine)
	    , m_oldCol(oldCol)
	    , m_oldPos(oldPos)
	    , m_line(line)
	    , m_col(col)
	    , m_pos(pos) {};
	Position(const Position &a, const Position &b)
	    : m_oldLine(a.m_oldLine)
	    , m_oldCol(a.m_oldCol)
	    , m_oldPos(a.m_oldPos)
	    , m_line(b.m_line)
	    , m_col(b.m_col)
	    , m_pos(b.m_pos) {};

	/* Get line number */
	size_t line() const { return m_oldLine; }
	/* Get column number*/
	size_t col() const { return m_oldCol; }
	/* Get absolute position in source-file */
	size_t pos() const;
};

struct Token {
	Position m_pos;

	Token() = default;
	Token(const Token &) = default;
	Token(Token &&) = default;

	Token(Position pos)
	    : m_pos(pos) {};
	Token(Position pos, double f)
	    : m_pos(pos)
	    , floatValue(f) {};
	Token(Position pos, int i)
	    : m_pos(pos)
	    , intValue(i) {};
	Token(Position pos, const std::string &s)
	    : m_pos(pos)
	    , stringValue(s) {};
	Token(Position pos, std::string &&s)
	    : m_pos(pos)
	    , stringValue(std::move(s)) {};

	Token &operator=(const Token &) = default;
	Token &operator=(Token &&) = default;

	operator std::string() const { return stringValue; }
	operator const char *() const { return stringValue.c_str(); }
	operator double() const { return floatValue; }
	const Position &pos() const { return m_pos; }

	double floatValue = 0.0;
	int intValue = 0;
	std::string stringValue;
};

class ClassNode;
class MethodNode;
class ReturnStmtNode;
class ExprStmtNode;
class BlockExprNode;
class CascadeExprNode;
class MessageExprNode;
class AssignExprNode;
class IdentExprNode;
class IntExprNode;
class ExprNode;
class VarDecl;

class Visitor {
    public:
	virtual void visitClass(ClassNode *node);
	virtual void visitMethod(MethodNode *node);
	virtual void visitParameterDecl(VarDecl &node) {}
	virtual void visitLocalDecl(VarDecl &node) {}
	virtual void visitReturnStmt(ReturnStmtNode *node);
	virtual void visitBlockLocalReturn(ExprNode *node);
	virtual void visitExprStmt(ExprStmtNode *node);
	virtual void visitBlockExpr(BlockExprNode *node);
	virtual void visitCascadeExpr(CascadeExprNode *node) {}
	virtual void visitMessageExpr(MessageExprNode *node);
	virtual void visitAssignExpr(AssignExprNode *node) {}
	virtual void visitIdentExpr(IdentExprNode *node) {}
	virtual void visitIntExpr(IntExprNode *node) {}
};

struct Node {
	Position m_pos;

	Node() = default;
	Node(Position pos)
	    : m_pos(pos) {};

	virtual void accept(Visitor &visitor) { }
	virtual void print(int in)
	{
		printf("<node: %s/>\n", typeid(*this).name());
	}
};

/*
 * Type nodes
 */
struct Type {
	enum Kind {
		kIdent,
		kUnion,
		kBlock,
		kSelf,
		kInstanceType,
		kId,
		kUnknown
	} m_kind;

	std::vector<Type *> m_unionMembers;

	Type()
	    : m_kind(kUnknown) {};
};

/*
 * Top-level declarations: namespace, class, pool.
 */
class DeclNode : public Node {
    public:
	DeclNode(Position pos)
	    : Node(pos) {};
};

/*
 * Variable (and formal parameter) declarations.
 */
struct VarDecl {
	std::string name;
	Type *type;
};

/*
 * Statements within a method or block. Defined here as required by
 * BlockExprNode.
 */
class StmtNode : public Node {
    public:
	StmtNode(Position pos)
	    : Node(pos) {};
};

class ExprNode : public Node {
    public:
	ExprNode(Position pos)
	    : Node(pos) {};

	virtual bool isIdent() { return false; }
	virtual bool isSuper() { return false; }
	virtual bool isSelf() { return false; }
};

/*
 * Literal expression. May be compile-time evaluable.
 */
struct LiteralExprNode : public ExprNode {

    public:
	LiteralExprNode(Position pos)
	    : ExprNode(pos) {};
};

/* Character literal */
struct CharExprNode : public LiteralExprNode {
	std::string khar;

	CharExprNode(Position pos, std::string aChar)
	    : LiteralExprNode(pos)
	    , khar(aChar)
	{
	}
};

/* Symbol literal */
struct SymbolExprNode : public LiteralExprNode {
	std::string sym;

	SymbolExprNode(Position pos, std::string aSymbol)
	    : LiteralExprNode(pos)
	    , sym(aSymbol)
	{
	}
};

/* Integer literal */
struct IntExprNode : public LiteralExprNode {
	int num;

	IntExprNode(Position pos, int aNum)
	    : LiteralExprNode(pos)
	    , num(aNum)
	{
	}

	void accept(Visitor &visitor) { visitor.visitIntExpr(this); }
};

/* String literal */
struct StringExprNode : public LiteralExprNode {
	std::string str;

	StringExprNode(Position pos, std::string aString)
	    : LiteralExprNode(pos)
	    , str(aString)
	{
	}
};

/* Integer literal */
struct FloatExprNode : public LiteralExprNode {
	double num;

	FloatExprNode(Position pos, double aNum)
	    : LiteralExprNode(pos)
	    , num(aNum)
	{
	}
};

/* Array literal */
struct ArrayExprNode : public LiteralExprNode {
	std::vector<ExprNode *> elements;

	ArrayExprNode(Position pos, std::vector<ExprNode *> exprs)
	    : LiteralExprNode(pos)
	    , elements(exprs)
	{
	}
};

struct IdentExprNode : ExprNode {
	std::string id;

	/* semantic analysis */
	Variable * variable;

	IdentExprNode(Position pos, std::string id)
	    : ExprNode(pos)
	    , id(id)
	{
	}

	void accept(Visitor &visitor) { visitor.visitIdentExpr(this); }

	bool isSuper() { return id == "super"; }
	bool isSelf() { return id == "self"; }
};

struct AssignExprNode : ExprNode {
	IdentExprNode *left;
	ExprNode *right;

	AssignExprNode(IdentExprNode *l, ExprNode *r)
	    : ExprNode(l->m_pos)
	    , left(l)
	    , right(r)
	{
	}

	void accept(Visitor &visitor) { visitor.visitAssignExpr(this); }
};

struct MessageExprNode : ExprNode {
	ExprNode *receiver;
	std::string selector;
	std::vector<ExprNode *> args;

	MessageExprNode(ExprNode *receiver, std::string selector,
	    std::vector<ExprNode *> args = {})
	    : ExprNode(receiver->m_pos)
	    , receiver(receiver)
	    , selector(selector)
	    , args(args)
	{
	}

	MessageExprNode(ExprNode *receiver,
	    std::vector<std::pair<std::string, ExprNode *>> list)
	    : ExprNode(receiver->m_pos)
	    , receiver(receiver)
	{
		for (auto &p : list) {
			selector += p.first;
			args.push_back(p.second);
		}
	}

	void accept(Visitor &visitor) { visitor.visitMessageExpr(this); }
};

struct CascadeExprNode : ExprNode {
	ExprNode *receiver;
	std::vector<MessageExprNode *> messages;

	CascadeExprNode(ExprNode *r)
	    : ExprNode(r->m_pos)
	    , receiver(r)
	{
		MessageExprNode *m = dynamic_cast<MessageExprNode *>(r);
		if (m) {
			receiver = m->receiver;
			messages.push_back(m);
		}
	}

	void accept(Visitor &visitor) { visitor.visitCascadeExpr(this); }
};

class BlockExprNode : public ExprNode {
    public:
	std::vector<VarDecl> m_args;
	std::vector<VarDecl> m_locals;
	std::vector<StmtNode *> m_stmts;

	/* semantic analysis */
	CodeScope * scope;

	BlockExprNode(std::vector<VarDecl> args, std::vector<VarDecl> locals,
	    std::vector<StmtNode *> stmts)
	    : ExprNode({})
	    , m_args(args)
	    , m_locals(locals)
	    , m_stmts(stmts)
	{
	}

	void accept(Visitor &visitor) { visitor.visitBlockExpr(this); }
	void print(int in);

	bool isSelf(); /**< in case inlined */
};

struct ExprStmtNode : public StmtNode {
	ExprNode *expr;

	ExprStmtNode(ExprNode *e)
	    : expr(e)
	    , StmtNode(e->m_pos)
	{
	}

	void accept(Visitor &visitor) { visitor.visitExprStmt(this); }
	virtual void print(int in);
};

struct ReturnStmtNode : public StmtNode {
	ExprNode *expr;

	ReturnStmtNode(ExprNode *e)
	    : StmtNode(e->m_pos)
	    , expr(e)
	{
	}

	void accept(Visitor &visitor) { visitor.visitReturnStmt(this); }
	void print(int in);
};

/*!
 * Method declaration node.
 */
class MethodNode : public DeclNode {
    public:
	bool m_isClassMethod;
	std::string m_selector;
	std::vector<VarDecl> m_parameters;
	std::vector<VarDecl> m_locals;
	std::vector<StmtNode *> m_statements;

	/* semantic analysis */
	CodeScope * scope;

	MethodNode(bool isClassMethod, std::string selector,
	    std::vector<VarDecl> parameters, std::vector<VarDecl> locals,
	    std::vector<StmtNode *> statements)
	    : DeclNode({})
	    , m_isClassMethod(isClassMethod)
	    , m_selector(selector)
	    , m_parameters(parameters)
	    , m_locals(locals)
	    , m_statements(statements) {};

	void accept(Visitor &visitor) { visitor.visitMethod(this); }
};

/*!
 * Class declaration node.
 */
class ClassNode : public DeclNode {
    public:
	std::string m_name;
	std::string m_superName;
	std::vector<VarDecl> m_instanceVars;
	std::vector<VarDecl> m_classVars;
	std::vector<MethodNode*> m_instanceMethods;
	std::vector<MethodNode*> m_classMethods;


	ClassNode(std::string name, std::string superName,
	    std::vector<VarDecl> instanceVars, std::vector<VarDecl> classVars)
	    : DeclNode({})
	    , m_name(name)
	    , m_superName(superName)
	    , m_instanceVars(instanceVars)
	    , m_classVars(classVars) {};

	void accept(Visitor &visitor) { visitor.visitClass(this); }

	void addMethods(std::vector<MethodNode *> meths);

};

} /* namespace AST */

#endif /* AST_H_ */
