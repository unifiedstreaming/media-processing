/*
 * Copyright (C) 2021-2025 CodeShop B.V.
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

#include <cuti/cmdline_reader.hpp>
#include <cuti/option_walker.hpp>
#include <cuti/process_utils.hpp>

#include <exception>
#include <iostream>

// enable assert()
#undef NDEBUG
#include <cassert>

namespace // anonymous
{

using namespace cuti;

#ifndef _WIN32 // POSIX

void typical_umasks()
{
  char const* argv[] = { "command", "--um1=002", "--um2=022" };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  umask_t um1;
  umask_t um2;

  while(!walker.done())
  {
    if(!walker.match("--um1", um1) &&
       !walker.match("--um2", um2))
    {
      break;
    }
  }

  assert(walker.done());
  assert(um1.value() == 02);
  assert(um2.value() == 022);
}
  
void non_octal_umask()
{
  char const* argv[] = { "command", "--um=22" };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  bool caught = false;
  try
  {
    umask_t um;
    walker.match("--um", um);
  }
  catch(std::exception const&)
  {
    caught = true;
  }

  assert(caught);
}
  
void overflow_umask()
{
  char const* argv[] = { "command", "--um=02775" };
  int argc = sizeof argv / sizeof argv[0];
  cmdline_reader_t reader(argc, argv);
  option_walker_t walker(reader);

  bool caught = false;
  try
  {
    umask_t um;
    walker.match("--um", um);
  }
  catch(std::exception const&)
  {
    caught = true;
  }

  assert(caught);
}
  
void current_user_test()
{
  auto user1 = user_t::current();
  assert(!user1.empty());

  auto user2 = user_t::current();
  assert(!user2.empty());

  assert(user1.user_id() == user2.user_id());
}
  
void root_user_test()
{
  auto root_user = user_t::root();
  assert(!root_user.empty());

  auto current_user = user_t::current();
  assert(!current_user.empty());

  if(current_user.user_id() == root_user.user_id())
  {
    return;
  }

  bool caught = false;
  try
  {
    root_user.apply();
  }
  catch(std::exception const&)
  {
    caught = true;
  }
  assert(caught);
}
  
void current_user_lookup()
{
  auto user1 = user_t::current();
  assert(!user1.empty());

  auto user2 = user_t::resolve(user1.name());
  assert(!user2.empty());

  assert(user1.user_id() == user2.user_id());
}

void root_user_lookup()
{
  auto root_user = user_t::resolve("root");
  assert(!root_user.empty());

  assert(root_user.user_id() == 0);
}
  
void failing_user_lookup()
{
  bool caught = false;
  try
  {
    user_t::resolve("unethical-blackhat");
  }
  catch(std::exception const&)
  {
    caught = true;
  }

  assert(caught);
}
  
#endif // POSIX

void run_tests(int, char const* const*)
{
#ifndef _WIN32 // POSIX

  typical_umasks();
  non_octal_umask();
  overflow_umask();

  current_user_test();
  root_user_test();
  
  current_user_lookup();
  root_user_lookup();
  failing_user_lookup();

#endif // POSIX
}

} // anonymous

int main(int argc, char* argv[])
{
  try
  {
    run_tests(argc, argv);
  }
  catch(std::exception const& ex)
  {
    std::cerr << argv[0] << ": exception: " << ex.what() << std::endl;
    throw;
  }

  return 0;
}
