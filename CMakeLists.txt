cmake_minimum_required(VERSION 3.10)

project(AnalogPulse LANGUAGES CXX)

# The three applications share the gauge: GaugeView draws, animates and
# archives the panel, the subclasses only supply the value and the text.
set(GAUGE_SOURCES
	src/Source/GaugeView.cpp
	src/Source/panel.cpp
)

# Single-pane window, used by AnalogRAM and AnalogDisk
set(APP_SOURCES
	src/Source/GaugeWindow.cpp
)

# AnalogPulse: one pane per CPU
add_executable(AnalogPulse
	src/Source/AnalogMain.cpp
	src/Source/AnalogView.cpp
	src/Source/AnalogWindow.cpp
	${GAUGE_SOURCES}
)

# AnalogRAM: memory in use
add_executable(AnalogRAM
	src/Source/AnalogRAM.cpp
	${APP_SOURCES}
	${GAUGE_SOURCES}
)

# AnalogDisk: boot volume in use
add_executable(AnalogDisk
	src/Source/AnalogDisk.cpp
	${APP_SOURCES}
	${GAUGE_SOURCES}
)

foreach(app AnalogPulse AnalogRAM AnalogDisk)
	target_include_directories(${app} PRIVATE src/Headers)
	target_link_libraries(${app} PRIVATE be)
	target_compile_options(${app} PRIVATE -Wno-multichar)
	set_target_properties(${app} PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
endforeach()

# --- Haiku application resources --------------------------------------------
# The resources carry the MIME signature.  When a replicant is dropped, the
# receiving application looks the executable up by that signature and
# load_add_on()s it to call <Class>::Instantiate, so a binary without
# resources still runs but cannot provide replicants.
find_program(RC_EXECUTABLE rc)
find_program(XRES_EXECUTABLE xres)
find_program(MIMESET_EXECUTABLE mimeset)

function(analog_add_resources app rsrc)
	add_custom_command(TARGET ${app} POST_BUILD
		COMMAND "${XRES_EXECUTABLE}" -o "$<TARGET_FILE:${app}>" "${rsrc}"
		COMMENT "Embedding Haiku resources into ${app}")

	if(MIMESET_EXECUTABLE)
		add_custom_command(TARGET ${app} POST_BUILD
			COMMAND "${MIMESET_EXECUTABLE}" -f "$<TARGET_FILE:${app}>"
			COMMENT "Registering ${app} with the MIME database")
	endif()
endfunction()

function(analog_compile_and_add_resources app rdef)
	add_custom_command(TARGET ${app} POST_BUILD
		COMMAND "${RC_EXECUTABLE}" -o "${CMAKE_CURRENT_BINARY_DIR}/${app}.rsrc"
			"${rdef}"
		COMMENT "Compiling ${app}.rdef")

	analog_add_resources(${app} "${CMAKE_CURRENT_BINARY_DIR}/${app}.rsrc")
endfunction()

if(XRES_EXECUTABLE)
	# AnalogPulse is stamped with the resource file it has always carried
	analog_add_resources(AnalogPulse
		"${CMAKE_CURRENT_SOURCE_DIR}/src/Source/AnalogPulse_Lite.rsrc")

	if(RC_EXECUTABLE)
		foreach(app AnalogRAM AnalogDisk)
			analog_compile_and_add_resources(${app}
				"${CMAKE_CURRENT_SOURCE_DIR}/src/Source/${app}.rdef")
		endforeach()
	else()
		message(WARNING
			"rc was not found, so AnalogRAM and AnalogDisk are built without "
			"resources and cannot be used as replicants.")
	endif()
else()
	message(WARNING
		"xres was not found, so the applications are built without Haiku "
		"resources and cannot be used as replicants.")
endif()
