#include <cassert>
#include <fstream>
#include <sstream>

#include "driver.hh"

int
main(int argc, char *argv[])
{
	assert(argc == 2);

	std::string fname = argv[1];
	std::ifstream t(fname);
	std::stringstream buffer;

	buffer << t.rdbuf();
	MVST_Parser::parseFile(argv[1], buffer.str());

	return 42;
}
