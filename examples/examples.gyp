{
  'targets': [
    # {
    #   'target_name': 'shell',
    #   'type': 'executable',
    #   'sources': [ 'shell.cc' ],
    #   'dependencies': [ '../src/native.gyp:native' ],
    # },
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
    }
  ] # end targets
}
