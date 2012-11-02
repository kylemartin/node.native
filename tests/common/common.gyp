{
  'includes': ['../../build/common.gypi'],
  'targets': [
    {
      'target_name': 'cctest',
      'type': 'static_library',
      'sources': [
        'cctest.cc'
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '.'
        ],
      },
    }
  ]
}

