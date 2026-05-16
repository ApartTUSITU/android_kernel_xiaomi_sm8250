# Warning
**中文:**  
内核源码仍在开发中，可能会导致一些不可预料的问题，请谨慎使用。  
**English:**  
The kernel source code is still under development and may cause some unpredictable problems. Please use it with caution.  

# ApartTUSITU's Xiaomi SM8250 Kernel  

## 目录 / Table of Contents
- [简介 / Introduction](#简介--introduction)  
- [特性 / Features](#特性--features)  
- [注意事项 / Notes](#注意事项--notes)  
- [社区 / Community](#社区--community)  
- [支持的设备 / Supported Devices](#支持的设备--supported-devices)   
- [构建方法 / Build Instructions](#构建方法--build-instructions)  
  - [快速构建 / Quick Build](#快速构建--quick-build)  
  - [手动构建 / Manual Build](#手动构建--manual-build)  

---

## 简介 / Introduction
**中文:**  
该 repo 基于 [LineageOS/android_kernel_xiaomi_sm8250](https://github.com/LineageOS/android_kernel_xiaomi_sm8250) 的 `lineage-23.2` 分支。HyperOS/MIUI 所需的代码及部分设备所需驱动是对照着 [Strawing 的仓库](https://github.com/liyafe1997/kernel_xiaomi_sm8250_mod) 的提交记录挑选并提取的，它们来自 [UtsavBalar1231 的仓库](https://github.com/UtsavBalar1231/kernel_xiaomi_sm8250) 和 [Xiaomi_Kernel_OpenSource](https://github.com/MiCode/Xiaomi_Kernel_OpenSource)。  

本内核站在巨人的肩膀上，结合了多个上游的贡献，使其尽可能实用和完善。  

**English:**  
This repository is based on the `lineage-23.2` branch of [LineageOS/android_kernel_xiaomi_sm8250](https://github.com/LineageOS/android_kernel_xiaomi_sm8250).  

Code required for HyperOS/MIUI support, as well as some device-specific drivers, was selectively cherry-picked by comparing commit histories from [Strawing's repo](https://github.com/liyafe1997/kernel_xiaomi_sm8250_mod), which in turn sources changes from [UtsavBalar1231's repo](https://github.com/UtsavBalar1231/kernel_xiaomi_sm8250) and [Xiaomi_Kernel_OpenSource](https://github.com/MiCode/Xiaomi_Kernel_OpenSource).  

This kernel stands on the shoulders of giants, combining contributions from multiple upstreams to make it as practical and well-rounded as possible.  

---

## 特性 / Features
**中文:**  
本内核支持 [ReSukiSU](https://github.com/ReSukiSU/ReSukiSU)(基于 [SukiSU Ultra](https://github.com/SukiSU-Ultra/SukiSU-Ultra), 一个 KernelSU 的 fork，支持 KPM) & [SuSFS](https://gitlab.com/simonpunk/susfs4ksu)。请自行安装 ReSukiSU 的管理器。NoKernelSU 版本支持应用 Magisk 和 APatch(及他们的分支)。  

**Release** 里的编译好的内核成品由 `android16-aptusitu-new` 分支编译，应当能在原版 MIUI/HyperOS 和第三方的基于 AOSP 的各种 Android11-16 的 ROM 上使用。欢迎大家尝试并反馈(提 Issue 或 Pull Requests)! 酷友们到 [这个帖子](https://www.coolapk.com/feed/69700122) 讨论或反馈，也可以加入 QQ 群，或者给我私信反馈!  

以下是一些具体的功能:  
1. 支持 USB 串口驱动(CH340/FTDI/PL2303/OTI6858/TI/SPCP8X5/QT2/UPD78F0730/CP210X)  
2. 支持 CANBus 和 USB CAN (如 CANable) 适配器  
3. F2FS 开启了 realtime discard 以更好地 TRIM 闪存  
4. 支持 EROFS  
5. zRAM 支持 LZ4、LZ4HC、lz4k_oplus、LZ4KD、ZSTD 等压缩算法  
6. 向后移植 5.10 BPF(支持安卓 16)  
7. 触摸屏驱动、相机驱动、音频驱动、GPU/DRM/MSM、CNSS2 驱动使用小米版本(来自 [UtsavBalar1231's repo](https://github.com/UtsavBalar1231/kernel_xiaomi_sm8250) 和 [MiCode](https://github.com/MiCode), AOSP 版 Display/DRM 驱动为 LineageOS 版本), 并且触摸屏驱动具有双击亮屏节点  
8. 修复[电量卡在 1% 的问题](https://github.com/liyafe1997/Xiaomi-fix-battery-one-percent)，并且支持解容  
9. 集成 [BBG(Baseband-guard)](https://github.com/vc-teahouse/Baseband-guard)  

**English:**  
This kernel supports [ReSukiSU](https://github.com/ReSukiSU/ReSukiSU) (based on [SukiSU Ultra](https://github.com/SukiSU-Ultra/SukiSU-Ultra), a fork of KernelSU with KPM support) & [SuSFS](https://gitlab.com/simonpunk/susfs4ksu).  
Please install the ReSukiSU Manager by yourself.  
The NoKernelSU version supports Magisk and APatch (and their forks).  

The prebuilt kernel in the **Release** section is compiled from the `android16-aptusitu-new` branch, and should work on stock MIUI/HyperOS as well as third-party AOSP-based ROMs for Android 11–16.  
Feedback is welcome (via Issues or Pull Requests)! Coolapk users can join the discussion in [this post](https://www.coolapk.com/feed/69700122), you can also join my QQ group, or send me private feedback.  

Below are some of the key features:  
1. Support for USB serial drivers (CH340 / FTDI / PL2303 / OTI6858 / TI / SPCP8X5 / QT2 / UPD78F0730 / CP210X)  
2. Support for CAN bus and USB CAN adapters (e.g. CANable)  
3. F2FS with realtime discard enabled for improved flash TRIM behavior  
4. Support for EROFS  
5. zRAM with support for multiple compression algorithms, including LZ4, LZ4HC, lz4k_oplus, LZ4KD, and ZSTD  
6. Backported BPF from Linux 5.10 (Android 16 compatible)  
7. Touchscreen, camera, audio, GPU/DRM/MSM, and CNSS2 drivers use Xiaomi-specific implementations (sourced from [UtsavBalar1231’s repository](https://github.com/UtsavBalar1231/kernel_xiaomi_sm8250) and [MiCode](https://github.com/MiCode). The AOSP version of the Display/DRM driver is the LineageOS version); also, the `double_tap` node has been added to the touchscreen driver  
8. Fixes [the issue where the battery percentage gets stuck at 1%](https://github.com/liyafe1997/Xiaomi-fix-battery-one-percent), and supports recognizing higher-capacity replacement batteries  
9. Integrate [BBG(Baseband-guard)](https://github.com/vc-teahouse/Baseband-guard)  

---

## 注意事项 / Notes
**中文:**  
提示：该内核的 zip 包中包含 `dtbo.img`，但是默认不会刷入你的 dtbo 分区。推荐使用原厂的 `dtbo`，或者来自第三方系统包自带的 dtbo(如果原作者确认那好用的话)。因为该源码 build 出来的 `dtbo.img` 有些小问题，比如在锁屏界面上尝试熄屏时，屏幕会突然闪一下到最高亮度。如果你刷过其它第三方内核，或者遇到一些奇怪的问题，建议检查一下你的 `dtbo` 是否被替换过。  

**注意：如果你在用 HyperOS/MIUI 请刷 MIUI 的版本，AOSP 版因为 display 驱动不同，在 HyperOS/MIUI 上屏幕无法正常显示，如果刷内核之后开机黑屏，请先检查你是不是正在用着 HyperOS/MIUI 但是刷了 AOSP 版，默认不受理关于这条的反馈。**  

**English:**  
**Note**: The kernel zip package **contains** `dtbo.img`, but it will **not** flash your dtbo partition by default.  
It is recommended to use the stock `dtbo`, or one from the bundled files of a third-party ROM (if the original author confirms it works well).  
The `dtbo.img` built from this source has some issues—for example, on the lock screen, the display may suddenly flash to max brightness when trying to turn off the screen.  
If you have flashed other third-party kernels or encounter strange issues, please check whether your `dtbo` has been replaced.  

**Warning**: If you are using HyperOS/MIUI, please flash the **MIUI version**.  
The AOSP version has different display drivers, which will cause the screen not to display properly on HyperOS/MIUI.  
If you get a black screen after flashing, check if you are on HyperOS/MIUI but flashed the AOSP version.  
Feedback about this specific issue will not be accepted by default.  

---

## 社区 / Community
**中文:**  
欢迎加入 ApartTUSITU's QQ 交流群: [700675046](https://qm.qq.com/q/Md7nXA3Toa)。  

**English:**  
Join ApartTUSITU's QQ Group: [700675046](https://qm.qq.com/q/Md7nXA3Toa)。  

---

## 支持的设备 / Supported Devices
| 设备代号 / Codename  | 设备名称 / Device Name            |
|---------------------|-----------------------------------|
| psyche              | Xiaomi 12X                        |
| thyme               | Xiaomi 10S                        |
| umi                 | Xiaomi 10                         |
| munch               | Redmi K40S / POCO F4              |
| lmi                 | Redmi K30 Pro / POCO F2 Pro       |
| cmi                 | Xiaomi 10 Pro                     |
| cas                 | Xiaomi 10 Ultra                   |
| apollo              | Xiaomi 10T / Redmi K30S Ultra     |
| alioth              | Xiaomi 11X / POCO F3 / Redmi K40  |
| elish               | Xiaomi Pad 5 Pro                  |
| enuma               | Xiaomi Pad 5 Pro 5G               |
| dagu                | Xiaomi Pad 5 Pro 12.4             |
| pipa                | Xiaomi Pad 6                      |

---

## 构建方法 / Build Instructions

### 快速构建 / Quick Build
**中文:**  
1. fork 本仓库(别忘了点个 Star~)  
2. 进入 **Actions**  
3. 如果你要为所有支持的设备编译内核，找到 `Build All Devices Kernel (Matrix Parallel + Release)`，点击 `Run workflow`  
4. 如果你要为单个设备编译内核，找到 `Build Kernel`， 点击 `Run workflow` 并选择必要内容  

**English:**  
1. Fork this repo (don’t forget to leave a Star~)  
2. Go to **Actions**  
3. If you want to compile the kernel for all supported devices, find `Build All Devices Kernel (Matrix Parallel + Release)` and click `Run workflow`  
4. If you want to compile the kernel for a single device, find `Build Kernel`, click `Run workflow`, and select the necessary options  

---

### 手动构建 / Manual Build
**中文:**  
1. 准备基本构建环境。  
   需要常用工具链 `git`、`make`、`curl`、`bison`、`flex`、`zip` 等，以及一些软件包。  
   - 在 Debian/Ubuntu:  
   ```
   sudo apt install build-essential git curl wget bison flex zip bc cpio libssl-dev ccache tar
   ```
   还需要 `python` (仅有 `python3` 不够)，可安装:  
   ```
   sudo apt install python-is-python3
   ```

   - 在 RHEL/RPM 系统:  
   ```
   sudo yum groupinstall 'Development Tools'
   sudo yum install wget bc openssl-devel ccache tar
   ```

   注意：`build.sh` 中启用了 `ccache`，路径是 `$HOME/.cache/ccache_mikernel`。可修改或删除。  

2. 下载 [ZyC-Clang v16](https://github.com/ZyCromerZ/Clang/releases/download/16.0.6-20260510-release/Clang-16.0.6-20260510.tar.gz) 工具链:  
   ```
   mkdir zyc-clang
   cd zyc-clang
   wget https://github.com/ZyCromerZ/Clang/releases/download/16.0.6-20260510-release/Clang-16.0.6-20260510.tar.gz
   tar -zxvf Clang-16.0.6-20260510.tar.gz
   cd ..
   ```

3. 构建:  
   - 不使用 KernelSU:  
     ```
     bash build.sh TARGET_DEVICE
     ```
   - 使用 KernelSU:  
     ```
     bash build.sh TARGET_DEVICE ksu
     ```

   示例:  
   - lmi (Redmi K30 Pro/POCO F2 Pro) 不使用 KernelSU:  
     ```
     bash build.sh lmi
     ```
   - umi (Xiaomi 10) 使用 KernelSU:  
     ```
     bash build.sh umi ksu
     ```

   另外，`buildall.sh` 可一次性为所有设备构建。  

**English:**  
1. Prepare the build environment.  
   You need `git`, `make`, `curl`, `bison`, `flex`, `zip`, etc.  
   - On Debian/Ubuntu:  
   ```
   sudo apt install build-essential git curl wget bison flex zip bc cpio libssl-dev ccache tar
   ```
   You also need `python` (not just `python3`):  
   ```
   sudo apt install python-is-python3
   ```

   - On RHEL/RPM-based OS:  
   ```
   sudo yum groupinstall 'Development Tools'
   sudo yum install wget bc openssl-devel ccache tar
   ```

   Note: `ccache` is enabled in `build.sh` (`$HOME/.cache/ccache_mikernel`). You may remove/modify it.  

2. Download [ZyC-Clang v16](https://github.com/ZyCromerZ/Clang/releases/download/16.0.6-20260510-release/Clang-16.0.6-20260510.tar.gz) toolchain:  
   ```
   mkdir zyc-clang
   cd zyc-clang
   wget https://github.com/ZyCromerZ/Clang/releases/download/16.0.6-20260510-release/Clang-16.0.6-20260510.tar.gz
   tar -zxvf Clang-16.0.6-20260510.tar.gz
   cd ..
   ```

3. Build:  
   - Without KernelSU:  
     ```
     bash build.sh TARGET_DEVICE
     ```
   - With KernelSU:  
     ```
     bash build.sh TARGET_DEVICE ksu
     ```

   Example:  
   - lmi (Redmi K30 Pro/POCO F2 Pro) without KernelSU:  
     ```
     bash build.sh lmi
     ```
   - umi (Xiaomi 10) with KernelSU:  
     ```
     bash build.sh umi ksu
     ```

   Additionally, `buildall.sh` can build for all supported devices at once.  
