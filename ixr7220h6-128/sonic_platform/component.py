"""
    NOKIA IXR7220 H6-128

    Module contains an implementation of SONiC Platform Base API and
    provides the Components' (e.g., BIOS, CPLD, FPGA, etc.) available in
    the platform
"""

try:
    import os
    import subprocess
    import time
    import glob
    from sonic_platform_base.component_base import ComponentBase
    from sonic_platform.sysfs import read_sysfs_file, write_sysfs_file
except ImportError as e:
    raise ImportError(str(e) + "- required module not found") from e

SYSFS_DIR = ["/sys/class/dmi/id/",
             "/sys/bus/i2c/devices/1-0060/",
             "/sys/bus/i2c/devices/135-0071/",
             "/sys/bus/i2c/devices/149-0074/",
             "/sys/bus/i2c/devices/150-0075/",
             "/sys/bus/i2c/devices/153-0076/",
             "/sys/bus/i2c/devices/154-0076/",
             "/sys/bus/i2c/devices/151-0073/",
             "/sys/bus/i2c/devices/152-0073/",
             "/sys/bus/i2c/devices/145-0032/hwmon/hwmon*/",
             "/sys/bus/i2c/devices/146-0033/hwmon/hwmon*/"]

class Component(ComponentBase):
    """Nokia platform-specific Component class"""

    CHASSIS_COMPONENTS = [
        ["BIOS", "Basic Input/Output System"],
        ["CB_FPGA", "Used for managing CPU board"],
        ["MB_FPGA", "Used for managing BCM chip, PSUs and LEDs"],
        ["MB_PORT_FPGA_0", "Used for managing PORT 33-48, 65-80"],
        ["MB_PORT_FPGA_1", "Used for managing PORT 49-64, 81-96, 129"],
        ["UDB_LPORT_FPGA", "Used for managing PORT 1-16"],
        ["UDB_RPORT_FPGA", "Used for managing PORT 17-32"],
        ["LDB_LPORT_FPGA", "Used for managing PORT 97-112"],
        ["LDB_RPORT_FPGA", "Used for managing PORT 113-128"],
        ["FCM0_CPLD", "Used for managing upper fan drawers"],
        ["FCM1_CPLD", "Used for managing lower fan drawers"] ]
    DEV_NAME = ["", "", "MAIN_FPGA", "MAIN_FPGA", "MAIN_FPGA", "MAIN_FPGA", 
                "MAIN_FPGA", "MAIN_FPGA", "MAIN_FPGA", "FAN0_CPLD", "FAN1_CPLD"]

    BIOS_UPDATE_COMMAND = ('./afulnx_64', '/B', '/P', '/N', '/K')
    FPGA_CHECK_COMMAND = ('./fpga_spi_flash.sh', '-rid')
    FPGA_UPDATE_COMMAND = ('./fpga_spi_flash.sh', '-upd', '-all')
    CPLD_CHECK_COMMAND = ('./cpldupd', '-s')
    CPLD_UPDATE_COMMAND = ('./cpldupd', '-u')

    def __init__(self, component_index):
        self.index = component_index
        self.name = self.CHASSIS_COMPONENTS[self.index][0]
        self.description = self.CHASSIS_COMPONENTS[self.index][1]
        if self.name in ("FCM0_CPLD", "FCM1_CPLD"):
            hwmon_dir = glob.glob(SYSFS_DIR[self.index])
            self.sysfs_dir = hwmon_dir[0]
        else:
            self.sysfs_dir = SYSFS_DIR[self.index]
        self.dev_name = self.DEV_NAME[self.index]

    def get_name(self):
        """
        Retrieves the name of the component

        Returns:
            A string containing the name of the component
        """
        return self.name

    def get_model(self):
        """
        Retrieves the part number of the component
        Returns:
            string: Part number of component
        """
        return 'NA'

    def get_serial(self):
        """
        Retrieves the serial number of the component
        Returns:
            string: Serial number of component
        """
        return 'NA'

    def get_presence(self):
        """
        Retrieves the presence of the component
        Returns:
            bool: True if  present, False if not
        """
        return True

    def get_status(self):
        """
        Retrieves the operational status of the component
        Returns:
            bool: True if component is operating properly, False if not
        """
        return True

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return -1

    def is_replaceable(self):
        """
        Indicate whether component is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False

    def get_description(self):
        """
        Retrieves the description of the component

        Returns:
            A string containing the description of the component
        """
        return self.description

    def get_firmware_version(self):
        """
        Retrieves the firmware version of the component

        Returns:
            A string containing the firmware version of the component
        """
        if self.name == "BIOS":
            return read_sysfs_file(self.sysfs_dir + "bios_version")
        else:
            return read_sysfs_file(self.sysfs_dir + "version")

    def install_firmware(self, image_path):
        """
        Installs firmware to the component

        Args:
            image_path: A string, path to firmware image

        Returns:
            A boolean, True if install was successful, False if not
        """
        image_name = os.path.basename(image_path)
        image_full_path = os.path.join("/tmp", image_name)

        if not os.path.isfile(image_full_path):
            print(f"ERROR: the image {image_name} doesn't exist in /tmp")
            return False

        if self.name == "BIOS":
            if not os.path.isfile('/tmp/afulnx_64'):
                print("ERROR: the BIOS upgrade tool /tmp/afulnx_64 doesn't exist ")
                return False
            os.chmod('/tmp/afulnx_64', 0o755)
            cmd = [self.BIOS_UPDATE_COMMAND[0], image_full_path, *self.BIOS_UPDATE_COMMAND[1:]]
            try:
                subprocess.run(cmd, stderr=subprocess.STDOUT, check=True, cwd="/tmp")
            except subprocess.CalledProcessError as e:
                print(f"ERROR: Failed to upgrade BIOS: rc={e.returncode}")
                return False
            print("\nBIOS update has ended\n")

        elif self.name == "CB_FPGA":
            if not os.path.isfile('/tmp/fpga_spi_flash.sh'):
                print("ERROR: the fpga upgrade tool /tmp/fpga_spi_flash.sh doesn't exist ")
                return False
            if not os.path.isfile('/tmp/fpga_upd2'):
                print("ERROR: the fpga upgrade tool /tmp/fpga_upd2 doesn't exist ")
                return False
            os.chmod('/tmp/fpga_spi_flash.sh', 0o755)
            os.chmod('/tmp/fpga_upd2', 0o755)
            check_cmd = list(self.FPGA_CHECK_COMMAND)
            try:
                subprocess.run(check_cmd, cwd="/tmp")
                result = subprocess.check_output(check_cmd, cwd="/tmp")
                text = result.decode('utf-8')
                print(text)
            except subprocess.CalledProcessError as e:
                print(f"ERROR: Failed to check SYS_FPGA RDID: rc={e.returncode}")
                return False
            last = text.splitlines()
            if last[-1].strip() != "RDID: c2 20 18":
                print("FPGA RDID check failed!")
                return False
            update_cmd = [self.FPGA_UPDATE_COMMAND[0], self.FPGA_UPDATE_COMMAND[1], image_full_path, self.FPGA_UPDATE_COMMAND[2]]
            try:
                subprocess.run(update_cmd, stderr=subprocess.STDOUT, check=True, cwd="/tmp")
            except subprocess.CalledProcessError as e:
                print(f"ERROR: Failed to upgrade SYS_FPGA: rc={e.returncode}")
                return False
            print("\nCB_FPGA firmware update has ended\n")

        else:
            if not os.path.isfile('/tmp/cpldupd'):
                print("ERROR: the cpld upgrade tool /tmp/cpldupd doesn't exist ")
                return False
            os.chmod('/tmp/cpldupd', 0o755)
            check_cmd = [*self.CPLD_CHECK_COMMAND, self.dev_name]
            try:
                subprocess.run(check_cmd, stderr=subprocess.STDOUT, check=True, cwd="/tmp")
            except subprocess.CalledProcessError as e:
                print(f"ERROR: Failed to Scan Jtag chain for {self.name}: rc={e.returncode}")
                return False
            update_cmd = [*self.CPLD_UPDATE_COMMAND, self.dev_name, image_full_path]
            try:
                subprocess.run(update_cmd, stderr=subprocess.STDOUT, check=True, cwd="/tmp")
            except subprocess.CalledProcessError as e:
                print(f"ERROR: Failed to upgrade {self.name}: rc={e.returncode}")
                return False
            print(f"\n{self.name} firmware update has ended\n")

        return True

    def update_firmware(self, image_path):
        """
        Updates firmware of the component

        This API performs firmware update: it assumes firmware installation and loading in a single call.
        In case platform component requires some extra steps (apart from calling Low Level Utility)
        to load the installed firmware (e.g, reboot, power cycle, etc.) - this will be done automatically by API

        Args:
            image_path: A string, path to firmware image

        Returns:
            Boolean False if image_path doesn't exist instead of throwing an exception error
            Nothing when the update is successful

        Raises:
            RuntimeError: update failed
        """
        return self.install_firmware(image_path)

    def get_available_firmware_version(self, image_path):
        """
        Retrieves the available firmware version of the component

        Note: the firmware version will be read from image

        Args:
            image_path: A string, path to firmware image

        Returns:
            A string containing the available firmware version of the component
        """
        if image_path:    
            image_name = os.path.basename(image_path)
            return image_name

        return 'NA'
   
