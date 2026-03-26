# =============================================================================
# Static Analysis (Optional)
# =============================================================================

option(ENABLE_CPPCHECK "Enable static analysis with Cppcheck" ON)

if(ENABLE_CPPCHECK)
    find_program(CPPCHECK_EXECUTABLE "cppcheck")

    if(CPPCHECK_EXECUTABLE)
        set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

        # Target to generate the XML file
	add_custom_target(${PROJECT_NAME}_cppcheck
		COMMAND ${CPPCHECK_EXECUTABLE} -D__GNUC__
	    --project=${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
                --xml
                --xml-version=2
		--output-file=${CMAKE_CURRENT_BINARY_DIR}/cppcheck-report.xml
                # ... other flags ...
            COMMENT "Running Cppcheck analysis... Report will be at cppcheck-report.xml"
            VERBATIM
        )
        message(STATUS "Cppcheck target added. Run with 'make cppcheck'.")

        # --- Add a target to generate an HTML report from the XML output ---
        find_program(CPPCHECK_HTMLREPORT_SCRIPT "cppcheck-htmlreport")

        if(CPPCHECK_HTMLREPORT_SCRIPT)
            add_custom_target(${PROJECT_NAME}_cppcheck-html
                COMMAND ${CPPCHECK_HTMLREPORT_SCRIPT}
		--file=${CMAKE_CURRENT_BINARY_DIR}/cppcheck-report.xml
			--source-dir=${CMAKE_CURRENT_SOURCE_DIR}
			--report-dir=${CMAKE_CURRENT_BINARY_DIR}/cppcheck-html
                COMMENT "Generating Cppcheck HTML report in 'build/${PROJECT_NAME}/cppcheck-html/' directory..."
                VERBATIM
            )

            # Make the 'cppcheck-html' target depend on the 'cppcheck' target.
            add_dependencies(${PROJECT_NAME}_cppcheck-html ${PROJECT_NAME}_cppcheck)

	    message(STATUS "Cppcheck HTML report target added. Run with '${CMAKE_GENERATOR} ${PROJECT_NAME}_cppcheck-html'.")
        else()
            message(STATUS "cppcheck-htmlreport script not found, HTML target disabled.")
        endif()
    else()
        message(WARNING "ENABLE_CPPCHECK is ON, but the Cppcheck executable was not found.")
    endif()
endif()
