# Yocto on the STM32MP157D-DK1: Session 1 handoff

Everything a new Claude Code session needs to continue this project. Session 1 ran from 2026-10-16 to 2026-11-06.

- Repo: **https://github.com/HoriaOpris/yocto** (branch `main`)
- Visual recap for the user: [docs/yocto-dk1-session1.html](docs/yocto-dk1-session1.html) (open in a browser). It's also published at https://claude.ai/artifact/SeZedgRDcZsQJCeUvkp5JB.
- Human setup guide: [README.md](README.md)
- `CLAUDE.md` contains only `@claudeYoctoSession1.md`, so Claude Code loads this file automatically when started in the repo folder.

---

## Start here (next session)

At the end of Session 1 the user was **moving to a new PC and reinstalling the VM**. The next session is probably on a fresh VM where nothing is built or installed yet. First work out which situation you're in:

```bash
ls ~/sdk/dk1 2>/dev/null; ls build-dk1/tmp/deploy/images 2>/dev/null; uname -m; lsb_release -ds; nproc; free -h; df -h ~
git -C <repo> submodule status        # layers present at the pinned commits?
```

**Fresh VM. Bring it back up in this order.** Tell the user plainly which steps are theirs.
1. *(User)* Clone: `git clone --recurse-submodules https://github.com/HoriaOpris/yocto.git <dir>`. If it was cloned without submodules: `git submodule update --init`.
2. *(User, needs sudo)* `sh setup/host-setup.sh`, then log out and back in. It installs the packages, sets the AppArmor sysctl, adds the user to `dialout`, adds the `dk1` SSH alias and creates an SSH key.
3. *(Optional, user)* Copy `build-dk1/downloads/` (about 5 GB) from the old VM to skip re-downloading. Copy `sstate-cache/` (about 8 GB) only if the new VM is also aarch64.
4. *(Claude)* `source poky/oe-init-build-env build-dk1 && bitbake core-image-minimal`. Without the cache this takes hours; run it in the background (see Working conventions).
5. *(Claude)* `bitbake core-image-minimal -c populate_sdk`, then install the SDK with `tmp/deploy/sdk/poky-glibc-*-toolchain-5.0.21.sh -y -d ~/sdk/dk1` (no sudo needed).
6. *(Claude)* Create the SD card `.raw` image (see "SD card"). *(User)* Write it to the card and boot the board.
7. *(User)* Add the new VM's SSH key to the board (one-liner under "The board"), and check its IP; update `HostName` in `~/.ssh/config` if it changed.
8. Check the loop works: `source ~/sdk/dk1/environment-setup-*; cd apps/ledblink; make run ARGS="200 5"`.
9. Continue with **Next steps** below.

**If the new PC is x86_64, not aarch64:**
- Host-specific names change: the SDK installer is `poky-glibc-x86_64-...`, and `tmp/work/x86_64-nativesdk-...`.
- The `TOOLCHAIN_HOST_TASK:remove` line in `local.conf` was an aarch64 workaround. It's harmless on x86_64 and can be removed if the user wants ST's bare-metal toolchain in the SDK.
- `sstate-cache` copied from the old aarch64 VM won't be reused.

**Old VM, still running:** everything already exists in `~/yocto`, which is the working build tree with outputs. `~/Documents/yocto` is the clone of the GitHub repo. `~/sdk/dk1` is installed, and `~/dk1-apps` is a symlink to `~/yocto/apps`.

**Not done at the end of Session 1 (the user's step):** committing and pushing the repo. Git had no user identity configured, `gh` wasn't installed, and the remote is HTTPS, so pushing needs a GitHub personal access token or switching to SSH. Check `git log` and `git status` before assuming it's on GitHub.

---

## Working with the user
- Horia is **learning Yocto from scratch**. Explain concepts along with commands (layer, recipe, machine, image, SDK, sysroot, sstate). They like visual explanations; they asked for the recap as HTML.
- **Be explicit about whose step it is.** Claude can't type the `sudo` password, so it can't run apt, sysctl, `dd` to the card or usermod. The user also does everything physical: SD card, cables, boot switches, USB passthrough. They once assumed Claude had flashed the card when it hadn't.
- The user works in VS Code on an Ubuntu VM. Use clickable markdown links for files.
- The user has approved Claude using SSH on the board, with key login through the `dk1` alias. Claude Code's safety check blocked an earlier attempt to answer the board's empty-password prompt with an `SSH_ASKPASS` script; don't do that.

## Working conventions (for Claude)
- Run long builds with Bash `run_in_background`, logging to `build-dk1/<name>.log`. Use Monitor with an until-loop to catch the startup sanity checks (`Executing Tasks`, `ERROR`). Bitbake stops at the first error, and the background task's exit notifies you.
- After a crash or an interrupted build, half-written objects cause odd link errors (e.g. undefined `lang_hooks`). Fix with `bitbake -c clean <recipe>`, then rebuild.
- `du` on `build-dk1/tmp` takes more than 2 minutes. Avoid it, or run it in the background.
- Check the image's package list with `tmp/deploy/images/stm32mp15-disco/core-image-minimal-stm32mp15-disco.rootfs.manifest`.
- When the setup changes, update this file. At the end of a session, consider writing `claudeYoctoSession2.md` and pointing `CLAUDE.md` at it.

---

## Old host (Session 1 VM), for reference
- Ubuntu 26.04.1 **aarch64** on an Apple Silicon Mac (UTM or Parallels). Python 3.14. 6 vCPUs, 11 GB RAM + 4 GB swap, 200 GB disk (90 GB used).
- Network: NAT `192.168.64.3/24`. The VM reaches the home LAN (the board), but probably not the other way round, so NFS root or TFTP would need port forwarding.
- Ubuntu 26.04 isn't validated for scarthgap. Expect the warning "Host distribution has not been validated"; it was harmless. The fallback plan, if host problems appear, is an Ubuntu 24.04 container.
- AppArmor: `kernel.apparmor_restrict_unprivileged_userns = 0` (in `/etc/sysctl.d/60-yocto-userns.conf`). Bitbake needs user namespaces.
- `.trackerignore` in the repo root stops GNOME's localsearch indexer from scanning the build tree; it was eating CPU.
- The VM crashed once during the SDK build. There was no OOM or panic in the guest log, so the cause was probably the host or hypervisor. If it happens again, check the Mac's memory pressure.

## Repo layout
```
<repo>/
  CLAUDE.md                 @import of this file
  claudeYoctoSession1.md    this file
  README.md                 human setup guide
  docs/yocto-dk1-session1.html   visual recap
  poky/                     submodule, scarthgap @ e306fa0fec (Yocto 5.0.21, bitbake 2.8.1)
  meta-openembedded/        submodule, scarthgap @ 0f00f8b9a2 (meta-oe and meta-python used)
  meta-st-stm32mp/          submodule, scarthgap @ 99e728c (github.com/STMicroelectronics/meta-st-stm32mp)
  build-dk1/conf/           local.conf, bblayers.conf (tracked); everything else in build-dk1 is ignored
  apps/ledblink/            ledblink.c, Makefile
  setup/host-setup.sh       one-time VM setup (sudo)
  setup/ssh_config.dk1      "Host dk1" SSH alias
```
- `bblayers.conf` uses `${TOPDIR}/../<layer>` paths, so the repo works from any folder.
- None of the layers is modified. All customisation is in `build-dk1/conf/local.conf`; the user's own layer comes next.

## Build configuration
Additions at the end of `build-dk1/conf/local.conf`. The stock `EXTRA_IMAGE_FEATURES ?= "debug-tweaks"` (near line 149) is kept, so root logs in with an empty password.
```
MACHINE = "stm32mp15-disco"
# stm32mp15-disco is ST's DK2-only example; build the DK1 device tree instead
STM32MP_DT_FILES_SDCARD:stm32mp15-disco = "stm32mp157d-dk1"
MACHINE_FEATURES:remove = "wifi bluetooth"        # DK1 has no Wi-Fi/BT chip
BB_NUMBER_THREADS = "4"
PARALLEL_MAKE = "-j 4"
INHERIT += "rm_work"
EXTRA_IMAGE_FEATURES += "ssh-server-dropbear tools-debug"
# ST's prebuilt aarch64-host bare-metal toolchain needs libncurses.so.5 + libpython3.8 -> breaks populate_sdk
TOOLCHAIN_HOST_TASK:remove = "nativesdk-gcc-arm-none-eabi nativesdk-gcc-arm-none-eabi-staticdev"
```
- The `stm32mp15-disco` machine targets the DK2 only, building just `stm32mp157f-dk2`. The generic `stm32mp1` machine includes the DK1 but builds for every ST board, which is slow.
- Raise the 4 jobs / `-j 4` limits if the new VM has more RAM: about 2 GB per job.
- Build stats from Session 1: the first `core-image-minimal` ran 4,635 tasks in about 4 h 50 min. Adding the SSH/debug tools reused 4,529 of 4,793 tasks.

## SD card
```bash
cd build-dk1/tmp/deploy/images/stm32mp15-disco
rm -f FlashLayout_sdcard_stm32mp157d-dk1-opteemin.raw     # avoid flashing a stale image
./scripts/create_sdcard_from_flashlayout.sh flashlayout_core-image-minimal/opteemin/FlashLayout_sdcard_stm32mp157d-dk1-opteemin.tsv
```
- The result is `FlashLayout_sdcard_stm32mp157d-dk1-opteemin.raw`, about 4.8 GB, so the card must be 8 GB or more. The user's card is 32 GB.
- **Writing it is the user's step.**
  1. Pass the card reader through to the VM.
  2. Identify the card with `lsblk -d -o NAME,SIZE,TRAN,RM,MODEL`. On the old VM it was `sdb`: 31.3 GB, usb, RM=1. `sda` (0 B) was an empty slot in the reader.
  3. `sudo umount /dev/sdX?*`
  4. `sudo dd if=<raw> of=/dev/sdX bs=8M conv=fdatasync status=progress`
- USB alternative, not yet used: STM32CubeProgrammer through the USB-C port **CN7**, with SW1 both OFF. Run `STM32_Programmer_CLI -c port=usb1 -w flashlayout_core-image-minimal/opteemin/FlashLayout_sdcard_stm32mp157d-dk1-opteemin.tsv` from the images dir. The board shows up as `0483:df11` and reconnects twice during flashing, so set up a USB auto-connect filter in the VM. The `.tsv` already has the USB-loader entries `0x01` and `0x03`.
- Card partitions: fsbl1/2 (TF-A), metadata1/2, fip-a/b (OP-TEE + U-Boot), u-boot-env, bootfs (kernel + device tree), vendorfs, rootfs, userfs.

## The board
- **Booting:** SW1 both **ON** boots from the SD card. Power through USB-C **CN6**.
- **Boot chain:** ROM → TF-A → OP-TEE → U-Boot → Linux 6.6.129.
- **Image contents:** `core-image-minimal` plus dropbear 2022.83 (SSH), gdbserver and strace. There's no compiler on the board.
- **Serial console:** ST-LINK micro-USB **CN11** appears as `/dev/ttyACM0` at 115200 baud. It must be passed through to the VM, and usually isn't after a VM restart. Use `picocom -b 115200 /dev/ttyACM0` (exit with Ctrl+A Ctrl+X). Log in as `root`, no password.
- **Network:** Ethernet to the home router, DHCP. The address was **192.168.0.19**. If it changes, check the router or run `ip addr` on the console.
- **SSH:**
  - The alias `dk1` in `~/.ssh/config` means root@192.168.0.19, with `WarnWeakCrypto no` to hide OpenSSH's post-quantum warning against old dropbear.
  - The key is `~/.ssh/id_ed25519`. **A new VM or a reflash means re-adding the key:**
    `cat ~/.ssh/id_ed25519.pub | ssh root@<ip> 'mkdir -p ~/.ssh && chmod 700 ~/.ssh && cat >> ~/.ssh/authorized_keys && chmod 600 ~/.ssh/authorized_keys'`
  - Don't use `ssh-copy-id`: it hangs, because dropbear accepts blank-password ("none") auth and the probe sits in an open shell.
- **LED:** the device tree defines only the blue **LD8** (PD11), at `/sys/class/leds/heartbeat`. Default trigger: `heartbeat`. Manual control: `echo none > trigger; echo 1 > brightness`.
- **Clock:** the board clock isn't set (March timestamps). There's no NTP yet.

## SDK and the app loop
- The SDK goes in `~/sdk/dk1`: gcc 13.5.0, prefix `arm-poky-linux-gnueabi-`. In each terminal: `source ~/sdk/dk1/environment-setup-cortexa7t2hf-neon-vfpv4-poky-linux-gnueabi`. This sets `CC`, `CFLAGS` and `LDFLAGS` with `--sysroot`.
- Rebuild and reinstall the SDK whenever the image gains libraries that apps link against.
- `apps/ledblink/ledblink.c`: `ledblink [period_ms=500] [count=10, 0=forever] [led=heartbeat]`. It saves the LED trigger, sets it to `none`, toggles `brightness`, and restores the trigger on exit or SIGINT/SIGTERM. Tested on the board; it works.
- `apps/ledblink/Makefile` targets:
  - `make`: cross-compile.
  - `make deploy`: `scp` to `dk1:`.
  - `make run ARGS="200 10"`: deploy, then run over ssh.
  - `make install DESTDIR=...`: installs to `/usr/bin`, for the future recipe.
  - `make clean`.

  `BOARD ?= dk1`. The comment sits on its own line, because a trailing comment would add spaces to the value.
- What the user learned: `make` compiles on the VM, `scp` copies only the binary, and `ssh` runs it on the board. This is not flashing.

## Session 1 log
| Date | What happened |
|---|---|
| 16 Oct | Checked the VM (it needed more RAM and tools; the user did that). Cloned the scarthgap layers and configured the DK1 build. First build succeeded and the `.raw` card image was made. |
| 20 Oct | User flashed the card, booted, used the serial console and toggled LD8 by hand. Image rebuilt with dropbear and the debug tools. |
| 6 Nov | SDK built, after the VM crash (cleaned nativesdk-gcc) and the ST toolchain problem (`TOOLCHAIN_HOST_TASK:remove`); installed to `~/sdk/dk1`. User reflashed and set up SSH key login. Wrote ledblink and ran it with `make run`. Made the HTML recap. `~/yocto` became a repo with the layers as submodules; files were copied to the GitHub clone `~/Documents/yocto`. |

## Next steps (agreed order)
1. **Own layer `meta-mydk1`** (in the repo root): `bitbake-layers create-layer ../meta-mydk1`, then `bitbake-layers add-layer ../meta-mydk1`.
   - Write `recipes-apps/ledblink/ledblink_1.0.bb`, using `SRC_URI = "file://ledblink.c file://Makefile"` (copy them into `files/`), `S = "${WORKDIR}"`, `EXTRA_OEMAKE`, `do_install` calling `oe_runmake install DESTDIR=${D}`, plus a LICENSE and LIC_FILES_CHKSUM.
   - Add it to the image: `IMAGE_INSTALL:append = " ledblink"` at first, then a custom image recipe in the layer.
   - Explain each variable as you go.
2. Move the DK1 settings from `local.conf` into the layer: a `conf/machine/stm32mp157d-dk1.conf` that reuses ST's includes, or the layer's `layer.conf`/bbappends.
3. Dev conveniences:
   - Bake the SSH public key into the image (a rootfs postprocess or a recipe), so a reflash doesn't lose it.
   - Set the hostname, a static IP or NTP.
   - Debug from VS Code with `gdbserver`.
   - `devtool modify` for the kernel or device tree.
4. Later topics:
   - Edit the device tree: another GPIO or LED, or a sensor on the Arduino connector.
   - Cortex-M4 firmware through `remoteproc`.
   - ST's full `openstlinux-weston` distro, if a GUI is wanted.
