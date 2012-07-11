{
  'variables': {
  },

  'targets': [
    {
      'target_name': 'native',
      'type': 'static_library',

      'sources': [
        'native.cpp',
      ],
      
      'dependencies': [
        'deps/uv/uv.gyp:uv',
        'deps/http-parser/http_parser.gyp:http_parser',
      ],

      'include_dirs': [
        'deps/uv/src/ares',
        '<(SHARED_INTERMEDIATE_DIR)',
      ],

      'direct_dependent_settings': {
        'include_dirs': [
          'deps/uv/src/ares',
        ],
      },

      'export_dependent_settings': [
        'deps/uv/uv.gyp:uv',
        'deps/http-parser/http_parser.gyp:http_parser',
      ],
    },
    # {
    #   'target_name': 'webclient',
    #   'type': 'executable',
    #   'sources': [ 'webclient.cpp' ],
    #   'dependencies': [ 'native' ]
    # },
    {
      'target_name': 'webserver',
      'type': 'executable',
      'sources': [ 'webserver.cpp' ],
      'dependencies': [ 'native' ]
    },
    {
      'target_name': 'echo',
      'type': 'executable',
      'sources': [ 'echo.cpp' ],
      'dependencies': [ 'native' ]
    }
  ] # end targets
}

