{
  'variables': {
  },

  'targets': [
    {
      'target_name': 'native',
      'type': 'static_library',

      'sources': [
        'src/native.cpp',
      ],
      
      'dependencies': [
        'deps/uv/uv.gyp:uv',
        'deps/http-parser/http_parser.gyp:http_parser',
      ],

      'include_dirs': [
        'include',
        'deps/uv/src/ares',
        '<(SHARED_INTERMEDIATE_DIR)',
      ],

      'direct_dependent_settings': {
        'include_dirs': [
          'include',
          'deps/uv/src/ares',
        ],
      },

      'export_dependent_settings': [
        'deps/uv/uv.gyp:uv',
        'deps/http-parser/http_parser.gyp:http_parser',
      ],
    },
    {
      'target_name': 'timers',
      'type': 'executable',
      'sources': ['examples/timers.cc'],
      'dependencies': ['native'],
    },
    {
      'target_name': 'webserver',
      'type': 'executable',
      'sources': [ 'examples/webserver.cpp' ],
      'dependencies': [ 'native' ]
    },
    {
      'target_name': 'echo',
      'type': 'executable',
      'sources': [ 'examples/echo.cpp' ],
      'dependencies': [ 'native' ]
    },
    {
      'target_name': 'socket',
      'type': 'executable',
      'sources': ['examples/socket.cc'],
      'dependencies': ['native'],
    },
  ] # end targets
}
