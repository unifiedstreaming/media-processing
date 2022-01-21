/*
 * Copyright (C) 2021-2022 CodeShop B.V.
 *
 * This file is part of the cuti library.
 *
 * The cuti library is free software: you can redistribute it and/or
 * modify it under the terms of version 2.1 of the GNU Lesser General
 * Public License as published by the Free Software Foundation.
 *
 * The cuti library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See version
 * 2.1 of the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of version 2.1 of the GNU Lesser
 * General Public License along with the cuti library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <cuti/bound_inbuf.hpp>
#include <cuti/bound_outbuf.hpp>
#include <cuti/default_scheduler.hpp>
#include <cuti/final_result.hpp>
#include <cuti/logging_context.hpp>
#include <cuti/method_map.hpp>
#include <cuti/method_runner.hpp>
#include <cuti/nb_string_inbuf.hpp>
#include <cuti/nb_string_outbuf.hpp>

#include <stdexcept>

#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

/*
 * The 'succeed' method implementation
 */
struct succeed_t
{
  using result_value_t = void;

  succeed_t(result_t<void>& result,
            logging_context_t const& /* ignored */,
            bound_inbuf_t& /* ignored */,
            bound_outbuf_t& /* ignored */)
  : result_(result)
  { }

  void start()
  {
    result_.submit();
  }

private :
  result_t<void>& result_;
};

/*
 * The 'fail' method implementation
 */
struct fail_t
{
  using result_value_t = void;

  fail_t(result_t<void>& result,
         logging_context_t const& /* ignored */,
         bound_inbuf_t& /* ignored */,
         bound_outbuf_t& /* ignored */)
  : result_(result)
  { }

  void start()
  {
    result_.fail(std::runtime_error("method failed"));
  }

private :
  result_t<void>& result_;
};

/*
 * A configurable method implementation
 */
struct configurable_t
{
  using result_value_t = void;

  configurable_t(result_t<void>& result,
                 logging_context_t const& /* ignored */,
                 bound_inbuf_t& /* ignored */,
                 bound_outbuf_t& /* ignored */,
                 bool fail)
  : result_(result)
  , fail_(fail)
  { }

  void start()
  {
    if(fail_)
    {
      result_.fail(std::runtime_error("configured to fail"));
      return;
    }

    result_.submit();
  }

private :
  result_t<void>& result_;
  bool fail_;
};

auto configurable_method_factory(bool fail)
{
  return [fail = fail](
    result_t<void>& result,
    logging_context_t const& context,
    bound_inbuf_t& inbuf,
    bound_outbuf_t& outbuf)
  {
    return make_method<configurable_t>(
      result, context, inbuf, outbuf, fail);
  };
}
    
void test_method(
  method_map_t const& method_map,
  std::string const& method_name,
  std::string const& expected_what = "")
{
  /*
   * Set up required logging context, bound_inbuf, bound_outbuf (not used)
   */
  logger_t logger("program");
  logging_context_t logging_context(logger, loglevel_t::info);

  auto nb_inbuf = make_nb_string_inbuf("");

  std::string output;
  auto nb_outbuf = make_nb_string_outbuf(output);

  default_scheduler_t scheduler;
  stack_marker_t base_marker;
  
  bound_inbuf_t bound_inbuf(base_marker, *nb_inbuf, scheduler);
  bound_outbuf_t bound_outbuf(base_marker, *nb_outbuf, scheduler);

  /*
   * Set up method runner
   */
  final_result_t<void> final_result;
  method_runner_t runner(final_result,
    logging_context, bound_inbuf, bound_outbuf, method_map);

  // All sample methods used here submit immediately
  runner.start(method_name);
  assert(final_result.available());

  if(expected_what == "")
  {
    final_result.value();
  }
  else
  {
    bool caught = false;
    try
    {
      final_result.value();
    }
    catch(std::exception const& ex)
    {
      caught = true;
      assert(ex.what() == expected_what);
    }
    assert(caught);
  }
}

void test_methods()
{
  method_map_t map;

  // Default method factories
  map.add_method_factory("succeed", default_method_factory<succeed_t>());
  map.add_method_factory("fail", default_method_factory<fail_t>());

  // Configurable method factories
  map.add_method_factory("configured_to_succeed",
    configurable_method_factory(false));
  map.add_method_factory("configured_to_fail",
    configurable_method_factory(true));
    
  test_method(map, "succeed");
  test_method(map, "unknown", "method \'unknown\' not found");
  test_method(map, "fail", "method failed");
  test_method(map, "configured_to_succeed");
  test_method(map, "configured_to_fail", "configured to fail");
}  

} // anonymous

int main()
{
  test_methods();

  return 0;
}
