/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <graphene/custom_authorities/custom_authorities_plugin.hpp>

namespace graphene { namespace custom_authorities {

namespace detail
{

class custom_authorities_plugin_impl
{
   public:
      custom_authorities_plugin_impl(custom_authorities_plugin& _plugin)
         : _self( _plugin )
      {}

      graphene::chain::database& database()
      {
         return _self.database();
      }

private:
      custom_authorities_plugin& _self;
};

} // end namespace detail

custom_authorities_plugin::custom_authorities_plugin() :
   my( new detail::custom_authorities_plugin_impl(*this) )
{}

custom_authorities_plugin::~custom_authorities_plugin()
{}

std::string custom_authorities_plugin::plugin_name()const
{
   return "custom_authorities_plugin";
}
   
std::string custom_authorities_plugin::plugin_description()const
{
   return "custom_authorities_plugin description";
}

void custom_authorities_plugin::plugin_set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg
   )
{}

void custom_authorities_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
}

void custom_authorities_plugin::plugin_startup()
{
   ilog("custom_authorities_plugin: plugin_startup() begin");
}

} }
