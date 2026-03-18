project('libgbm-msm', 'c',
        version : '1.0.0',
        default_options : [
          'buildtype=debugoptimized',
          'c_std=gnu99',
          'warning_level=1',
        ],
        meson_version : '>= 0.50'
)

cc = meson.get_compiler('c')
add_global_arguments(['-Wno-unused-variable'], language: 'c')
add_global_arguments(['-Wno-discarded-qualifiers'], language: 'c')

dep_libdrm = dependency('libdrm', version: '>= 2.4.75')
ext_includes = include_directories('external')
gbm = dependency('gbm', version : ['>=21.2'])
dep_libxml2 = dependency('libxml-2.0')

arch = run_command('dpkg-architecture', '-qDEB_HOST_MULTIARCH').stdout().strip()
libdir = '/usr/lib' + (arch != '' ? '/' + arch : '')

c_args = ['-DLIBDIR="' + libdir + '"']
subdir('src')
