###############################################################################
#                                                                             #
# YencaP software, LORIA-INRIA LORRAINE, MADYNES RESEARCH TEAM                #
# Copyright (C) 2005  Vincent CRIDLIG                                         #
#                                                                             #
# This program is free software; you can redistribute it and/or               #
# modify it under the terms of the GNU General Public License                 #
# as published by the Free Software Foundation; either version 2              #
# of the License, or (at your option) any later version.                      #
#                                                                             #
# This program is distributed in the hope that it will be useful,             #
# but WITHOUT ANY WARRANTY; without even the implied warranty of              #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               #
# GNU General Public License for more details.                                #
#                                                                             #
# You should have received a copy of the GNU General Public License           #
# along with this program; if not, write to the Free Software                 #
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. #
#                                                                             #
# Author Info:                                                                #
#   Name : Vincent CRIDLIG                                                    #
#   Email: Vincent.Cridlig@loria.fr                                           #
#                                                                             #
###############################################################################

from RIP_Command import RIP_Command


class Network_RC(RIP_Command):

	def __init__(self, network="", ifname="", positive = True):
		self.name = "network"
		if network!="":
			self.network = str(network)
		elif ifname!="":
			self.ifname = ifname
		else:
			print "RIP Module: Network_RC: This can should never happen."
		self.positive = positive


	def toCLI(self):
		if self.positive:
			tmp = ""
		else:
			tmp = "no "

		tmp = "%s%s %s" % (tmp, self.name, self.network)
		return tmp


	def toXML(self, document, namespace):
		element = document.createElementNS(namespace, self.name)
		if self.network!="":
			textNode = document.createTextNode(self.network)
		elif self.ifname!="":
			textNode = document.createTextNode(self.ifname)
		element.appendChild(textNode)
		return element


	def equals(self, command):
		if self.name == command.name:
			if self.network == command.network:
				return True
			else:
				return False
		else:
			return False
