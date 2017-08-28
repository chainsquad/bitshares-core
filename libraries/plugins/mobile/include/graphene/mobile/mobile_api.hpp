
#pragma once

#include <fc/api.hpp>

/*
 * We need this so we can refer to graphene::app::application without including the entire header
 */
namespace graphene { namespace app {
class application;
} }

/*
 * Our basic namespace
 */
namespace graphene { namespace mobile {

namespace detail {
 /* The "detail" is used to namespace the actual implemention and separated it
  * from the interface
  */
 class mobile_api_impl;
}

class mobile_api
{
   public:
      /**
       * The API requires a constructor which takes app.
       */
      mobile_api( graphene::app::application& app );

      /**
       * Called immediately after the constructor. If the API class uses enable_shared_from_this,
       * shared_from_this() is available in this method, which allows signal handlers to be registered
       * with app::connect_signal()
       */
      void on_api_startup();

      // TODO: Add API methods here
      uint32_t mobile();

   private:
      /*
       * The implementation instance is stored privately in `my`
       */
      std::shared_ptr< detail::mobile_api_impl > my;
};

} }

/*
 * Reflection for C++ allowing automatic serialization in Json & Binary of C++
 * Structs 
 */
FC_API( graphene::mobile::mobile_api,
    (mobile)
)
