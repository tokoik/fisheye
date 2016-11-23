fisheye
=======

魚眼レンズ画像の GLSL を使った平面展開のサンプルプログラム

    Copyright (c) 2016 Kohe Tokoi. All Rights Reserved.
    
    Permission is hereby granted, free of charge,  to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction,  including without limitation the rights
    to use, copy,  modify, merge,  publish, distribute,  sublicense,  and/or sell
    copies or substantial portions of the Software.
    
    The above  copyright notice  and this permission notice  shall be included in
    all copies or substantial portions of the Software.
    
    THE SOFTWARE  IS PROVIDED "AS IS",  WITHOUT WARRANTY OF ANY KIND,  EXPRESS OR
    IMPLIED,  INCLUDING  BUT  NOT LIMITED  TO THE WARRANTIES  OF MERCHANTABILITY,
    FITNESS  FOR  A PARTICULAR PURPOSE  AND NONINFRINGEMENT.  IN  NO EVENT  SHALL
    KOHE TOKOI  BE LIABLE FOR ANY CLAIM,  DAMAGES OR OTHER LIABILITY,  WHETHER IN
    AN ACTION  OF CONTRACT,  TORT  OR  OTHERWISE,  ARISING  FROM,  OUT OF  OR  IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

* Kodak SP360 4K、RICOH THETA S などの魚眼カメラのライブストリーミング映像を平面展開します。
* [床井研究室：魚眼レンズ画像の平面展開](http://marina.sys.wakayama-u.ac.jp/~tokoi/?date=20160629)にある手法を使っています。
* [床井研究室：矩形の描き方](http://marina.sys.wakayama-u.ac.jp/~tokoi/?date=20160831)の手法でバーテックスシェーダに実装しています。
* GLFW の version 3 と OpenCV 3.1 (Windows 版) 2.4 (Mac 版) を使っています。
* GLFW と OpenCV を組み込みには、Windows 版は NuGet、Mac 版は HomeBrew を使っています。
* Xcode で実行するときは Working Directory に "$PROJECT_DIR/fisheye" を設定してください。
* マウスの左ボタンドラッグで視線を変更できます。
* マウスのホイールを回すとスクリーンの画角を調整できます。
* シフトキーを押しながらマウスのホイールを回すとレンズの画角を調整できます。
* SHIFT キーを押しながら矢印キーで左目と右目の相対的な位置を調整できます。
* CONTROL キーを押しながら矢印キーで両目のウィンドウ上の位置を調整できます。
    
設定
----

設定項目は main.cpp の最初の部分にあります。

* shader_selection に ExpansionShader.h の中にあるものの番号を設定してください。
* UVC Blender を使うとき (shader_selection == 2, panorama.vert) は main.cpp で GL_CLAMP_TO_BORDER を GL_REPEAT に変更してください。

```
    // 使用するシェーダー
    const int shader_selection(1);
```

* capture_device に使用する USB カメラの番号を設定してください。
* USB カメラが１台しかなければ 0 を設定してください。

```
    // 使用するデバイス
    const int capture_device(0);
```

* カメラの解像度は、THETA S の USB 接続で 1280x720、HDMI 接続で 1920x1080 でテストしています。
* Kodak SP360 4K の HDMI 接続では 1440x1440 を設定してください。
* いまのところ HDMI 接続でカメラがうまく設定されないことがあります (他のプログラムは動くのに)。

```
    // カメラの解像度 (0 ならカメラから取得)
    const int capture_width(1280);
    const int capture_height(720);
```
