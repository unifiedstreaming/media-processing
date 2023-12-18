#
# Copyright (C) 2023 CodeShop B.V.
#
# This file is part of the with_dll_dir python utility.
#
# The with_dll_dir python utility is free software: you can redistribute it
# and/or modify it under the terms of version 2.1 of the GNU Lesser
# General Public License as published by the Free Software
# Foundation.
#
# The with_dll_dir python utility is distributed in the hope that it
# will be useful, but WITHOUT ANY WARRANTY; without even the implied
# warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See version 2.1 of the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of version 2.1 of the GNU Lesser
# General Public License along with the cuti library.  If not, see
# <http://www.gnu.org/licenses/>.
#
# This python script is intended to be used as a frontend to some
# other python script that relies on native DLLs in some other
# directory than a depending python extension (on Windows, the python
# interpreter uses OS magic is to ignore %PATH% when finding these
# DLLs).

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

  if os.name == "nt":

    if "add_dll_directory" in dir(os):

      # PATH is ignored when loading *native* DLLs as dependencies from
      # python extension modules.  See https://bugs.python.org/issue36085
      # or https://github.com/python/cpython/issues/80266 for details.
      # Use os.add_dll_directory() instead.
      os.add_dll_directory(dll_dir)
      if verbose:
        print_stderr(sys.argv[0] + ": added dll directory " + dll_dir)

    else:

      suffix = ""
      if "PATH" in os.environ:
        suffix = ";" + os.environ["PATH"]
      os.environ["PATH"] = dll_dir + suffix
      if verbose:
        print_stderr(sys.argv[0] + ": added " + dll_dir + " to PATH")

  elif os.name == "posix":

    suffix = ""
    if "LD_LIBRARY_PATH" in os.environ:
      suffix = ":" + os.environ["LD_LIBRARY_PATH"]
    os.environ["LD_LIBRARY_PATH"] = dll_dir + suffix
    if verbose:
      print_stderr(sys.argv[0] + ": added " + dll_dir + " to LD_LIBRARY_PATH")

  else:

    print_stderr(sys.argv[0] + ": unsupported os name " + os.name)
    sys.exit(1)

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
