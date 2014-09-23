set(
	UCHARDET_SOURCES
	uchardet.cpp
)

add_executable(
	uchardet
	${UCHARDET_SOURCES}
)

target_link_libraries(
	uchardet
	${UCHARDET_TARGET}
)

install(
	TARGETS
		uchardet
	RUNTIME
	DESTINATION
		${DIR_BIN}
)

