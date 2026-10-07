# FDP-Rocks Evaluation Kernel

This branch is the guest kernel used for the FDP-Rocks (USENIX ATC '26)
evaluation. It reports itself as `6.10.0-fdp-enabler-g5b8dbac4391d-dirty`:
revision `5b8dbac4391d` plus the commit on top of it, which was uncommitted
when the evaluation kernel was built.

## What the top commit changes

Upstream 6.10 accepts write-lifetime hints 0-5 only. FDP-Rocks uses hints
6-12 (one per LSM level). The commit:

- `include/uapi/linux/fcntl.h`: defines hint values up to 15;
- `fs/fcntl.c`: accepts hints 0-15 in `F_SET_RW_HINT` and no longer forces
  the inode hint to 0;
- `fs/ext4/fdp.c`: passes hints above 5 through unchanged;
- `drivers/nvme/host/core.c`: logs the number of placement handles reported
  by the device.

The NVMe driver uses the hint as the placement-handle index, capped at
`nr_plids - 1`. The device must expose at least 13 reclaim unit handles; the
evaluated WARP configuration exposes 15. `fcntl_set_rw_hint` logs one
`pr_info` line per call, as in the evaluated kernel.

## Build and install (Ubuntu 22.04 guest)

```bash
sudo apt-get install -y build-essential libncurses-dev flex bison bc \
    libssl-dev libelf-dev dwarves fakeroot cpio rsync
git clone -b fdp-rocks-atc26 \
    https://github.com/strivesnail/linux-6.10-fdp-enabler.git
cd linux-6.10-fdp-enabler
cp fdp-rocks/config-6.10.0-fdp-enabler .config
make olddefconfig
make -j"$(nproc)" bindeb-pkg
sudo dpkg -i ../linux-image-6.10.0-fdp-enabler*.deb \
    ../linux-headers-6.10.0-fdp-enabler*.deb
sudo reboot
```

Select the `6.10.0-fdp-enabler` entry in GRUB if it is not the default.

## Check

```bash
uname -r
sudo dmesg | grep nr_plids
```

With the evaluated WARP configuration, `dmesg` shows
`FDP: final nr_plids = 15`.
