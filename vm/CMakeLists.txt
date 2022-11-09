FlexComp(scanner.ll)
LemonComp(parser.yy)

add_subdirectory(mir)

add_executable(vm ast.cc main.cc ${CMAKE_CURRENT_BINARY_DIR}/scanner.ll.cc
    ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.cc)
target_include_directories(vm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/vendor/lemonpp)
