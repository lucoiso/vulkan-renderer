# Author: Lucas Vilas-Boas
# Year: 2023
# Repo: https://github.com/lucoiso/VulkanRenderer

# ----------- Global Definitions -----------
SET(LIBRARY_NAME RenderCoreBenchmark)

# ------------- Library Setup --------------
SET(PRIVATE_MODULES
	Modules/Private/RenderCoreBenchmark.cpp
	Modules/Private/Utils.ixx
	Modules/Private/RenderCore.ixx
	Modules/Private/Helpers.ixx
)

ADD_EXECUTABLE(${LIBRARY_NAME} ${PRIVATE_MODULES})
SET_TARGET_PROPERTIES(${LIBRARY_NAME} PROPERTIES LINKER_LANGUAGE CXX)

FIND_PACKAGE(benchmark CONFIG REQUIRED)

TARGET_LINK_LIBRARIES(${LIBRARY_NAME} PRIVATE
					  benchmark::benchmark_main
					  RenderCore
)

IF (BUILD_TYPE STREQUAL "Debug")
	FIND_PACKAGE(Boost REQUIRED COMPONENTS log)
	TARGET_LINK_LIBRARIES(${LIBRARY_NAME} PRIVATE Boost::log)
ENDIF (BUILD_TYPE STREQUAL "Debug")