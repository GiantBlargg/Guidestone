function(make_shader_lib lib_name namespace target_env source_path)
	find_program(GLSLC_EXECUTABLE
		NAMES glslc
		HINTS "$ENV{VULKAN_SDK}/bin"
		REQUIRED
		)

	cmake_parse_arguments(PARSE_ARGV 4 ARG "" "" "")

	set(sources ${ARG_UNPARSED_ARGUMENTS}) 

	set(BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/${lib_name})

	set(shaders_hpp ${BINARY_DIR}/include/${lib_name}.hpp)
	file(WRITE ${shaders_hpp} "#pragma once\n#include <vector>\n#include <cstdint>\nnamespace ${namespace} {\n")

	foreach(rel_path ${sources})
		set(glsl_file ${source_path}/${rel_path})
		set(spv_file ${BINARY_DIR}/${rel_path}.spv)
		set(dep_file ${BINARY_DIR}/${rel_path}.d)
		set(cpp_file ${BINARY_DIR}/${rel_path}.cpp)
		add_custom_command(
			COMMAND ${GLSLC_EXECUTABLE}
				${glsl_file} -o ${spv_file}
				--target-env=${target_env} -mfmt=c
				-MD -MF ${dep_file}

			OUTPUT ${spv_file}
			DEPENDS ${glsl_file}
			DEPFILE ${dep_file}
		)
		string(REGEX REPLACE "[^a-zA-Z0-9]" "_" name ${rel_path})
		file(APPEND ${shaders_hpp} "extern const std::vector<uint32_t> ${name};\n")
		file(WRITE ${cpp_file} "#include \"shaders.hpp\"\nconst std::vector<uint32_t> ${namespace}::${name} = \n#include \"${rel_path}.spv\"\n;\n")
		set(SPIRV_SHADERS ${SPIRV_SHADERS} ${cpp_file} ${spv_file})
	endforeach()

	file(APPEND ${shaders_hpp} "}\n")

	add_library(${lib_name} ${SPIRV_SHADERS} ${shaders_hpp})
	target_include_directories(${lib_name} PUBLIC ${BINARY_DIR}/include)
endfunction()
