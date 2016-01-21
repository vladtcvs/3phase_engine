cmake_minimum_required(VERSION 2.8)
project(firmware)

add_definitions(-Wall)

set(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_CROSSCOMPILING 1)

set(CMAKE_C_COMPILER "/usr/bin/avr-gcc")
set(CMAKE_CXX_COMPILER "/usr/bin/avr-g++")
set(CMAKE_OBJCOPY "avr-objcopy")
set(CMAKE_AVRDUDE "avrdude")
set(MKEE ${PROJECT_BINARY_DIR}/mkee)

set(CSTANDARD "-std=gnu99")
set(CDEBUG "-gstabs")
set(CWARN "-Wall -Wstrict-prototypes")
set(CTUNING "-funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums")
set(PPORT "/dev/ttyUSB0")
set(AVRDUDEMCU "m48p")

set(COPT "-Os")
set(CMCU "-mmcu=atmega48pa")

set(CFLAGS "${CMCU} ${CDEBUG} ${CDEFS} ${CINCS} ${COPT} ${CWARN} ${CSTANDARD} ${CEXTRA}")
set(CXXFLAGS "${CMCU} ${CDEFS} ${CINCS} ${COPT}")
set(CMAKE_C_FLAGS  ${CFLAGS})
set(CMAKE_CXX_FLAGS ${CXXFLAGS})
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

set(SOURCE_FILES code-c.c uart.c cmd.c)

set(HEADERS_FILES )

add_custom_target(mksin gcc -o mksin ${CMAKE_SOURCE_DIR}/mksin.c -lm)
add_custom_target(mkee gcc -o mkee ${CMAKE_SOURCE_DIR}/mkee.c)
add_executable(fw.elf ${HEADERS_FILES} ${SOURCE_FILES})
add_custom_target(fw.hex ${CMAKE_OBJCOPY} -j .text -j .data -O ihex fw.elf fw.hex)
add_custom_target(programm ${CMAKE_AVRDUDE} -p ${AVRDUDEMCU} -P ${PPORT} -c stk500 -U flash:w:fw.hex -U eeprom:w:eesin.bin)
add_custom_target(write_ee ${CMAKE_AVRDUDE} -p ${AVRDUDEMCU} -P ${PPORT} -c stk500 -U eeprom:w:eesin.bin)
add_custom_target(test_connection ${CMAKE_AVRDUDE} -p ${AVRDUDEMCU} -P ${PPORT} -c stk500)
add_custom_target(eesin.bin ${MKEE} eesin.bin ${CMAKE_SOURCE_DIR}/sin.txt ${CMAKE_SOURCE_DIR}/ampl.txt ${CMAKE_SOURCE_DIR}/freq.txt ${CMAKE_SOURCE_DIR}/ample_freq.txt)

add_dependencies(eesin.bin mkee)
add_dependencies(fw.hex fw.elf)
add_dependencies(programm fw.hex)
add_dependencies(write_ee eesin.bin)
