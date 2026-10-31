find_package(OpenSSL REQUIRED COMPONENTS Crypto)

set(IIOT_SOURCES
    main.cc
    Attacker.cc
    Auth.cc
    Coap.cc
    ConfigParser.cc
    Cooler.cc
    Logger.cc
    PLC.cc
    Sensor.cc
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
