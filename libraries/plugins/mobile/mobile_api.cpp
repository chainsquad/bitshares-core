#include <graphene/app/application.hpp>
#include <graphene/chain/database.hpp>

#include <graphene/mobile/mobile_api.hpp>
#include <graphene/mobile/mobile_plugin.hpp>

namespace graphene { namespace mobile {

/*
 * The detail is used to separated the implementation from the interface
 */
namespace detail {

class mobile_api_impl
{
   public:
      mobile_api_impl( graphene::app::application& _app );
      graphene::app::application& app;
      std::shared_ptr< graphene::mobile_plugin::mobile_plugin > get_plugin();

      // TODO:  Add API methods here
      uint32_t mobile();
};

/**
 * Constructor Implementation
 */
mobile_api_impl::mobile_api_impl( graphene::app::application& _app ) : app( _app )
{}

/**
 * Get plugin name Implementation
 */
std::shared_ptr< graphene::mobile_plugin::mobile_plugin > mobile_api_impl::get_plugin()
{
   /// We are loading the mobile_plugin's get_plugin all here
   return app.get_plugin< graphene::mobile_plugin::mobile_plugin >( "mobile" );
}

/**
 * Custom call 'mobile' implementation
 */
uint32_t mobile_api_impl::mobile()
{
   // Obtain access to the internal database objects
   std::shared_ptr< graphene::chain::database > db = app.chain_database();
   // Return the head block number
   return db->head_block_num();
}

} /// detail

/*
 * Now we are done with implementation/detail, let's do the interface (linking
 * implementation)
 */

/*
 * Plugin constructor implementation
 */
mobile_api::mobile_api( graphene::app::application& app )
{
   my = std::make_shared< detail::mobile_api_impl >(app);
}

/*
 * Custom call 'mobile' interface
 */
uint32_t mobile_api::mobile()
{
   return my->mobile();
}

} } // graphene::mobile
