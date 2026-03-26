cmake_minimum_required(VERSION 3.9)
project(libcsp_docs)

set(SPHINX_EXECUTABLE sphinx-build)
set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/html)
set(SPHINX_INDEX_FILE ${SPHINX_BUILD}/index.html)

file(GLOB DOCS_FILES
    "${SPHINX_SOURCE}/*.md"
    "${SPHINX_SOURCE}/api/*.rst"
    "${SPHINX_SOURCE}/api/arch/*.rst"
    "${SPHINX_SOURCE}/api/crypto/*.rst"
    "${SPHINX_SOURCE}/api/drivers/*.rst"
    "${SPHINX_SOURCE}/api/interfaces/*.rst")

file(COPY ${SPHINX_SOURCE}/.nojekyll DESTINATION ${SPHINX_BUILD})
add_custom_target(docs ALL DEPENDS ${SPHINX_INDEX_FILE})
add_custom_command(OUTPUT ${SPHINX_INDEX_FILE}
    COMMAND
        ${SPHINX_EXECUTABLE} -E -b html ${SPHINX_SOURCE} ${SPHINX_BUILD} ${DOCS_FILES}
    MAIN_DEPENDENCY conf.py
    COMMENT "Building DOCS for lib CSP")
