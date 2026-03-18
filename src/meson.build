if not cc.has_function('dlsym')
    libdl = cc.find_library('dl')
else
    libdl = []
endif

add_project_arguments('-Wall', language : 'c')
add_project_arguments('-Werror', language : 'c')
add_project_arguments('-fvisibility=hidden', language : 'c')
add_project_arguments('-D_GNU_SOURCE', language : 'c')
c_args = meson.get_compiler('c').get_supported_arguments(c_args)

src = [
    'buffer_alloc.c',
    'buffer_alloc.h',
    'buffer_layout.c',
    'buffer_layout.h',
    'gbm_msm.c',
    'gbm_msm.h',
    'gbm_msm_int.h',
    'schema_parser.c',
    'schema_parser.h',
]

msm_gbm = library('msm_gbm',
    src,
    dependencies : [
        gbm,
        dep_libdrm,
        dep_libxml2,
        libdl,
    ],
    include_directories : ext_includes,
    version : meson.project_version(),
    install : true,
    name_prefix: '',
    c_args: c_args,
)
