find_package(OpenSSL REQUIRED COMPONENTS Crypto)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(IIOT_SOURCES
    main.cc
    attacker/Attacker.cc
    utils/Auth.cc
    protocol/Coap.cc
    utils/ConfigParser.cc
    cooler/Cooler.cc
    logger/Logger.cc
    plc/PLC.cc
    sensor/Sensor.cc
)

build_exec(
    EXECNAME main
    EXECNAME_PREFIX scratch_iiot-basic_
    SOURCE_FILES "${IIOT_SOURCES}"
    LIBRARIES_TO_LINK
        "${ns3-libs}"
        "${ns3-contrib-libs}"
        OpenSSL::Crypto
    EXECUTABLE_DIRECTORY_PATH ${CMAKE_OUTPUT_DIRECTORY}/scratch/iiot-basic
)
