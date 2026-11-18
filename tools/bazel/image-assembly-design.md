# SONiC 镜像组装规则设计

## 交付范围

本文件描述组装规则及其格式验证。代码位于 `tools/bazel/image/`，入口为
`//tools/bazel/image:defs.bzl`。组件源码、生产者安装接口和完整产品目标由各自配方维护。
产品目标负责声明组件闭包、安装阶段和功能配置，并承担实际安装及启动验收。

规则直接消费 Bazel 声明的安装 TAR、OCI layout、内核 boot tree 和元数据。
执行工具采用 `cfg = "exec"` 的 executable label；gzip 的 `pigz` 默认引用共享工具，
调用方可显式覆盖。运行依赖通过
目标的 runfiles 进入 action。产品依赖在根 MODULE 和产品 BUILD 中声明。

## 五种 rule

统一导入：

```starlark
load("//tools/bazel/image:defs.bzl",
     "sonic_squashfs", "sonic_initramfs", "docker_overlay2_store",
     "onie_installer", "sonic_disk_image")
```

| rule | 主要输入 | 输出 |
| --- | --- | --- |
| `sonic_squashfs` | 有序安装 TAR、排除路径、压缩算法/块大小、`mksquashfs` | SquashFS |
| `sonic_initramfs` | 对应生成阶段的安装 TAR、kernel metadata、配置目录、BusyBox applet 清单、`modprobe`/`depmod`/`zstd` | 一个 initrd；`audit` 输出组 |
| `docker_overlay2_store` | OCI layout 目标到 tag 的映射、平台、可选版本文件、`tar_split_json`、`pigz`、按需 `zstd`、压缩方式 | Docker overlay2 store TAR，默认 gzip，即 dockerfs |
| `onie_installer` | SquashFS、boot tree、kernel metadata、initrd、平台归档、安装脚本/头部、必需安装资源、模板变量、签名配置、可选 dockerfs/附加资源 | ONIE `.bin` |
| `sonic_disk_image` | 相同系统输入、ONIE boot TAR、平台 TAR、分区布局、匹配 GRUB 资源和执行工具 | qcow2 |

每个 rule 的 `out` 指定输出名。除 initramfs 的 `audit` 外，DefaultInfo 仅导出最终产物。
`installer_payload` 不单独设 rule：ONIE action 内直接写 ZIP；磁盘 action 直接消费文件。

## 输入映射与装配边界

下表定义调用方应提供的目标接口，不创建或迁移对应组件。

| 输入来源 | 应传入的 Bazel 产物 | 进入组装的方式 |
| --- | --- | --- |
| APT | repository 阶段已解出的 `:data` TAR；需要包状态时另取 `:control` | data TAR 直接进入 `layers`；control 由调用方包状态目标消费 |
| 自建 DEB 对应组件 | DEB 封包前的安装 TAR | TAR 直接进入 `layers`，不解 DEB |
| 自建 Python wheel 对应组件 | 优先使用生产者已有安装 TAR | TAR 直接进入 `layers` |
| pip / 只有 wheel 的生产者 | 现有 `wheel_layer` 安装产物 TAR | 由既有安装规则处理 metadata、data、scripts、entry points，再进入 `layers` |
| 普通 file / 配置 / 链接 / 权限 | 现有 `tar` rule 按安装路径和数值元数据生成的 TAR | 进入相应阶段的 `layers` |
| Docker 镜像 | 完整 OCI layout tree；关联 blobs 可由同一目标 runfiles 声明 | `images = {layout_label: "repository:tag"}`，生成 store TAR |
| 内核 | runtime TAR、完整准备后的 boot tree、schema-1 metadata | runtime TAR 进入 rootfs/initrd；boot tree 与 metadata 进入 ONIE/磁盘 |
| 账户 / 包状态 / 维护脚本效果 / 缓存 | 调用方提供的效果 TAR | 按原安装阶段插入有序 `layers` |
| boot / kdump initrd | 对对应阶段分别调用 `sonic_initramfs` 得到的文件 | boot initrd 进入 ONIE/磁盘；kdump initrd 由调用方映射进安装 TAR |

`layers` 的顺序属于调用方安装契约。组装器处理覆盖、merged-/usr 链接、硬链接、
whiteout、opaque xattr 和 numeric uid/gid；它不执行任意 maintainer script、pip
源码编译或目标架构程序。账户、服务链接、证书、dpkg 状态等效果应显式作为输入层。
本轮不替调用方选择完整包集合、解析 APT/pip 锁或生成这些效果。

自建 wheel 有安装 TAR 时直接使用该 TAR；只有 wheel 时才使用已有 wheel 安装能力。
无需额外制造“wheel 的 TAR 层”接口，也不把包名映射表变成新的组件构建系统。

## kernel 接口

镜像规则不假设现有 kernel DEB 的文件名或解包布局。调用方适配其封包前产物为：

- `boot_files`：单个 tree artifact，包含 `boot/vmlinuz-<release>`、
  `boot/config-<release>`、`boot/System.map-<release>`，以及准备后的 EFI、DTB 等完整 boot 内容。
- `kernel_metadata`：单个 JSON；`schema = 1`、`kernel_release`，以及 `boot` 中
  `vmlinuz`、`config`、`System.map` 各自的 `path` 和 `sha256`。
- initramfs 另需 metadata 的 `architecture`、`module_directory` 与对应 runtime TAR。

ONIE 和磁盘规则校验三件套路径、release 与 SHA256 配对，并保留完整 boot tree。
当前 release 的 initrd 由显式 `initrd` 输入替换，安装名由 release 派生。
`boot_file_modes` 使用相对 `boot/` 的路径到八进制权限字符串映射；默认目录 0744、
System.map 0600、其他文件 0644，避免依赖 Bazel 输入在宿主上的权限。
生产者是否已有此接口、如何增加导出属于后续接入，不在本轮修改 kernel 实现。

## 执行工具接口

| 消费方 | 必须显式传入的 executable 属性 |
| --- | --- |
| SquashFS | `mksquashfs` |
| initramfs | `modprobe`、`depmod`；gzip 输出使用 `pigz`；zstd 输出或 .zst 模块另需 `zstd` |
| Docker store | `tar_split_json`；元数据及 gzip 外层使用 `pigz`；含 zstd layer 或输出 zstd 时另需 `zstd` |
| ONIE 开发签名 | `openssl`；证书与密钥为显式文件输入 |
| ONIE 生产签名 | `signing_tool`；其余数据由 `signing_inputs` 声明 |
| qcow2 公共工具 | `mke2fs`、`debugfs`、`sgdisk`、`qemu_img`、`grub_editenv` |
| BIOS | 另需 `grub_mkimage`、`grub_bios_setup` |
| UEFI | 另需 `mkfs_fat`、`mcopy`、`mmd`；`uefi_mode = "grub"` 还需 `grub_mkimage` |

调用方优先选择当前项目受管 APT 工具目标，其解释器、动态加载器、共享库、配置和数据
必须在 runfiles 中。规则不调用宿主隐式安装的同名程序，也不在 action 内执行 apt。

`mke2fs` 的实际能力要求包括 `-d -` 从 TAR stdin 直写、分区 offset、数值所有者、
硬链接、setuid、设备 whiteout 及 opaque xattr。验证到的 APT 1.47.2 构建支持 TAR
直写，但其 xattr 过滤会丢弃 opaque 属性；因此版本号本身不足以判定适用性。
若调用方工具不满足这些能力，应在工具生产者修复或源码构建；本轮只规定并消费该接口。

GRUB core、modules 与 BIOS setup 必须版本匹配。UEFI 需要所选平台的资源树，例如
`x86_64-efi`。规则不混用恢复 ISO 中的旧 modules 与新工具生成的 core。

## 流式与磁盘格式

- 安装 TAR 流合并后直接送入 `mksquashfs`，不物化整棵 rootfs。
- OCI layer 直接转换为 overlay2 store TAR，保留 diff ID、chain ID、image metadata、
  whiteout 和 xattr。原始 layer TAR 同时流入 `tar_split_json`，使用固定官方 API 生成
  raw JSON，再由声明的 `pigz` 一次压缩为 layerdb 的 `tar-split.json.gz`，支持按原字节重建。
  元数据生产过程没有先压缩再解压的中间步骤；layerdb `size` 取 TAR header 声明的数据长度总和。
  产品中间 Docker store 使用 `compression = "none"`，避免非格式边界所需的压缩。
  `compression = "gzip"` 为默认，另支持 `"none"` 和 `"zstd"`；均可供磁盘消费。
  zstd 输出使用声明的 `zstd` 工具直接写最终压缩流。
- ONIE 在最终 `.bin` 中直接写内部 ZIP；为兼容 ONIE BusyBox，local header 写入已知
  CRC/长度，避免 data descriptor。大 dockerfs 保留外层 TAR 的原流式布局。
- 磁盘在 action 临时目录创建稀疏 raw，GPT/ext4 按 offset 组装；SquashFS 通过
  `debugfs` 直接写入。新 raw 使用 `nodiscard`，避免对已知全零文件重复 discard。
- `qemu-img convert -f raw -O qcow2 -S <阈值>` 输出最终 qcow2，随后删除内部 raw。
  raw 不进入 DefaultInfo、输出组或缓存交付。

分区布局由 `partition_layout` JSON 给出，包含 `firmware`、`partitions`、GRUB 平台、
modules、prefix、`grub_partition_role` 和 `grub_directory`；UEFI 另给
`efi_boot_filename`（回退入口），生成 GRUB EFI 时还给 `efi_grub_filename`（厂商目录文件名）。
分区项定义 number、role、起始 MiB、大小 MiB、GPT type 和 label。
规则检查重叠、越界和必需分区；1/4/6 ASIC 的具体参数由产品调用方传入。

## 安装器、签名和 EFI 配置

`onie_installer` 必须传入 `default_platform_conf`、`onie_image_conf`、`arch_image_conf`、
`platforms_asic`、`efi_sbatlevel`，可选 `platform_conf`。安装资源使用原脚本规定的文件名；
`extra_files` 与保留路径冲突会失败。`installer_variables` 提供不含 `%%` 的变量名到值映射，
例如 `ARCH`、`IMAGE_VERSION`、`DEMO_TYPE`、`ENABLE_KDUMP`、`ONIE_IMAGE_PART_SIZE`、
`EXTRA_CMDLINE_LINUX`、`OUTPUT_RAW_IMAGE`、`VAR_LOG_FILE_SIZE`。
原模板中仍有未替换变量时 action 失败，调用方根据实际模板提供所需变量。

`platform_compression` 和 `dockerfs_compression` 均可配置为 `gzip`（默认）或 `none`。
前者直接消费已有 gzip 字节，后者流式压缩普通 TAR。`dockerfs_compression` 另支持
`zstd` 原字节消费，此时须选 `docker_mode = "ram"`，对应原 initramfs 的 zstd 解码路径。
普通磁盘安装的原 `tar xz` 消费路径要求 gzip。
Docker store 保留 gzip 默认值兼容既有调用。完整镜像调用方显式选择 `compression = "none"`，
ONIE 选择 `dockerfs_compression = "none"`，KVM 选择 `docker_store_compression = "none"`。
这样中间存储只生成一次未压缩 TAR，ONIE 在最终格式边界用 `pigz` 压缩；KVM 直接消费 TAR。
平台归档同样可保持未压缩至 ONIE 边界。已经压缩的输入原字节复用，initrd 在 payload ZIP 中采用 stored 模式。
内部 ZIP 和大 dockerfs 外置 TAR 的选择
遵循原安装器的非 ZIP64 限制。

`signing_mode` 接受 `no_sign`、`dev`、`prod`。开发模式使用显式 `openssl`、
`signing_cert`、`signing_key`，按 `scripts/sign_image_dev.sh` 生成 PEM CMS 签名。
生产模式调用 `signing_tool`，`signing_tool_args` 支持 `{input}`、`{output}`、`{mode}`；
默认参数顺序为这三个值。`$(location ...)` 引用必须由 `signing_inputs` 声明。
两种签名均在 header、payload 长度和 SHA1 完成后生成，并追加到完整 unsigned 镜像尾部。
这些是安装器 CMS 签名；EFI 可执行文件的签名由输入生产者提供。

磁盘规则通过声明的 `grub_editenv create` 生成环境块，放在所选分区的
`grub_directory/grubenv`。`uefi_mode = "grub"` 生成 EFI GRUB；`"signed"` 使用
`efi_files` 的目标路径到文件 label 映射，例如 `EFI/SONiC-OS/shimx64.efi`、
`grubx64.efi`、`mmx64.efi`。`efi_bootloader` 指定映射中的启动入口，规则将其原字节放入
`EFI/BOOT/<efi_boot_filename>`。同时安装 `EFI/debian/grub.cfg`，按所选 GRUB 分区
label 和目录跳转；调用方可通过 `efi_files` 提供自己的同路径配置。


### 组装与安装配置的单一来源

ONIE 与磁盘规则共享下列可配置属性，调用方可用同一组 `select()` 绑定：

| 属性 | 默认值 | 对应消费者设置 |
| --- | --- | --- |
| `payload_name` | `fs.zip` | `INSTALLER_PAYLOAD` |
| `squashfs_name` | `fs.squashfs` | `FILESYSTEM_SQUASHFS` |
| `dockerfs_name` | `dockerfs.tar.gz` | `FILESYSTEM_DOCKERFS` |
| `docker_directory` | `docker` | `DOCKERFS_DIR` |
| `docker_mode` | `disk` | `docker_inram=off`；`ram` 对应 `on` |

名称采用 shell 安全的单个文件名，避免原安装脚本未加引号的路径消费产生歧义。
ONIE 在通用和架构配置末尾写入所选属性，使实际成员名与消费者变量一致。
保留的安装资源、boot/platform 及数据文件发生名称冲突时失败。
`default_platform_conf` 的历史硬编码内核路径按 metadata 的 release 绑定；`ARCH`
与 metadata 不匹配时失败。`installer_variables` 继续负责安装脚本的业务变量。

磁盘的 `grub_config` 支持 `%%KERNEL_RELEASE%%`、`%%IMAGE_VERSION%%`、
`%%SONIC_ROOT%%`、`%%SQUASHFS_NAME%%`、`%%DOCKERFS_NAME%%`。
`SONIC_ROOT` 使用所选 SONiC 分区的 `LABEL=...`，避免依赖构建时随机生成的 UUID。
历史 SONiC `image-*/boot/vmlinuz-*`、initrd 路径、loop 文件与该条 linux 的 root
参数也绑定到所选 metadata、版本、分区和 SquashFS 名称，保留 ONIE 菜单与其他参数。

`sonic_disk_image` 的 `docker_store_compression` 声明输入为 `none|gzip|zstd`，默认 gzip，
与实际 magic 不符时失败。`docker_mode = "disk"` 将 TAR 流写入
`image-<version>/<docker_directory>`，zstd 解码通过显式 exec 工具流式进行。
`ram` 将已有 gzip/zstd 归档原字节放入 `image-<version>/<dockerfs_name>`；普通 TAR
流式转为 gzip。这样与 `installer/install.sh` 的两种放置结果一致。
调用方为 ONIE、磁盘及 initrd 输入生产者选择一致的命名与 Docker 模式；initrd 中的
union-mount 脚本仍由对应输入层提供，组装器直接消费显式 initrd 文件。

直接组装保留为实现方案。验收比较可观察的文件、元数据、分区和启动引用，
跳过虚拟机中的安装过程属于实现方式变化。完整产品启动测试属于后续产品接入验证。
发布层若需要原脚本的 `.qcow2.gz`，由调用方压缩目标完成；UEFI 磁盘提供回退入口。

## Bazel 配置方式

可变选择通过 rule 属性与 `select()` 传入。调用方定义 build setting 和
`config_setting`；规则保持五种，不自行读取 Make 变量或环境变量。下面是调用方
BUILD 的属性片段；示例 label 由调用方提供：

```starlark
load("@bazel_skylib//rules:common_settings.bzl", "string_flag")

string_flag(
    name = "signing",
    build_setting_default = "no_sign",
    values = ["no_sign", "dev", "prod"],
)
config_setting(name = "dev", flag_values = {":signing": "dev"})
config_setting(name = "prod", flag_values = {":signing": "prod"})

# onie_installer(...) 中的属性：
# signing_mode = select({":dev": "dev", ":prod": "prod",
#                        "//conditions:default": "no_sign"}),
# openssl = select({":dev": ":openssl", "//conditions:default": None}),
# signing_cert = select({":dev": ":dev_cert", "//conditions:default": None}),
# signing_key = select({":dev": ":dev_key", "//conditions:default": None}),
# signing_tool = select({":prod": ":prod_signer", "//conditions:default": None}),
```

同样配置 `installer_variables`、压缩方式、`boot_file_modes`、`uefi_mode`、`efi_files`、
`efi_bootloader`、上述命名与 Docker 放置属性及工具 label。BIOS/UEFI 专用工具可在不适用的配置选 `None`，所选布局
在 action 中验证其实际工具需求。

`sonic_initramfs` 提供 `modules = "most" | "list"` 与
`compression = "zstd" | "gzip" | "xz"`。`compression_level = -1` 表示读取输入配置的
`COMPRESSLEVEL`，缺省遵循 initramfs-tools 0.142：zstd 9，gzip/xz 6；显式属性优先。
最终压缩算法必须得到 kernel `CONFIG_RD_*` 支持。`list` 使用显式模块及其依赖，
`most` 另外选择常见模块。这些属性可用 `select()`，写入 initrd 的配置与所选值一致。
现有 hook 采用源码身份审查；任意 shell hook 和未实现的配置项仍会明确拒绝。

## 规则验收与任务

| 修复项 | 独立可观察结果 | 验收方式 |
| --- | --- | --- |
| Docker 压缩 | 安装器可 gzip 解码，数据字节保留 | none/gzip 两种输入读取 |
| 完整 boot | EFI、DTB 等嵌套路径保留，内核 SHA 配对 | ZIP 与 TAR 成员读取 |
| 安装资源与变量 | 必需资源齐全、变量按输入替换、冲突失败 | 安装器 TAR/ZIP 读取和无效输入测试 |
| Docker 元数据 | tar-split 可重建原 TAR，size 符合 Moby 数据长度语义 | raw API → pigz 元数据经独立 APT tar-split asm 逐字节重建 |
| CMS 签名 | no_sign 无尾签名，dev/prod 签名覆盖完整 unsigned 内容 | Bazel 格式测试中实际调用 OpenSSL sign/verify |
| 磁盘启动资源 | grubenv 可读写，EFI 厂商与回退路径字节一致 | 真实 GRUB 工具、ext4/FAT 读回和 qcow2 检查 |
| initramfs 配置 | 压缩数据可独立解码、模块策略按所选值执行 | initramfs 规则测试 |

规则回归目标包括 `assembly_formats_test`、`mkinitramfs_test` 和
`kernel_deb_test`。前者使用 `tests.bzl` 的 `image_assembly_test` 宏传入真实
`tar_split_json`、作为独立 asm oracle 的 `tar_split`、`openssl`、`grub_editenv`、`zstd`
与 `pigz`，执行格式互操作用例。
`mkinitramfs_test` 检查 0.142 配置、模块处理及压缩契约；`kernel_deb_test`
按 Debian ar/TAR 协议检查解压边界、原文件元数据和生产者 receipt 配对。
组件移植、完整产品闭包与系统启动分别由产品目标验收。

## 受管工具与真实格式验收

`assembly_formats_test` 通过共享 `build_tools` 的 executable/runfiles 消费
`tar-split`、OpenSSL、`grub-editenv`、zstd 和 pigz。CI 直接执行 Bazel 测试，
工具版本、执行架构、动态加载器和运行依赖由共享 APT 锁及工具生产目标固定。
测试保持工具调用路径的符号链接语义，使用 `absolute()` 而不是 `resolve()`。

`action_test` 使用真实 `mksquashfs`、`tar_split_json` 和 `pigz` 生产 SquashFS、Docker store 和 ONIE。
`tar_split_json` 复用固定官方 API 源码；APT `tar-split` 仅作为独立 asm oracle，逐字节重建输入 layer TAR。
独立 `unsquashfs` 读取 payload、数值 uid/gid 和 setuid 权限；ZIP/TAR 读取器检查安装器。
AMD64 的独立 `disk_action_test` 通过受管分区、ext4、GRUB 和 qemu-img 工具生成 BIOS qcow2，执行格式、虚拟大小
、`qemu-img check`、MBR/GPT 签名和 BIOS core 嵌入位置检查。GRUB 资源直接引用匹配 `image_runtime` 的 `usr/lib/grub`，
避免复制资源树。BIOS setup 使用匹配 Debian 补丁集的原生 GRUB 2.12 生产者，
规避 APT BIOS setup 对沙箱宿主挂载设备的探测。CI 在 ARM64 上选取 SquashFS、Docker store 和 ONIE 检查。
这些测试中的内核和 initrd 是格式测试输入；真实产品 initramfs 和启动由产品目标验收。

本目录组装规则及本轮新增 Debian 包装的 gzip 输出通过显式 pigz 工具执行。ZIP deflate 保持 ZIP 格式压缩，
输入 gzip 解码保持已有读取协议；两者独立于 gzip 输出工具选择。
