﻿CMAKE_MINIMUM_REQUIRED (VERSION 3.5)

PROJECT (test CXX)

SET(TARGETSVR TestSvr)
SET(TARGETCLT TestClt)

#FILE(GLOB_RECURSE MTHREADTEST_HEADERS *.h)
#AUX_SOURCE_DIRECTORY(. MTHREADTEST_SRCS)

ADD_EXECUTABLE(${TARGETSVR} TCPServerSpi.h Server.cpp TCPServerSpi.cpp)
ADD_EXECUTABLE(${TARGETCLT} TCPClientSpi.h Client.cpp TCPClientSpi.cpp)

TARGET_INCLUDE_DIRECTORIES(${TARGETSVR} PRIVATE ${INC_PATH})
TARGET_LINK_DIRECTORIES(${TARGETSVR} PRIVATE ${LIB_PATH})
TARGET_INCLUDE_DIRECTORIES(${TARGETCLT} PRIVATE ${INC_PATH})
TARGET_LINK_DIRECTORIES(${TARGETCLT} PRIVATE ${LIB_PATH})

TARGET_LINK_LIBRARIES(${TARGETSVR} PRIVATE ${TARGETBIN})
TARGET_LINK_LIBRARIES(${TARGETCLT} PRIVATE ${TARGETBIN})

SET_TARGET_PROPERTIES(${TARGETSVR} PROPERTIES OUTPUT_NAME "${TARGETSVR}${ARCH}$<$<CONFIG:Debug>:D>")
SET_TARGET_PROPERTIES(${TARGETCLT} PROPERTIES OUTPUT_NAME "${TARGETCLT}${ARCH}$<$<CONFIG:Debug>:D>")
