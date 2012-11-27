{
  'includes': ['../build/common.gypi'],
  'targets': [
    {
      'target_name': 'Tests',
      'type': 'none',
      'dependencies': [
      	'common/common.gyp:*',
        'http/http.gyp:*',
        'net/net.gyp:*',
        'native/native.gyp:*'
      ],
    }
  ]
}

