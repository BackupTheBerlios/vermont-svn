import os
import sys
import time
import logging

d = os.path.dirname(sys.argv[0])
sys.path.insert(0, d + 'common')
sys.path.insert(0, d + 'controller')
sys.path.insert(0, d + 'webinterface')

import LocalVermontInstance
import VermontConfigurator
import VermontInstanceManager
import VermontLogger


VermontLogger.init("testapp", "tmp/test.log", True)
VermontLogger.logger().info("starting test")
lvi = LocalVermontInstance.LocalVermontInstance("tmp", "tmp.conf", "tmp.log", True)
vim = VermontInstanceManager.VermontInstanceManager("tmp", False)
vim.vermontInstances = (lvi)
vim._checkInterval = 5

vc = VermontConfigurator.VermontConfigurator()
vc.parseConfigs([lvi])
vim.vermontInstances = [lvi]
vim._configurator = vc

vim.startConfigThread()
time.sleep(10)
