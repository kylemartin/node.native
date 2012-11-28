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
        'net.cc',
        'stream.cc',
        'buffers.cc',
        'detail_handle.cc',
        'detail_node.cc',
        'detail_pipe.cc',
        'detail_stream.cc',
        'detail_tcp.cc',
        'detail_http.cc',
        'http_client.cc',
        'http_incoming.cc',
        'http_outgoing.cc',
        'http_parser.cc',
        'http_server.cc'
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

