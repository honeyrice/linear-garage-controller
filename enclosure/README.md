# Enclosure & STL

[English](#english) | [简体中文](#简体中文)

## English

The latest CAD archive is V0.27. The main body measures approximately **82.22 × 57.5 × 32 mm**, excluding four external mounting ears. It houses three circuit boards, with connections for USB, a four-wire encoder branch, and two dry-contact leads.

See [dimensions.json](dimensions.json) for the full dimensional record. Values come from measurements and engineering envelopes for this unit; different development boards, headers, relays, and printing tolerances require checking. This file records dimensions and is not standalone CAD source that can generate the model.

The V0.27 control-wire opening is 4.23 × 2.23 mm, with its center 14.35 mm above the enclosure's external bottom surface. Use the complete base and matching lid together; do not freely mix older lids with this base.

Some lid-to-base snap geometry was adapted from external reference models. This directory provides the final V0.27 base and lid STLs. Original reference models and build scripts that depend on them are not included.

The author confirmed final printing, assembly, and installation inside the opener. Photographs cannot uniquely identify the printed STL revision. V0.27 is the latest CAD record; installation photographs are not exact revision identification.

### STL downloads

- [V0.27 complete base](stl/garage-v0.27-base.stl)
- [V0.27 matching lid](stl/garage-v0.27-lid.stl)
- [SHA-256 checksums](stl/SHA256SUMS)
- [Mesh validation results](mesh-validation.json)

Choose **Download raw file** on the GitHub file page. Both models use **millimeters (mm)**. STL does not store units, so import into the slicer as millimeters at 100% scale. The base includes external mounting ears, making its full bounding width about 73.9 mm; the 57.5 mm dimension above is the body width without ears.

Both STLs are byte-for-byte identical to the local V0.27 archive and have watertight meshes, consistent winding, and a single connected component. Publication only rechecked existing meshes; it did not include new slicing or physical fit testing.

---

## 简体中文

最新 CAD 档案为 V0.27，主体外形约 **82.22 × 57.5 × 32 mm**，不含四个外置固定耳。布置三块电路板，外接 USB、四芯编码器支路、两芯干接点线。

完整尺寸参数见 [dimensions.json](dimensions.json)。参数来自本机实测及工程包络；不同开发板、排针、继电器和打印公差须重新核对。该文件是尺寸记录，不是独立可生成模型的 CAD 源码。

V0.27 控制线孔为 4.23 × 2.23 mm，中心距盒子外底面 14.35 mm。完整底壳和上盖必须配套，旧上盖不能随意混搭。

外壳的部分盒盖扣合几何参考了外部模型。本目录提供最终 V0.27 底壳与上盖 STL；原始参考模型及依赖它们的构建脚本未包含。

项目最终打印、装配与机内安装已由作者确认；照片无法唯一识别打印 STL 的修订号。V0.27 是最新 CAD 记录，不把安装照片当作精确版本鉴定。

### STL 下载

- [V0.27 完整底壳](stl/garage-v0.27-base.stl)
- [V0.27 配套上盖](stl/garage-v0.27-lid.stl)
- [SHA-256 校验值](stl/SHA256SUMS)
- [本次网格核验](mesh-validation.json)

在 GitHub 文件页选择 **Download raw file** 下载。两个模型均使用 **毫米（mm）**；STL 本身不保存单位，导入切片软件时按毫米、100% 比例解释。底壳包含外置固定耳，完整包络宽度约 73.9 mm，正文 57.5 mm 为不含固定耳的主体宽度。

两份 STL 与本地 V0.27 档案逐字节一致，均为闭合网格、法线绕向一致、单个连通件。此次仅发布已有模型并重验网格，没有重新切片或进行新的实物试装。
