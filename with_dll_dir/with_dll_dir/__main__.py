#
# Copyright (C) 2023-2026 CodeShop B.V.
#
# This file is part of the with_dll_dir python utility.
#
# The with_dll_dir python utility is free software: you can
# redistribute it and/or modify it under the terms of version 2.1 of
# the GNU Lesser General Public License as published by the Free
# Software Foundation.
#
# The with_dll_dir python utility is distributed in the hope that it
# will be useful, but WITHOUT ANY WARRANTY; without even the implied
# warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See version 2.1 of the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of version 2.1 of the GNU Lesser
# General Public License along with the with_dll_dir python utility.
# If not, see <http://www.gnu.org/licenses/>.
#
# This Windows-only python script is intended to be used as a frontend
# for some other python script that imports a python extension which
# in turn drags in a native DLL from a different directory (on
# Windows, the python interpreter uses OS magic to ignore %PATH% when
# finding such DLLs).
#
# See https://bugs.python.org/issue36085 or
# https://github.com/python/cpython/issues/80266 for details.

import os
import pkgutil
import runpy
import sys

def print_stderr(*args, **kwargs):
  print(*args, file=sys.stderr, **kwargs)

def usage_error():
  print_stderr("usage: " + sys.executable + " " + sys.argv[0] +
    " [<option>...] <dll directory> <script> [<arg>...]")
  print_stderr("options are:")
  print_stderr("  -v, --verbose     print diagnostic information")
  sys.exit(1)

def main():

  verbose = False

  while len(sys.argv) > 1 and sys.argv[1][0] == '-':
    option = sys.argv[1]
    sys.argv.pop(1)
    if option == "--":
      break
    elif option == "-v" or option == "--verbose":
      verbose = True
    else:
      usage_error()

  if len(sys.argv) <= 1:
    usage_error()
  dll_dir = os.path.realpath(sys.argv[1])
  sys.argv.pop(1)

  if len(sys.argv) <= 1:
    usage_error()
  script = os.path.realpath(sys.argv[1])
  sys.argv.pop(1)

  if os.name != "nt":
    print_stderr(sys.argv[0] + ": unsupported os " + os.name)
    sys.exit(1)
    
  if "add_dll_directory" not in dir(os):
    print_stderr(sys.argv[0] +
      ": os.add_dll_directory not supported by python interpreter " +
      sys.executable)
    sys.exit(1)

  os.add_dll_directory(dll_dir)
  if verbose:
    print_stderr(sys.argv[0] + ": added dll directory " + dll_dir)

  if pkgutil.get_importer(script) is None:

    # Add script directory to sys.path; runpy.run_path() only does
    # this for directories and zipfiles, while the command line
    # interpreter also does this for scripts.
    script_dir = os.path.dirname(script)
    sys.path.insert(0, script_dir)
    if verbose:
      print_stderr(sys.argv[0] + ": added " + script_dir + " to sys.path")
    
  runpy.run_path(script, run_name = "__main__")
  
if __name__ == "__main__":
  main()
