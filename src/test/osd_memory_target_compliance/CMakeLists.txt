# OSD memory target compliance test

add_executable(ceph_test_osd_memory_target_compliance
  main.cc)
target_link_libraries(ceph_test_osd_memory_target_compliance
  librados ${UNITTEST_LIBS} Boost::system StdFilesystem::filesystem)
install(TARGETS ceph_test_osd_memory_target_compliance
  DESTINATION ${CMAKE_INSTALL_BINDIR})
