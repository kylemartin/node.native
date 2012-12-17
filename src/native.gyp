{
  'variables': {
  },

  'targets': [
    {
      'target_name': 'native',
      'type': 'static_library',

      'sources': [
        'detail_handle.cc',
        'detail_signal.cc',
        'detail_pipe.cc',
        'detail_stream.cc',
        'detail_tcp.cc',
        'detail_http.cc',
        'detail_tty.cc',
        'native.cc',
        'process.cc',
        'timers.cc',
        'buffers.cc',
        'stream.cc',
        'tty.cc',
        'readline.cc',
        'shell.cc',
        'net.cc',
        'http_client.cc',
        'http_incoming.cc',
        'http_outgoing.cc',
        'http_parser.cc',
        'http_server.cc'
      ],
      
      'dependencies': [
        '../deps/uv/uv.gyp:libuv',
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
      	'libraries': [ '-lboost_regex-mt' ],
      },

      'export_dependent_settings': [
        '../deps/uv/uv.gyp:libuv',
        '../deps/http-parser/http_parser.gyp:http_parser',
      ],
    }
  ] # end targets
}

