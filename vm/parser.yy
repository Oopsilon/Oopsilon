%include {
#include <cstdlib>
#include <list>

#define YY_NO_UNISTD_H

#include "ast.hh"
#include "driver.hh"
#include "parser.tab.h"
#include "scanner.ll.hh"

#define LEMON_SUPER MVST_Parser
using namespace AST;

struct SelectorNode {
	Type *m_retType;
	std::string m_sel;
	std::vector<VarDecl> m_params;
};

static std::string removeFirstChar (std::string aString)
{
	return aString.substr(1, aString.size() - 1);
}

static std::string removeFirstAndLastChar(std::string aString)
{
	return aString.substr(1, aString.size() - 2);
}
}

%token_type    {Token}
%token_prefix  TOK_
%left BAR.

%code {
std::vector<DeclNode*> MVST_Parser::parseFile (std::string fName, std::string src)
{
	MVST_Parser *parser;
	yyscan_t scanner;
	YY_BUFFER_STATE yyb;

	parser = MVST_Parser::create(fName, src);
	if (1)
		parser->trace(stdout, "<parser>: ");

	mvstlex_init_extra(parser, &scanner);
	/* Now we need to scan our string into the buffer. */
	yyb = mvst_scan_string(src.c_str(), scanner);

	while (mvstlex(scanner))
		;
	parser->parse(0);

	return parser->decls;
}

MethodNode * MVST_Parser::parseText (std::string src)
{
	MVST_Parser *parser;
	yyscan_t scanner;
	YY_BUFFER_STATE yyb;

	parser = MVST_Parser::create("<no-file>", src);
	if (1)//0)
		//parser->trace(stdout, "<parser>: ");

	mvstlex_init_extra(parser, &scanner);

	printf("Compiling immediate method: [ %s ]\n", src.c_str());
	/* Now we need to scan our string into the buffer. */
	yyb = mvst_scan_string(src.c_str(), scanner);

	//parser->parse(TOK_DIRECTMETH);
	while (mvstlex(scanner))
		;
	parser->parse(0);

	return parser->meth;
}

MVST_Parser * MVST_Parser::create(std::string fName, std::string & fText)
{
	return new yypParser(fName, fText);
}

Position MVST_Parser::pos()
{
	yypParser *self = (yypParser *)this;
	return Position(self->m_oldLine, self->m_oldCol, self->m_oldPos,
	    self->m_line, self->m_col, self->m_pos);
}

}

%syntax_error {
	const YYACTIONTYPE stateno = yytos->stateno;
	size_t eolPos = fText.find("\n", m_pos);
	std::string eLine = fText.substr(m_pos, eolPos - m_pos);
	size_t i;

	printf("PDST: %lu/%lu: Syntax error, unexpected %s\n", m_line, m_col,
	    yyTokenName[yymajor]);
	printf("+  %s\n", eLine.c_str());
	printf("+ \n");
	for (i = 0; i < m_oldCol; i++) {
		putchar(((char)eLine[i] == '\t' ? '\t' : ' '));
	}
	for (; i < m_col; i++)
		putchar('^');

	printf("\n\texpected one of: \n");
	for (unsigned i = 0; i < YYNTOKEN; ++i) {
		int yyact = yy_find_shift_action(i, stateno);
		if (yyact != YY_ERROR_ACTION && yyact != YY_NO_ACTION)
			printf("\t%s\n", yyTokenName[i]);
	}
}

prog ::= decl_list. { printf("Program finished!!\n"); }

%type decl_list { std::vector<DeclNode *> }
%type decl { DeclNode * }
%type class_def { ClassNode * }

decl_list ::= decl(d).
		{ decls.push_back(d); }
decl_list ::= decl_list(l) decl(d).
		{ decls.push_back(d); }

decl ::= class_def.


class_def(D) ::= identifier(super) type_args_opt(superTyArgs) SUBCLASSCOLON
    identifier(name) type_params_opt(tyParams) SQB_OPEN
    var_defs_opt(iVars) method_defs_opt(methods) SQB_CLOSE. {
	D = new ClassNode(name, super, iVars, {});
	D->addMethods(methods);
}

%type ivar_cvar_defs_opt { std::vector<VarDecl> }
%type var_defs_opt { std::vector<VarDecl> }
%type var_def_list_opt { std::vector<VarDecl> }
%type var_def_list { std::vector<VarDecl> }

/* optional bar-enclosed variable definition list */
var_defs_opt ::= .
var_defs_opt(L) ::= BAR var_def_list(l) BAR. { L = l; }

var_def_list(L) ::= type_spec_opt(t) IDENTIFIER(i). { L = {{i, t}}; }
var_def_list(L) ::= var_def_list(l) type_spec_opt(t) IDENTIFIER(i). {
	L = l;
	L.push_back({i, t});
}

%type method_defs_opt { std::vector<MethodNode *> }
%type method_defs { std::vector<MethodNode *> }
%type method_def { MethodNode * }

method_defs_opt(L) ::= method_defs(l). { L = l; }
method_defs_opt ::= .

method_defs(L) ::= method_def(d). { L = {d}; }
method_defs(L) ::= method_defs(l) method_def(d). {
	L = l;
	L.push_back(d);
}

method_def(D) ::= opt_class_meth_spec(isClass) selector_pattern(s)
    type_params_opt(tyParams) SQB_OPEN var_defs_opt(locals) statements(stmts)
	dot_opt SQB_CLOSE. {
	D = new MethodNode(isClass, s.m_sel, s.m_params, locals, stmts);
}

%type opt_class_meth_spec { bool }

opt_class_meth_spec(C) ::= . { C = false; }
opt_class_meth_spec(C) ::= CLASSRR. { C = true; }

%type statements { std::vector<StmtNode *> }
%type statement { StmtNode * }

statements(L) ::= statement(s). { L = {s}; }
statements(L) ::= statements(l) DOT statement(s). {
	L = l;
	L.push_back(s);
}

statement(S) ::= UP assign_expr(e). { S = new ReturnStmtNode(e);  }
statement(S) ::= assign_expr(e). { S = new ExprStmtNode(e); }

%type assign_expr { ExprNode * }
assign_expr(E) ::= identifier(i) ASSIGN continuation_expr(e). {
	E = new AssignExprNode(new IdentExprNode (i.pos(), i), e);
}
assign_expr(E) ::= continuation_expr(e). { E = e;}

%type continuation_expr { ExprNode * }
%type keyw_continuation_expr { CascadeExprNode * }
%type bin_continuation_expr { CascadeExprNode * }
%type unary_continuation_expr { CascadeExprNode * }
%type msg_expr { ExprNode * }
continuation_expr(C) ::= msg_expr(e). { C = e; }
continuation_expr(C) ::= keyw_continuation_expr(e). { C = e; }

keyw_continuation_expr(C) ::= bin_continuation_expr(c).
	{ C = c; }
keyw_continuation_expr(C) ::= bin_continuation_expr(c) keyw_msg_body(l). {
	C = c;
	C->messages.push_back(new MessageExprNode(C->receiver, l));
}

bin_continuation_expr(C) ::= unary_continuation_expr(c). { C = c; }
bin_continuation_expr(C) ::= bin_continuation_expr(c) binary_op(s)
	unary_expr(e). {
	C = c;
	C->messages.push_back(new MessageExprNode(C->receiver, s, {e}));
}


unary_continuation_expr(C) ::= continuation_expr(r) SEMICOLON. {
	/* FIXME: To avoid false positives, put bracketed expressions in
	 * their own thing. */
	CascadeExprNode * c = dynamic_cast<CascadeExprNode *> (r);
	C = c ? c : new CascadeExprNode (r);
}
unary_continuation_expr(C) ::= unary_continuation_expr(c) identifier(i). {
	C = c;
	C->messages.push_back(new MessageExprNode(C->receiver, i));
}

msg_expr(E) ::= binary_expr(b).
		{ E = b; }
msg_expr(E) ::= binary_expr(e) keyw_msg_body(k).
		{ E = new MessageExprNode(e, k); }

%type keyw_msg_body { std::vector<std::pair<std::string, ExprNode *>> }

keyw_msg_body(L) ::= keyword(k) binary_expr(e). {
	L = { {k, e}};
}
keyw_msg_body(L) ::= keyw_msg_body(l) keyword(k) binary_expr(e). {
	L = l;
	L.push_back( { k, e });
}

%type binary_expr { ExprNode * }
%type unary_expr { ExprNode * }
%type primary_expr { ExprNode * }

binary_expr(E) ::= unary_expr(e).
		{ E = e; }
binary_expr(E) ::= binary_expr(r) binary_op(s) unary_expr(a).
		{ E  = new MessageExprNode(r, s, { a }); }

unary_expr(E) ::= primary_expr(e).
		{ E = e; }
unary_expr(U) ::= unary_expr(p) identifier(i).
		{ U  = new MessageExprNode(p, i); }

primary_expr(S) ::= identifier(i).
		{ S = new IdentExprNode (i.pos(), i); }
primary_expr(S) ::= LBRACKET assign_expr(s) RBRACKET.
		{ S = s; }
primary_expr ::= block_expr.
primary_expr ::= literal_expr.
primary_expr(S) ::= PRIMNUM(n) primary_list_opt(l) RCARET.
		{ /*S = new PrimitiveExprNode(n.stringValue, l);*/ }

%type primary_list_opt { std::vector<ExprNode *> }
%type primary_list { std::vector<ExprNode *> }
primary_list_opt ::= .
primary_list_opt(L) ::= primary_list(l).
		{ L = l; }
primary_list(L) ::= primary_expr(p).
		{ L = {p}; }
primary_list(L) ::= primary_list(l) primary_expr(p). {
	L = l;
	L.push_back(p);
}

%type literal_expr { ExprNode * }
%type array_literal_member { ExprNode * }
%type basic_literal_expr { ExprNode * }

literal_expr(L) ::= HASH(beg) LBRACKET array_literal_members_opt(a)
    RBRACKET(end). {
	L = new ArrayExprNode({beg.pos(), end.pos()}, a);
}
literal_expr ::= basic_literal_expr.

%type array_literal_members { std::vector<ExprNode *> }
%type array_literal_members_opt { std::vector<ExprNode *> }

array_literal_members_opt ::= .
array_literal_members_opt(A) ::= array_literal_members(l).
		{A = l;}

array_literal_members(L) ::= array_literal_member(l).
		{ L = {l}; }
array_literal_members(L) ::= array_literal_members(l)
	array_literal_member(lit).{
	L = l;
	L.push_back(lit);
}

array_literal_member(A) ::= basic_literal_expr(i).
		{ A = i; }
array_literal_member(A) ::= identifier(i).
		{ A = new SymbolExprNode(i.pos(), i); }
array_literal_member(A) ::= keyword(k).
		{ A = new SymbolExprNode(k.pos(), k); }
array_literal_member(A) ::= LBRACKET(l) array_literal_members_opt(a)
    RBRACKET(r). {
	A = new ArrayExprNode({l.pos(), r.pos()}, a);
}
array_literal_member(A) ::= HASH(beg) LBRACKET array_literal_members_opt(a)
    RBRACKET(end). {
	A = new ArrayExprNode({beg.pos(), end.pos()}, a);
}

basic_literal_expr(S) ::= STRING(s).{
	S = new StringExprNode(s.pos(), removeFirstAndLastChar(s));
}
basic_literal_expr(S) ::= SYMBOL(s). {
	S = new SymbolExprNode(s.pos(), removeFirstChar(s));
}
basic_literal_expr(S) ::= INTEGER(i). {
	S = new IntExprNode(i.pos(), i.intValue);
}
basic_literal_expr(S) ::= FLOAT(f). {
	S = new FloatExprNode(f.pos(), f);
}
basic_literal_expr(S) ::= CHAR(c). {
	S = new CharExprNode(c.pos(), removeFirstChar(c));
}

%type block_formal_list_opt { std::vector<VarDecl> }
block_formal_list_opt(L) ::= colon_var_list(l) BAR.
		{ L = l; }
block_formal_list_opt(L) ::= colon_var_list(l) LBRACKET UP type RBRACKET
    BAR.
    		{ L = l; }
block_formal_list_opt    ::= .

%type block_expr { BlockExprNode * }
block_expr(B) ::= SQB_OPEN block_formal_list_opt(params) var_defs_opt(locals)
    statements_opt(stmts) SQB_CLOSE. {
	B = new BlockExprNode(params, locals, stmts);
}

dot_opt ::= .
dot_opt ::= DOT.

%type statements_opt { std::vector<StmtNode *> }
statements_opt ::= .
statements_opt(L) ::= statements(l).
		{ L = l; }

%type colon_var_list { std::vector<VarDecl> }

colon_var_list(L) ::= COLONVAR(v). {
	std::string s = v;
	s.erase(s.begin());
	L.push_back({s, NULL});
}

colon_var_list(L) ::= colon_var_list(l) COLONVAR(v). {
	L = l;
	std::string s = v;
	s.erase(s.begin());
	L.push_back({s, NULL});
}

%type selector_pattern { SelectorNode }
selector_pattern(S) ::= type_spec_opt(t) identifier(i). {
	S = SelectorNode{t, i, {}};
}
selector_pattern(S) ::= type_spec_opt(t) binary_pattern(b). {
	S = SelectorNode{t, b.first, {b.second}};
}
selector_pattern(S) ::= type_spec_opt(t) keyw_pattern(k). {
	S = SelectorNode{t, k.first, k.second};
}

%type keyw_pattern { std::pair<std::string, std::vector<VarDecl>> }
%type keyw_pattern_part { std::pair<std::string, VarDecl> }
keyw_pattern(L) ::= keyw_pattern_part(k).
		{L = {k.first, {k.second}};}
keyw_pattern(L) ::= keyw_pattern(l) keyw_pattern_part(k). {
	L = l;
	L.first += k.first;
	L.second.push_back(k.second);
}
keyw_pattern_part(K) ::= keyword(k) type_spec_opt(t) identifier(s). {
    K = {k, {s, t}};
}

%type binary_pattern { std::pair<std::string, VarDecl> }
binary_pattern(B) ::= binary_op(b) type_spec_opt(t) identifier(s). {
    B = {b, {s, t}};
}

identifier(I) ::= IDENTIFIER(i).
		{ I = i; }
identifier(I) ::= NAMESPACENAME(i).
		{ I = i; }
identifier(I) ::= NAMESPACE.
		{ I = Token(I.m_pos, "Namespace"); }

keyword(K) ::= KEYWORD(k).
		{ K = k; }
keyword(K) ::= SUBCLASSCOLON(s).
		{ K = s; K.stringValue = "subclass:"; }
keyword(K) ::= CURRENTCOLON(s).
		{ K = s; K.stringValue = "current:"; }

%type binary_op { std::string }
%type binary_op_part { std::string }
binary_op(B) ::= binary_op_part(c).
		{ B = c; }
binary_op(B) ::= binary_op(b) binary_op_part(c).
		{ B = b; B += c; }
binary_op_part(B) ::= BINARY(b).
		{ B = b.stringValue; }
binary_op_part(B) ::= LCARET.
		{ B = std::string("<"); }
binary_op_part(B) ::= RCARET.
		{ B = std::string(">"); }
binary_op_part(B) ::= COMMA.
		{ B = std::string(","); }

%type type_spec_opt { Type* }
%type type_spec { Type* }
%type type { Type* }
%type type_args_opt { std::vector<Type*> }
%type type_arg_parts { std::vector<Type*> }
%type type_params_opt { std::vector<VarDecl> }
%type type_param_parts { std::vector<VarDecl> }

type_spec_opt ::= type_spec.
type_spec_opt ::= .
type_spec(T) ::= LBRACKET type(t) RBRACKET.
		{ T = t; }

/* basic or parameterised type */
type(T) ::= identifier(ident) type_args_opt(args).
		{ T = new Type(/*ident, args*/); }
/* union type */
type(T) ::= type(t1) BAR type(t2). {
	if (t1->m_kind == Type::kUnion) {
		t1->m_unionMembers.push_back(t2);
		T = t1;
	} else {
		T = new Type;
		T->m_kind = Type::kUnion;
		T->m_unionMembers.push_back(t1);
		T->m_unionMembers.push_back(t2);
	}
}
/* block type */
type(T) ::= SQB_OPEN UP type(r) block_arg_types_opt(a) SQB_CLOSE. {
	T = new Type;
}

%type block_arg_types_opt { std::vector<Type *> }
%type block_arg_types { std::vector<Type *> }
block_arg_types_opt(L) ::= block_arg_types(a).
		{ L = a; }
block_arg_types_opt ::= .
block_arg_types(L) ::= COMMA type(t).
		{ L = {t}; }
block_arg_types(L) ::= block_arg_types(l) COMMA type(t). {
	L = l;
	L.push_back(t);
}

type_args_opt(A) ::= LCARET type_arg_parts(a) RCARET. { A = a; }
type_args_opt ::= .

type_arg_parts(A) ::= type(t). {
	A.push_back(t);
}
type_arg_parts(A) ::= type_arg_parts(a) COMMA type(t). {
	A = std::move(a);
	A.push_back(t);
}

type_params_opt(P) ::= LCARET type_param_parts(p) RCARET.
		{P = p;}
type_params_opt ::= .

type_param_parts(L) ::= type_spec_opt(t) identifier(id).
		{ L.push_back({id, t}); }
type_param_parts(L) ::= type_param_parts(l) COMMA type_spec_opt(t)
    identifier(id). {
	L = std::move(l);
	L.push_back({id, t});
}
