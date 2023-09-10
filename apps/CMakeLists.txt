add_executable(presolve presolve.cpp)

target_link_libraries(presolve
        PUBLIC mipworkshop2024)

add_executable(postsolve postsolve.cpp)
target_link_libraries(postsolve
        PUBLIC mipworkshop2024)

add_executable(runSCIP runSCIP.cpp)
target_link_libraries(runSCIP
        PUBLIC mipworkshop2024)

add_executable(runIntegratedSCIP runIntegratedSCIP.cpp)
target_link_libraries(runIntegratedSCIP
        PUBLIC mipworkshop2024)

add_executable(mpsConvert mpsConvert.cpp)
target_link_libraries(mpsConvert
        PUBLIC mipworkshop2024)

add_executable(checkSolutions CheckSolutions.cpp)
target_link_libraries(checkSolutions
        PUBLIC mipworkshop2024)