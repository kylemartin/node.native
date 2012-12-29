{
  'targets': [
    {
      'target_name': 'repl',
      'type': 'executable',
      'sources': [ 'repl.cc' ],
      'dependencies': [ '../src/native.gyp:native' ],
    },
    # {
    #   'target_name': 'tty',
    #   'type': 'executable',
    #   'sources': [ 'tty.cc' ],
    #   'dependencies': [ '../src/native.gyp:native' ],
    # },
    {
      'target_name': 'timers',
      'type': 'executable',
      'sources': ['timers.cc'],
      'dependencies': ['../src/native.gyp:native'],
    },
    {
      'target_name': 'echo',
      'type': 'executable',
      'sources': [ 'echo.cc' ],
      'dependencies': [ '../src/native.gyp:native' ]
    },
    {
      'target_name': 'socket',
      'type': 'executable',
      'sources': ['socket.cc'],
      'dependencies': ['../src/native.gyp:native'],
    },
    {
      'target_name': 'webclient',
      'type': 'executable',
      'sources': [ 'webclient.cc' ],
      'dependencies': [ '../src/native.gyp:native' ]
    },
    {
      'target_name': 'webserver',
      'type': 'executable',
      'sources': [ 'webserver.cc' ],
      'dependencies': [ '../src/native.gyp:native' ]
    },
    {
      'target_name': 'signals',
      'type': 'executable',
      'sources': [ 'signals.cc' ],
      'dependencies': [ '../src/native.gyp:native' ]
    },
    {
      'target_name': 'dns',
      'type': 'executable',
      'sources': [ 'dns.cc' ],
      'dependencies': [ '../src/native.gyp:native' ]
    }
  ] # end targets
}

