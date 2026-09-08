# Anonymous Chat Core Module - wraps e_identity_sdk and e_moderation_sdk via FFI

cmake_minimum_required(VERSION 3.14)
project(AnonChatCorePlugin LANGUAGES CXX)

# Include the Logos Module CMake helper
if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LogosModule.cmake")
    include(cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found")
endif()

# Define the module with external libraries
logos_module(
    NAME anon_chat_core
    SOURCES 
        src/anon_chat_interface.h
        src/anon_chat_plugin.h
        src/anon_chat_plugin.cpp
    EXTERNAL_LIBS
        e_identity_sdk
        e_moderation_sdk
)
