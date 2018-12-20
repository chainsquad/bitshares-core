file(GLOB HEADERS "include/graphene/custom_authorities/*.hpp")

add_library( graphene_custom_authorities 
             custom_authorities_plugin.cpp
           )

target_link_libraries( graphene_custom_authorities graphene_chain graphene_app )
target_include_directories( graphene_custom_authorities
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if(MSVC)
  set_source_files_properties( custom_authorities_plugin.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

install( TARGETS
   graphene_custom_authorities

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/graphene/custom_authorities" )

