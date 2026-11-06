#!/bin/sh
# One-time setup of a fresh Ubuntu VM for this Yocto project. Needs sudo.
#   sh ~/yocto/setup/host-setup.sh
# Log out and back in afterwards (for the dialout group).
set -e

# Yocto host packages (from the scarthgap docs) + serial console tool
sudo apt update
sudo apt install -y build-essential chrpath cpio debianutils diffstat file gawk gcc git \
  iputils-ping libacl1 lz4 locales python3 python3-git python3-jinja2 python3-pexpect python3-pip \
  python3-subunit socat texinfo unzip wget xz-utils zstd picocom

# Ubuntu 24.04+ blocks unprivileged user namespaces, which bitbake uses to cut tasks off the network
echo 'kernel.apparmor_restrict_unprivileged_userns = 0' | sudo tee /etc/sysctl.d/60-yocto-userns.conf
sudo sysctl --system > /dev/null

# Open /dev/ttyACM0 (ST-LINK serial console) without sudo
sudo usermod -aG dialout "$USER"

# SSH alias "dk1" for the board (edit HostName if the board's IP changes)
mkdir -p ~/.ssh && chmod 700 ~/.ssh
grep -q '^Host dk1$' ~/.ssh/config 2>/dev/null || cat "$(dirname "$0")/ssh_config.dk1" >> ~/.ssh/config
chmod 600 ~/.ssh/config
[ -f ~/.ssh/id_ed25519 ] || ssh-keygen -t ed25519 -N '' -f ~/.ssh/id_ed25519

echo "Done. Log out and back in so the dialout group applies."
