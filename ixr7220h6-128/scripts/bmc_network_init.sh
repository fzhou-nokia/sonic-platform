#!/bin/bash

CONFIG_FILE="/etc/sonic/bmc.json"

check_jq() {
    if ! command -v jq &> /dev/null; then
        logger -t bmc-network "Error: 'jq' is not installed. Please install it to parse JSON."
        echo "Error: 'jq' is not installed. Please install it to parse JSON." >&2
        exit 1
    fi
}

check_jq

if [ -z "$CONFIG_FILE" ]; then
    logger -t bmc-network "Error: Configuration file not found at $CONFIG_FILE"
    echo "Error: Configuration file not found at $CONFIG_FILE" >&2
    exit 1
fi

BMC_IF_NAME=$(jq -r '.bmc_if_name' "$CONFIG_FILE")
BMC_IF_ADDR=$(jq -r '.bmc_if_addr' "$CONFIG_FILE")
BMC_NET_MASK=$(jq -r '.bmc_net_mask' "$CONFIG_FILE")

if [ -z "$BMC_IF_NAME" ] || [ -z "$BMC_IF_ADDR" ] || [ -z "$BMC_NET_MASK" ]; then
    logger -t bmc-network "Error: Failed to extract all required values from $CONFIG_FILE. Check JSON format."
    echo "Error: Failed to extract all required values from $CONFIG_FILE. Check JSON format." >&2
    exit 1
fi

echo "  bmc_if_name: $BMC_IF_NAME" >&2
echo "  bmc_if_addr: $BMC_IF_ADDR" >&2
echo "  bmc_net_mask: $BMC_NET_MASK" >&2

if [ "$BMC_IF_NAME" != "usb0" ]; then
    echo "Renaming interface 'usb0' to '$BMC_IF_NAME'..." >&2
    if ip link show usb0 &> /dev/null; then
        ip link set usb0 name "$BMC_IF_NAME"
        if [ $? -eq 0 ]; then
            echo "Interface 'usb0' successfully renamed to '$BMC_IF_NAME'." >&2
        else
            logger -t bmc-network "Error: Failed to rename 'usb0' to '$BMC_IF_NAME'."
            echo "Error: Failed to rename 'usb0' to '$BMC_IF_NAME'." >&2
            exit 1
        fi
    elif ip link show "$BMC_IF_NAME" &> /dev/null; then
        echo "Interface '$BMC_IF_NAME' already exists. Skipping rename." >&2
    else
        logger -t bmc-network "Error: Neither 'usb0' nor '$BMC_IF_NAME' found. Cannot configure BMC network."
        echo "Error: Neither 'usb0' nor '$BMC_IF_NAME' found. Cannot configure BMC network." >&2
        exit 1
    fi
else
    echo "Interface name is already 'usb0'. No rename needed." >&2
fi

echo "Configuring interface '$BMC_IF_NAME' with IP '$BMC_IF_ADDR' and netmask '$BMC_NET_MASK'..." >&2
ifconfig "$BMC_IF_NAME" "$BMC_IF_ADDR" netmask "$BMC_NET_MASK" up

if [ $? -eq 0 ]; then
    echo "Interface '$BMC_IF_NAME' successfully configured." >&2
    echo "Current configuration for '$BMC_IF_NAME':" >&2
    ip addr show "$BMC_IF_NAME" >&2
else
    logger -t bmc-network "Error: Failed to configure interface '$BMC_IF_NAME'."
    echo "Error: Failed to configure interface '$BMC_IF_NAME'." >&2
    exit 1
fi

echo "BMC service finished." >&2
