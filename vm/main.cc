#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>

#include "analyse.hh"
#include "driver.hh"
#include "generate.hh"

int
main(int argc, char *argv[])
{
	assert(argc == 2);

	std::cout << "NetaScale(tm) Optimising Compiler for Valutron\n";
	std::cout << "Compiling " << argv[1] << "\n";

	std::string fname = argv[1];
	std::ifstream t(fname);
	std::stringstream buffer;

	buffer << t.rdbuf();

	std::cout << "Parsing...\n";
	auto decls = MVST_Parser::parseFile(argv[1], buffer.str());

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
	for (auto decl : decls) {
		CodeGeneratorVisitor visitor;
		decl->accept(visitor);
	}

	return 42;
}
