cmake_minimum_required(VERSION 3.16)
project(dron_companion LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Núcleo de evasión: sin dependencias externas para poder probarlo en cualquier PC
# y compilarlo igual en la Raspberry Pi 5.
add_library(evasion
  src/amenaza.cpp
  src/planificador.cpp
  src/percepcion.cpp
  src/supervisor.cpp
  src/simulacion.cpp
)
target_include_directories(evasion PUBLIC include)
target_compile_options(evasion PRIVATE -Wall -Wextra -Wpedantic)

add_executable(simular_piedra src/simular_piedra.cpp)
target_link_libraries(simular_piedra PRIVATE evasion)

enable_testing()
add_executable(pruebas_evasion pruebas/pruebas_evasion.cpp)
target_link_libraries(pruebas_evasion PRIVATE evasion)
target_compile_options(pruebas_evasion PRIVATE -Wall -Wextra)
add_test(NAME pruebas_evasion COMMAND pruebas_evasion)

add_executable(pruebas_vision pruebas/pruebas_vision.cpp)
target_link_libraries(pruebas_vision PRIVATE evasion)
target_compile_options(pruebas_vision PRIVATE -Wall -Wextra)
add_test(NAME pruebas_vision COMMAND pruebas_vision)
