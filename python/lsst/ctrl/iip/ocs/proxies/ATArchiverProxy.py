# This file is part of ctrl_iip
#
# Developed for the LSST Data Management System.
# This product includes software developed by the LSST Project
# (https://www.lsst.org).
# See the COPYRIGHT file at the top-level directory of this distribution
# for details of code ownership.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

import logging
from lsst.ctrl.iip.ocs.proxies.DeviceProxy import DeviceProxy
from lsst.ctrl.iip.ocs.messages.ATArchiverMessages import ATArchiverMessages

LOGGER = logging.getLogger(__name__)


class ATArchiverProxy(DeviceProxy):
    """Represents the ATArchiver CSC device
    """
    def __init__(self, local_ack, debugLevel):
        super().__init__("ATArchiver", "AT", ATArchiverMessages(), local_ack, debugLevel)

    def process_telemetry(self, msg):
        event_object = self.create_logevent_object("processingStatus")
        build_data = getattr(self.messages, "build_processingStatus_object")
        data = build_data(event_object, msg)
        log_event = self.get_log_event_method("processingStatus")
        log_event(data, 0)
        LOGGER.info(f"Published telemetry event to OCS")
