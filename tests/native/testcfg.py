import sys, os
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))

from common.cctest import CCTestConfiguration

def GetConfiguration(context, root):
  return CCTestConfiguration(context, root, 'test_native')