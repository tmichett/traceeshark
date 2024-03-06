#!/bin/bash

WS_VERSION_WANTED=$(cat ws_version.txt | grep -o -E "[0-9]+\.[0-9]+\.[0-9]+")
if command -v "wireshark" &> /dev/null; then
    WS_VERSION_EXISTS=$(wireshark --version | grep -o -E "Wireshark [0-9]+\.[0-9]+\.[0-9]+" | grep -o -E "[0-9]+\.[0-9]+\.[0-9]+")
    if [ "$WS_VERSION_WANTED" != "$WS_VERSION_EXISTS" ]; then
        read -p "Plugins were compiled for Wireshark $WS_VERSION_WANTED but you have version $WS_VERSION_EXISTS, install plugins anyway? (y/n): " user_input
        if [[ "$user_input" != "y" && "$user_input" != "Y" ]]; then
            exit 1
        fi
    fi
else
    read -p "Wireshark installation not found, install plugins anyway? (y/n): " user_input
    if [[ "$user_input" != "y" && "$user_input" != "Y" ]]; then
        exit 1
    fi
fi

mkdir -p ~/.config/wireshark/profiles
cp -r profiles/Tracee ~/.config/wireshark/profiles/

OS_NAME=$(uname -s)
if [ "$OS_NAME" == "Linux" ]; then
    mkdir -p ~/.local/lib/wireshark/plugins/epan
    cp tracee-event.so* ~/.local/lib/wireshark/plugins/epan
    cp tracee-network-capture.so* ~/.local/lib/wireshark/plugins/epan
    mkdir -p ~/.local/lib/wireshark/plugins/wiretap
    cp tracee-json.so* ~/.local/lib/wireshark/plugins/wiretap

    mkdir -p ~/.local/lib/wireshark/extcap
    cp extcap/tracee-capture.py ~/.local/lib/wireshark/extcap/
    chmod +x ~/.local/lib/wireshark/extcap/tracee-capture.py
    cp -r extcap/tracee-capture ~/.local/lib/wireshark/extcap/

    mkdir -p ~/.config/wireshark/extcap
    cp extcap/tracee-capture.py ~/.config/wireshark/extcap/
    chmod +x ~/.config/wireshark/extcap/tracee-capture.py
    cp -r extcap/tracee-capture ~/.config/wireshark/extcap/
else
    WS_VERSION_SHORT=$(wireshark --version | grep -o -E "Wireshark [0-9]+\.[0-9]+\.[0-9]+" | grep -o -E "[0-9]+\.[0-9]+")
    WS_VERSION_DIR=${WS_VERSION_SHORT//./-}
    mkdir -p ~/.local/lib/wireshark/plugins/$WS_VERSION_DIR/epan
    cp tracee-event.so* ~/.local/lib/wireshark/plugins/$WS_VERSION_DIR/epan
    cp tracee-network-capture.so* ~/.local/lib/wireshark/plugins/$WS_VERSION_DIR/epan
    mkdir -p ~/.local/lib/wireshark/plugins/$WS_VERSION_DIR/wiretap
    cp tracee-json.so* ~/.local/lib/wireshark/plugins/$WS_VERSION_DIR/wiretap
fi