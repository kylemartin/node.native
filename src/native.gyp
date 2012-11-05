{
  'variables': {
  },

  'targets': [
    {
      'target_name': 'native',
      'type': 'static_library',

      'sources': [
        'native.cc',
        'timers.cc',
        'detail/node.cc',
        'detail/handle.cc',
        'http/client.cc',
        'http/incoming.cc',
        'http/outgoing.cc',
        'http/parser.cc',
        'http/server.cc'
      ],
      
      'dependencies': [
        '../deps/uv/uv.gyp:uv',
        '../deps/http-parser/http_parser.gyp:http_parser',
      ],

      'include_dirs': [
        '../include',
        '../deps/uv/src/ares',
        '<(SHARED_INTERMEDIATE_DIR)',
      ],

      'direct_dependent_settings': {
        'include_dirs': [
          '../include',
          '../deps/uv/src/ares',
        ],
      },

      'export_dependent_settings': [
        '../deps/uv/uv.gyp:uv',
        '../deps/http-parser/http_parser.gyp:http_parser',
      ],
    }
  ] # end targets
}

