file(GLOB HEADERS "include/graphene/net_stats/*.hpp")

add_library( graphene_net_stats
             net_stats.cpp
             ${HEADERS}
           )
find_package(prometheus-cpp REQUIRED core push pull)

target_link_libraries( graphene_net_stats 
   PUBLIC graphene_chain 
   PUBLIC graphene_app 
   PUBLIC prometheus-cpp::core
   PUBLIC prometheus-cpp::push
   PUBLIC prometheus-cpp::pull
)

target_include_directories( graphene_net_stats
   PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
   PUBLIC "${prometheus-cpp_INCLUDE_DIR}"
)

if(MSVC)
  set_source_files_properties(net_stats.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

install( TARGETS
   graphene_net_stats

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
set_property(TARGET graphene_net_stats PROPERTY CXX_STANDARD 14)
INSTALL( FILES ${HEADERS} DESTINATION "include/graphene/net_stats" )
