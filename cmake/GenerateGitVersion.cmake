set(version "unknown")

if(EXISTS "${SOURCE_DIR}/.git")
  execute_process(
    COMMAND git -C "${SOURCE_DIR}" describe --tags --always --dirty --long
    RESULT_VARIABLE git_describe_result
    OUTPUT_VARIABLE git_describe_output
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(git_describe_result EQUAL 0 AND NOT git_describe_output STREQUAL "")
    set(version "${git_describe_output}")
  endif()
endif()

string(REPLACE "\\" "\\\\" version "${version}")
string(REPLACE "\"" "\\\"" version "${version}")
set(header "#ifndef SHANDALAR_BUILD_VERSION_H\n#define SHANDALAR_BUILD_VERSION_H\n\n#define SHANDALAR_BUILD_VERSION \"${version}\"\n\n#endif\n")

get_filename_component(output_directory "${OUTPUT_FILE}" DIRECTORY)
file(MAKE_DIRECTORY "${output_directory}")

set(existing_header "")
if(EXISTS "${OUTPUT_FILE}")
  file(READ "${OUTPUT_FILE}" existing_header)
endif()
if(NOT existing_header STREQUAL header)
  file(WRITE "${OUTPUT_FILE}" "${header}")
endif()
