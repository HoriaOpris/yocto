# Yocto for the STM32MP157D-DK1

A Yocto (5.0 scarthgap) build for the STM32MP157D-DK1 board, plus the apps developed for it.
Project notes, decisions and next steps are in [claudeYoctoSession1.md](claudeYoctoSession1.md) (loaded by Claude Code through `CLAUDE.md`). A visual recap is in [docs/yocto-dk1-session1.html](docs/yocto-dk1-session1.html).

## What's in this repo

| Path | What it is |
|---|---|
| `poky/`, `meta-openembedded/`, `meta-st-stm32mp/` | Yocto layers, as git submodules pinned to the exact commits that were built |
| `build-dk1/conf/` | Build config: `local.conf` (DK1 settings) and `bblayers.conf` (layer list) |
| `apps/ledblink/` | First app: blinks LED LD8. Build with the SDK, `make run` copies it to the board |
| `setup/host-setup.sh` | One-time Ubuntu VM setup (packages, AppArmor setting, serial and SSH) |
| `setup/ssh_config.dk1` | SSH alias `dk1` for the board |
| `docs/` | Visual recap of session 1 (open the HTML in a browser) |

Not tracked, because the build regenerates them: `build-dk1/tmp`, `downloads`, `sstate-cache`, the SD card image and the SDK.

## Reproduce on a new VM

```bash
# 1. Get the repo, with the layers at the pinned commits
git clone --recurse-submodules https://github.com/HoriaOpris/yocto.git ~/yocto

# 2. Host setup (needs sudo), then log out and back in
sh ~/yocto/setup/host-setup.sh

# 3. Build the image and the SDK (first build: several hours)
cd ~/yocto && source poky/oe-init-build-env build-dk1
bitbake core-image-minimal
bitbake core-image-minimal -c populate_sdk

# 4. Make the SD card image
cd tmp/deploy/images/stm32mp15-disco
./scripts/create_sdcard_from_flashlayout.sh flashlayout_core-image-minimal/opteemin/FlashLayout_sdcard_stm32mp157d-dk1-opteemin.tsv

# 5. Install the SDK
~/yocto/build-dk1/tmp/deploy/sdk/poky-glibc-*-toolchain-5.0.21.sh -y -d ~/sdk/dk1
```

Then flash the card, boot the board, and add the new VM's SSH key to it:
```bash
cat ~/.ssh/id_ed25519.pub | ssh root@<board-ip> 'mkdir -p ~/.ssh && chmod 700 ~/.ssh && cat >> ~/.ssh/authorized_keys && chmod 600 ~/.ssh/authorized_keys'
```

To save hours on the first build, copy `build-dk1/downloads/` (source archives, ~5 GB) from the old VM before step 3. Copying `build-dk1/sstate-cache/` (~8 GB) also helps, but only if the new VM has the same CPU type as the old one (aarch64).
