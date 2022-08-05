function (make_example name)
	add_executable(${name} ${name}.cpp)
	set_property(TARGET ${name} PROPERTY CXX_STANDARD 17)
	target_link_libraries(${name} PRIVATE ${PROJECT_NAME})
endfunction()


make_example(example1)
make_example(example2)
make_example(example3)
make_example(example4)
