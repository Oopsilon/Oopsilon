#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "analyse.hh"
#include "ast.hh"
#include "driver.hh"
#include "generate.hh"

template <typename T>void visit(std::vector<AST::DeclNode*>& decls)
{
	for (auto decl: decls) {
	T visitor;
	decl->accept(visitor);
	}
}

int
main(int argc, char *argv[])
{
	assert(argc >= 2);

	std::cout << "NetaScale(tm) Optimising Compiler for Valutron\n";
	std::cout << "Compiling " << argc - 1 << " files\n";

	auto outDir = std::filesystem::current_path() / "valuout";

	std::vector<AST::DeclNode *> decls;

	for (int i = 1; i < argc; i++) {
		std::cout << "Parsing " << argv[i] << "...\n";
		std::string fname = argv[i];
		std::ifstream t(fname);
		std::stringstream buffer;
		buffer << t.rdbuf();
		auto parsed = MVST_Parser::parseFile(fname, buffer.str());
		decls.insert(decls.end(), parsed.begin(), parsed.end());
	}
	std::cout <<"Analysing (global names)...\n";
	visit<RegistrarVisitor>(decls);
	visit<LinkupVisitor>(decls);

	std::cout << "Analysing (semantic)...\n";
	for (auto decl : decls) {
		AnalysisVisitor visitor;
		decl->accept(visitor);
	}

	std::cout << "Analysing (closure)...\n";
	for (auto decl : decls) {
		ClosureAnalysisVisitor visitor;
		decl->accept(visitor);
	}

	std::cout << "Generating code...\n";
	std::vector<std::string> classes;
	for (auto decl : decls) {
		AST::ClassNode *klass = dynamic_cast<AST::ClassNode *>(decl);
		if (klass) {
			classes.push_back(klass->m_name);
			CodeGeneratorVisitor visitor(outDir, klass);
		} else {
			// AST::NamespaceNode
			assert(!"Unreached");
		}
	}

	std::ofstream mod(outDir / "mod.c");
	mod << "#include <libruntime/vtrt.h>"
	       "\n"
	       "\nint main(int argc, char *argv[])"
	       "\n{";
	for (auto &klass : classes) {
		mod << "\n  extern struct vtrt_classTemplate " << klass
		    << ";"
		       "\n  vtrt_registerClass(\""
		    << klass << "\", &" << klass << ");";
	}
	mod << "\n  vtrt_main(argc, argv);"
	       "\n}"
	       "\n";

	return 0;
}
