#!/usr/bin/env python
import glob
import os
import shutil
import subprocess
import sys

from waflib.extras import autowaf as autowaf
import waflib.Logs as Logs, waflib.Options as Options

# Version of this package (even if built as a child)
SRATOM_VERSION       = '0.2.0'
SRATOM_MAJOR_VERSION = '0'

# Library version (UNIX style major, minor, micro)
# major increment <=> incompatible changes
# minor increment <=> compatible changes (additions)
# micro increment <=> no interface changes
# Sratom uses the same version number for both library and package
SRATOM_LIB_VERSION = SRATOM_VERSION

# Variables for 'waf dist'
APPNAME = 'sratom'
VERSION = SRATOM_VERSION

# Mandatory variables
top = '.'
out = 'build'

def options(opt):
    opt.load('compiler_c')
    autowaf.set_options(opt)
    opt.add_option('--test', action='store_true', default=False, dest='build_tests',
                   help="Build unit tests")
    opt.add_option('--static', action='store_true', default=False, dest='static',
                   help="Build static library")

def configure(conf):
    conf.load('compiler_c')
    conf.line_just = 41
    autowaf.configure(conf)
    autowaf.display_header('Sratom Configuration')

    if conf.env['MSVC_COMPILER']:
        conf.env.append_unique('CFLAGS', ['-TP', '-MD'])
    else:
        conf.env.append_unique('CFLAGS', '-std=c99')

    conf.env['BUILD_TESTS']  = Options.options.build_tests
    conf.env['BUILD_STATIC'] = Options.options.static

    # Check for gcov library (for test coverage)
    if conf.env['BUILD_TESTS']:
        conf.check_cc(lib='gcov',
                      define_name='HAVE_GCOV',
                      mandatory=False)

    autowaf.check_pkg(conf, 'lv2', atleast_version='1.0.0', uselib_store='LV2')
    autowaf.check_pkg(conf, 'serd-0', uselib_store='SERD',
                      atleast_version='0.14.0', mandatory=True)
    autowaf.check_pkg(conf, 'sord-0', uselib_store='SORD',
                      atleast_version='0.8.0', mandatory=True)

    autowaf.define(conf, 'SRATOM_VERSION', SRATOM_VERSION)
    conf.write_config_header('sratom_config.h', remove=False)

    autowaf.display_msg(conf, "Unit tests", str(conf.env['BUILD_TESTS']))
    print('')

lib_source = [ 'src/sratom.c' ]

def build(bld):
    # C Headers
    includedir = '${INCLUDEDIR}/sratom-%s/sratom' % SRATOM_MAJOR_VERSION
    bld.install_files(includedir, bld.path.ant_glob('sratom/*.h'))

    # Pkgconfig file
    autowaf.build_pc(bld, 'SRATOM', SRATOM_VERSION, SRATOM_MAJOR_VERSION,
                     ['SERD', 'SORD', 'LV2'],
                     {'SRATOM_MAJOR_VERSION' : SRATOM_MAJOR_VERSION})

    libflags = [ '-fvisibility=hidden' ]
    libs     = [ 'm' ]
    defines  = []
    if bld.env['MSVC_COMPILER']:
        libflags = []
        libs     = []
        defines  = ['snprintf=_snprintf']

    # Shared Library
    obj = bld(features        = 'c cshlib',
              export_includes = ['.'],
              source          = lib_source,
              includes        = ['.', './src'],
              lib             = libs,
              name            = 'libsratom',
              target          = 'sratom-%s' % SRATOM_MAJOR_VERSION,
              vnum            = SRATOM_LIB_VERSION,
              install_path    = '${LIBDIR}',
              defines         = defines,
              cflags          = libflags + [ '-DSRATOM_SHARED',
                                             '-DSRATOM_INTERNAL' ])
    autowaf.use_lib(bld, obj, 'SERD SORD LV2')

    # Static library
    if bld.env['BUILD_STATIC']:
        obj = bld(features        = 'c cstlib',
                  export_includes = ['.'],
                  source          = lib_source,
                  includes        = ['.', './src'],
                  lib             = libs,
                  name            = 'libsratom_static',
                  target          = 'sratom-%s' % SRATOM_MAJOR_VERSION,
                  vnum            = SRATOM_LIB_VERSION,
                  install_path    = '${LIBDIR}',
                  defines         = defines,
                  cflags          = ['-DSRATOM_INTERNAL'])
        autowaf.use_lib(bld, obj, 'SERD SORD LV2')

    if bld.env['BUILD_TESTS']:
        test_libs   = libs
        test_cflags = ['']
        if bld.is_defined('HAVE_GCOV'):
            test_libs   += ['gcov']
            test_cflags += ['-fprofile-arcs', '-ftest-coverage']

        # Static library (for unit test code coverage)
        obj = bld(features     = 'c cstlib',
                  source       = lib_source,
                  includes     = ['.', './src'],
                  lib          = test_libs,
                  name         = 'libsratom_profiled',
                  target       = 'sratom_profiled',
                  install_path = '',
                  defines      = defines,
                  cflags       = test_cflags + ['-DSRATOM_INTERNAL'])
        autowaf.use_lib(bld, obj, 'SERD SORD LV2')

        # Unit test program
        obj = bld(features     = 'c cprogram',
                  source       = 'tests/sratom_test.c',
                  includes     = ['.', './src'],
                  use          = 'libsratom_profiled',
                  lib          = test_libs,
                  target       = 'sratom_test',
                  install_path = '',
                  defines      = defines,
                  cflags       = test_cflags)

    # Documentation
    autowaf.build_dox(bld, 'SRATOM', SRATOM_VERSION, top, out)

    bld.add_post_fun(autowaf.run_ldconfig)
    if bld.env['DOCS']:
        bld.add_post_fun(fix_docs)

def test(ctx):
    autowaf.pre_test(ctx, APPNAME)
    os.environ['PATH'] = '.' + os.pathsep + os.getenv('PATH')
    autowaf.run_tests(ctx, APPNAME, ['sratom_test'], dirs=['./src','./tests'])
    autowaf.post_test(ctx, APPNAME)

def lint(ctx):
    subprocess.call('cpplint.py --filter=+whitespace/comments,-whitespace/tab,-whitespace/braces,-whitespace/labels,-build/header_guard,-readability/casting,-readability/todo,-build/include src/* sratom/*', shell=True)

def build_dir(ctx, subdir):
    if autowaf.is_child():
        return os.path.join('build', APPNAME, subdir)
    else:
        return os.path.join('build', subdir)

def fix_docs(ctx):
    try:
        top = os.getcwd()
        os.chdir(build_dir(ctx, 'doc/html'))
        os.system("sed -i 's/SRATOM_API //' group__sratom.html")
        os.system("sed -i 's/SRATOM_DEPRECATED //' group__sratom.html")
        os.system("sed -i 's/href=\"doc\/style.css\"/href=\"style.css\"/' group__sratom.html")
        os.remove('index.html')
        os.symlink('group__sratom.html', 'index.html')
        os.chdir(top)
        os.chdir(build_dir(ctx, 'doc/man/man3'))
        os.system("sed -i 's/SRATOM_API //' sratom.3")
        os.chdir(top)
    except:
        Logs.error("Failed to fix up %s documentation" % APPNAME)

def upload_docs(ctx):
    os.system("rsync -ravz --delete -e ssh build/doc/html/ drobilla@drobilla.net:~/drobilla.net/docs/sratom/")
