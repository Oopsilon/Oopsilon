find_package(FLEX)

add_executable (lemonxx lemon.c)
target_compile_definitions (lemonxx PUBLIC LEMONPLUSPLUS)

function(FlexComp path)
    GET_FILENAME_COMPONENT(file ${path} NAME_WE)
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${file}.ll.hh
          ${CMAKE_CURRENT_BINARY_DIR}/${file}.ll.cc
      COMMAND ${FLEX_EXECUTABLE}
      ARGS -o ${CMAKE_CURRENT_BINARY_DIR}/${file}.ll.cc
          --header-file=${CMAKE_CURRENT_BINARY_DIR}/${file}.ll.hh
          ${CMAKE_CURRENT_SOURCE_DIR}/${path}
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${path})
endfunction(FlexComp)

function (LemonComp path)
    GET_FILENAME_COMPONENT (file ${path} NAME_WE)
    add_custom_command(
        OUTPUT ${file}.tab.cc ${file}.tab.h ${file}.out
        COMMAND $<TARGET_FILE:lemonxx>
        ARGS ${CMAKE_CURRENT_SOURCE_DIR}/${path}
            -d. -p -T${CMAKE_SOURCE_DIR}/vendor/lemonpp/lemxx.tpl
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${path})
endfunction (LemonComp)
