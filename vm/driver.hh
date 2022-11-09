#ifndef COMPILER_HH_
#define COMPILER_HH_

#include <list>
#include <stack>
#include <string>
#include <vector>

#include "lemon/lemon_base.h"

#include "ast.hh"

#if 0
struct MethodNode;
class ClassNode;
struct NameScope;
class VarNode;
class GlobalVarNode;
class ProgramNode;
#endif
struct ProgramNode;

class MVST_Parser : public lemon_base<AST::Token> {
    protected:
	std::string fName;
	std::string path;
	std::string &fText;
	ProgramNode *program;
	AST::MethodNode *meth;
	size_t m_line = 0, m_col = 0, m_pos = 0;
	size_t m_oldLine = 0, m_oldCol = 0, m_oldPos = 0;

    public:
	using lemon_base::parse;

	int line() const { return m_line; }
	int col() const { return m_col; }
	int pos1() const { return m_pos; }

	static ProgramNode *parseFile(std::string fName, std::string text);
	static AST::MethodNode *parseText(std::string text);
	static MVST_Parser *create(std::string fName, std::string &fText);

	MVST_Parser(std::string f, std::string &ft)
	    : fName(f)
	    , fText(ft)
	    , program(nullptr) {};

	/* parsing */
	void parse(int major) { parse(major, AST::Token { pos() }); }

	template <class T> void parse(int major, T &&t)
	{
		parse(major, AST::Token(pos(), std::forward<T>(t)));
	}

	virtual void trace(FILE *, const char *) { }

	/* line tracking */
	AST::Position pos();

	/* record current position into the old position members */
	void recOldPos()
	{
		m_oldPos = m_pos;
		m_oldLine = m_line;
		m_oldCol = m_col;
	}

	/* accept carriage return */
	void cr()
	{
		m_pos += m_col + 1;
		m_line++;
		m_col = 0;
	}

	/* increment column */
	void incCol() { m_col++; }
};

#endif /* COMPILER_HH_ */
