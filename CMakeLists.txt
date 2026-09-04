cmake_minimum_required(VERSION 3.28)

project(geometry_dash
    VERSION 0.1.0
    LANGUAGES C CXX
)

# --- Norme C++ : la plus récente que ce compilateur ET ce CMake connaissent ---
# CMAKE_CXX_COMPILE_FEATURES est rempli par project() : il contient cxx_std_XX
# pour chaque norme réellement utilisable ici. On prend la plus haute, au lieu
# d'imposer un chiffre qui casse la configuration sur une autre machine.
set(GD_STANDARD_CANDIDATES 26 23 20 17)
set(GD_CXX_STANDARD 17)

foreach(candidate IN LISTS GD_STANDARD_CANDIDATES)
    if("cxx_std_${candidate}" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
        set(GD_CXX_STANDARD ${candidate})
        break()
    endif()
endforeach()

message(STATUS "Norme C++ retenue : C++${GD_CXX_STANDARD} (${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION})")

set(CMAKE_CXX_STANDARD ${GD_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(SFML 3 CONFIG REQUIRED COMPONENTS Graphics Window System Audio)

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
)

if(NOT PROJECT_SOURCES)
    message(FATAL_ERROR "Aucune source trouvée dans ${CMAKE_CURRENT_SOURCE_DIR}/src")
endif()

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_${GD_CXX_STANDARD})

target_link_libraries(${PROJECT_NAME} PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
)

# Copie Resources/ à côté de l'exécutable : l'AssetCache cherche depuis le
# répertoire courant, qui est celui du binaire et non la racine du projet.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Resources")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/Resources"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/Resources"
        COMMENT "Copie des ressources"
    )
endif()