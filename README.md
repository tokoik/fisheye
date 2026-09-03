# fisheye - 全天球・魚眼カメラ映像の展開と 3D 空間マッピング

## 1. 概要

本プログラムは、全天球カメラ（RICOH THETA など）や魚眼カメラ（Kodak SP360 など）で撮影された動画・静止画・ライブ映像をテクスチャとして取り込み、GLSL シェーダで正距円筒図法（パノラマ展開）や透視投影にリアルタイム変換して 3D 空間内にレンダリングするサンプルプログラムです。

- 移行元ブログ記事:
  - [全方位カメラのリアルタイムパノラマ展開 - 床井研究室](https://tokoik.github.io/blog/2016-12-03.html)

## 2. 対応環境

- **Windows**: Visual Studio 2019 / 2022 / 2026 (CMake 経由で GLFW および OpenCV 4.13.0 を自動構成)
- **macOS**: Xcode (GLFW 自動ダウンロード、OpenCV を使用)
- **Ubuntu Linux**: GCC / Make (システム標準の libglfw3-dev, libopencv-dev, libgl1-mesa-dev を使用)

## 3. ビルド手順

### Windows (Visual Studio)

```pwsh
cmake -B build -S .
cmake --build build --config Release
```

### macOS (Xcode)

```bash
brew install opencv
cmake -B build -G Xcode
cmake --build build --config Release
```

### Ubuntu Linux (Makefile)

```bash
sudo apt-get update
sudo apt-get install -y libglfw3-dev libgl1-mesa-dev libopencv-dev
cmake -B build -S .
cmake --build build
```

## 4. 起動方法

ビルド完了後、生成された実行ファイルを実行します。

- **Windows**: `build/Release/fisheye.exe`
- **macOS**: `build/Release/fisheye.app`
- **Linux**: `build/fisheye`

## 5. 操作方法

- **マウスドラッグ**: 視点の回転
- **マウスホイール**: ズーム
- **[SPACE]**: カメラ / 動画ソースの切り替え
- **[q] / [Q] / [ESC]**: プログラムの終了

## 6. プログラムの解説

OpenCV の `cv::VideoCapture` により動画ファイル（`theta.mp4`, `sp360.mp4`）や USB 接続カメラからフレームを取得し、GLSL バーテックスシェーダ・フラグメントシェーダ内で球座標変換を行って正距円筒画像への変換や透視投影による矩形領域の切り出しを行います。
