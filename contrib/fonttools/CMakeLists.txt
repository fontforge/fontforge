# Distributed under the original FontForge BSD 3-clause license

# Use these at your own risk, these are mostly unmaintained.

add_executable(acorn2sfd acorn2sfd.c)
add_executable(dewoff dewoff.c)
add_executable(findtable findtable.c)
add_executable(pcl2ttf pcl2ttf.c)
add_executable(pfadecrypt pfadecrypt.c)
add_executable(rmligamarks rmligamarks.c)
add_executable(showttf showttf.c)
add_executable(stripttc stripttc.c)
add_executable(ttf2eps ttf2eps.c)
add_executable(woff woff.c)

target_link_libraries(acorn2sfd PRIVATE fontforge)
target_link_libraries(dewoff PRIVATE ZLIB::ZLIB)
target_link_libraries(pcl2ttf PRIVATE MathLib::MathLib)
target_link_libraries(ttf2eps PRIVATE fontforge)
target_link_libraries(woff PRIVATE ZLIB::ZLIB)
