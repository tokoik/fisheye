#pragma once

/*
** ゲームグラフィックス特論用補助プログラム GLFW3 版
**

Copyright (c) 2011-2017 Kohe Tokoi. All Rights Reserved.

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

**
*/

// 補助プログラム
#include "gg.h"
using namespace gg;

// Oculus Rift SDK ライブラリ (LibOVR) の組み込み
#if defined(USE_OCULUS_RIFT)
#  if defined(_WIN32)
#    define GLFW_EXPOSE_NATIVE_WIN32
#    define GLFW_EXPOSE_NATIVE_WGL
#    include <GLFW/glfw3native.h>
#    undef APIENTRY
#    define OVR_OS_WIN32
#    pragma comment(lib, "LibOVR.lib")
#    define NOTIFY(msg) MessageBox(NULL, TEXT(msg), TEXT("HMD Sample"), MB_ICONERROR | MB_OK)
#  else
#    define NOTIFY(msg) std::cerr << msg << '\n'
#  endif
#  include <OVR_CAPI_GL.h>
#  include <Extras/OVR_Math.h>
#  if OVR_PRODUCT_VERSION > 0
#    include <dxgi.h> // GetDefaultAdapterLuid のため
#    pragma comment(lib, "dxgi.lib")

// デフォルトのグラフィックスアダプタの LUID を得る
inline ovrGraphicsLuid GetDefaultAdapterLuid()
{
  ovrGraphicsLuid luid = ovrGraphicsLuid();

#    if defined(_WIN32)
  IDXGIFactory *factory(nullptr);

  if (SUCCEEDED(CreateDXGIFactory(IID_PPV_ARGS(&factory))))
  {
    IDXGIAdapter *adapter(nullptr);

    if (SUCCEEDED(factory->EnumAdapters(0, &adapter)))
    {
      DXGI_ADAPTER_DESC desc;

      adapter->GetDesc(&desc);
      memcpy(&luid, &desc.AdapterLuid, sizeof luid);
      adapter->Release();
    }

    factory->Release();
  }
#    endif

  return luid;
}

// LUID を比較する
inline int Compare(const ovrGraphicsLuid& lhs, const ovrGraphicsLuid& rhs)
{
  return memcmp(&lhs, &rhs, sizeof (ovrGraphicsLuid));
}

#  endif
#endif

// 標準ライブラリ
#include <cstdlib>
#include <iostream>

//
// ウィンドウ関連の処理
//
class Window
{
  // ウィンドウの識別子
  GLFWwindow *window;

  // ビューポートの幅と高さ
  GLsizei width, height;

  // ビューポートのアスペクト比
  GLfloat aspect;

  // シフトキー
  bool shift_key;

  // コントロールキー
  bool control_key;

  // 矢印キー
  int arrow[2];

  // シフトキーを押しながら矢印キー
  int shift_arrow[2];

  // コントロールキーを押しながら矢印キー
  int control_arrow[2];

  // マウスの現在位置
  double mouse_x, mouse_y;

  // マウスホイールの回転量
  double wheel_rotation;

  // シフトを押しながらマウスホイールの回転量
  double shift_wheel_rotation;

  // コントロールを押しながらマウスホイールの回転量
  double control_wheel_rotation;

  // 左ドラッグによるトラックボール
  GgTrackball trackball_left;

  // 右ドラッグによるトラックボール
  GgTrackball trackball_right;

#if defined(USE_OCULUS_RIFT)
  //
  // Oculus Rift
  //

  // Oculus Rift のセッション
  ovrSession session;

  // Oculus Rift の状態
  ovrHmdDesc hmdDesc;

  // Oculus Rift のスクリーンのサイズ
  GLfloat screen[ovrEye_Count][4];

  // Oculus Rift 表示用の FBO
  GLuint oculusFbo[ovrEye_Count];

  // ミラー表示用の FBO
  GLuint mirrorFbo;

#  if OVR_PRODUCT_VERSION > 0
  // Oculus Rift に送る描画データ
  ovrLayerEyeFov layerData;

  // Oculus Rift にレンダリングするフレームの番号
  long long frameIndex;

  // Oculus Rift 表示用の FBO のデプステクスチャ
  GLuint oculusDepth[ovrEye_Count];

  // ミラー表示用の FBO のサイズ
  int mirrorWidth, mirrorHeight;

  // ミラー表示用の FBO のカラーテクスチャ
  ovrMirrorTexture mirrorTexture;
#  else
  // Oculus Rift に送る描画データ
  ovrLayer_Union layerData;

  // Oculus Rift のレンダリング情報
  ovrEyeRenderDesc eyeRenderDesc[ovrEye_Count];

  // Oculus Rift の視点情報
  ovrPosef eyePose[ovrEye_Count];

  // ミラー表示用の FBO のカラーテクスチャ
  ovrGLTexture *mirrorTexture;
#  endif
#endif

  // コピーコンストラクタを封じる
  Window(const Window &w);

  // 代入を封じる
  Window &operator=(const Window &w);

public:

  //
  // コンストラクタ
  //
  Window(const char *title = "GLFW Window", int width = 640, int height = 480,
    int fullscreen = 0, GLFWwindow *share = nullptr)
    : window(nullptr)
  {
    // 初期化済なら true
    static bool initialized(false);

    // GLFW が初期化されていなければ
    if (!initialized)
    {
      // GLFW を初期化する
      if (glfwInit() == GL_FALSE) return;

      // プログラム終了時の処理を登録
      atexit(glfwTerminate);

#if defined(USE_OCULUS_RIFT)
      // Oculus Rift (LibOVR) を初期化する
      if (OVR_FAILURE(ovr_Initialize(nullptr)))
      {
        NOTIFY("LibOVR can not be initialized");
        return;
      }

      // プログラム終了時には LibOVR を終了する
      atexit(ovr_Shutdown);

      // LUID は OpenGL では使っていないらしい
      ovrGraphicsLuid luid;

      // Oculus Rift のセッションを作成する
      session = nullptr;
      if (OVR_FAILURE(ovr_Create(&session, &luid)))
      {
        NOTIFY("Oculus Rift is not connected");
        return;
      }

      // Oculus Rift へのレンダリングに使う FBO の初期値を設定する
      for (int eye = 0; eye < ovrEye_Count; ++eye) oculusFbo[eye] = 0;

      // ミラー表示に使う FBO の初期値を設定する
      mirrorFbo = 0;
      mirrorTexture = nullptr;

#  if OVR_PRODUCT_VERSION > 0
      // デフォルトのグラフィックスアダプタが使われているか確かめる
      if (Compare(luid, GetDefaultAdapterLuid())) return;

      // Asynchronous TimeWarp 処理に使うフレーム番号の初期値を設定する
      frameIndex = 0LL;

      // Oculus Rift へのレンダリングに使う FBO のデプステクスチャの初期値を設定する
      for (int eye = 0; eye < ovrEye_Count; ++eye) oculusDepth[eye] = 0;
#  endif

      // Oculus Rift ではダブルバッファリングしない
      glfwWindowHint(GLFW_DOUBLEBUFFER, GL_FALSE);

      // Oculus Rift では SRGB でレンダリングする
      glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);
#endif

      // OpenGL Version 3.2 Core Profile を選択する
      glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
      glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
      glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
      glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

      // 矢印キーとマウス操作の初期値を設定する
      arrow[0] = arrow[1] = 0;
      shift_arrow[0] = shift_arrow[1] = 0;
      control_arrow[0] = control_arrow[1] = 0;
      wheel_rotation = shift_wheel_rotation = control_wheel_rotation = 0.0;

      // 初期化済みの印をつける
      initialized = true;
    }

    // ディスプレイの情報
    GLFWmonitor *monitor(nullptr);

    // フルスクリーン表示
    if (fullscreen > 0)
    {
      // 接続されているモニタの数を数える
      int mcount;
      GLFWmonitor **const monitors = glfwGetMonitors(&mcount);

      // セカンダリモニタがあればそれを使う
      if (fullscreen > mcount) fullscreen = mcount;
      monitor = monitors[fullscreen - 1];

      // モニタのモードを調べる
      const GLFWvidmode *mode(glfwGetVideoMode(monitor));

      // ウィンドウのサイズをディスプレイのサイズにする
      width = mode->width;
      height = mode->height;
    }

    // GLFW のウィンドウを作成する
    window = glfwCreateWindow(width, height, title, monitor, share);

    // ウィンドウが作成できなければ戻る
    if (!window) return;

    // 現在のウィンドウを処理対象にする
    glfwMakeContextCurrent(window);

    // ゲームグラフィックス特論の都合による初期化を行う
    ggInit();

    // このインスタンスの this ポインタを記録しておく
    glfwSetWindowUserPointer(window, this);

    // キーボードを操作した時の処理を登録する
    glfwSetKeyCallback(window, keyboard);

    // マウスボタンを操作したときの処理を登録する
    glfwSetMouseButtonCallback(window, mouse);

    // マウスホイール操作時に呼び出す処理を登録する
    glfwSetScrollCallback(window, wheel);

    // ウィンドウのサイズ変更時に呼び出す処理を登録する
    glfwSetFramebufferSizeCallback(window, resize);

#if defined(USE_OCULUS_RIFT)
    // Oculus Rift の情報を取り出す
    hmdDesc = ovr_GetHmdDesc(session);

#  if defined(_DEBUG)
    // Oculus Rift の情報を表示する
    std::cerr
      << "\nProduct name: " << hmdDesc.ProductName
      << "\nResolution:   " << hmdDesc.Resolution.w << " x " << hmdDesc.Resolution.h
      << "\nDefault Fov:  (" << hmdDesc.DefaultEyeFov[ovrEye_Left].LeftTan
      << "," << hmdDesc.DefaultEyeFov[ovrEye_Left].DownTan
      << ") - (" << hmdDesc.DefaultEyeFov[ovrEye_Left].RightTan
      << "," << hmdDesc.DefaultEyeFov[ovrEye_Left].UpTan
      << ")\n              (" << hmdDesc.DefaultEyeFov[ovrEye_Right].LeftTan
      << "," << hmdDesc.DefaultEyeFov[ovrEye_Right].DownTan
      << ") - (" << hmdDesc.DefaultEyeFov[ovrEye_Right].RightTan
      << "," << hmdDesc.DefaultEyeFov[ovrEye_Right].UpTan
      << ")\nMaximum Fov:  (" << hmdDesc.MaxEyeFov[ovrEye_Left].LeftTan
      << "," << hmdDesc.MaxEyeFov[ovrEye_Left].DownTan
      << ") - (" << hmdDesc.MaxEyeFov[ovrEye_Left].RightTan
      << "," << hmdDesc.MaxEyeFov[ovrEye_Left].UpTan
      << ")\n              (" << hmdDesc.MaxEyeFov[ovrEye_Right].LeftTan
      << "," << hmdDesc.MaxEyeFov[ovrEye_Right].DownTan
      << ") - (" << hmdDesc.MaxEyeFov[ovrEye_Right].RightTan
      << "," << hmdDesc.MaxEyeFov[ovrEye_Right].UpTan
      << ")\n" << std::endl;
#  endif

    // Oculus Rift に転送する描画データを作成する
#  if OVR_PRODUCT_VERSION > 0
    layerData.Header.Type = ovrLayerType_EyeFov;
#  else
    layerData.Header.Type = ovrLayerType_EyeFovDepth;
#  endif
    layerData.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;   // OpenGL なので左下が原点

    // Oculus Rift 表示用の FBO を作成する
    for (int eye = 0; eye < ovrEye_Count; ++eye)
    {
      // Oculus Rift の視野を取得する
      const auto &fov(hmdDesc.DefaultEyeFov[ovrEyeType(eye)]);

      // Oculus Rift 表示用の FBO のサイズを求める
      const auto textureSize(ovr_GetFovTextureSize(session, ovrEyeType(eye), fov, 1.0f));

      // Oculus Rift 表示用の FBO のアスペクト比を求める
      aspect = static_cast<GLfloat>(textureSize.w) / static_cast<GLfloat>(textureSize.h);

      // Oculus Rift のスクリーンのサイズを保存する
      screen[eye][0] = -fov.LeftTan;
      screen[eye][1] = fov.RightTan;
      screen[eye][2] = -fov.DownTan;
      screen[eye][3] = fov.UpTan;

#  if OVR_PRODUCT_VERSION > 0
      // 描画データに視野を設定する
      layerData.Fov[eye] = fov;

      // 描画データにビューポートを設定する
      layerData.Viewport[eye].Pos = OVR::Vector2i(0, 0);
      layerData.Viewport[eye].Size = textureSize;

      // Oculus Rift 表示用の FBO のカラーバッファとして使うテクスチャセットの特性
      const ovrTextureSwapChainDesc colorDesc =
      {
        ovrTexture_2D,                    // Type
        OVR_FORMAT_R8G8B8A8_UNORM_SRGB,   // Format
        1,                                // ArraySize
        textureSize.w,                    // Width
        textureSize.h,                    // Height
        1,                                // MipLevels
        1,                                // SampleCount
        ovrFalse,                         // StaticImage
        0, 0
      };

      // Oculus Rift 表示用の FBO のレンダーターゲットとして使うテクスチャチェインを作成する
      layerData.ColorTexture[eye] = nullptr;
      if (OVR_SUCCESS(ovr_CreateTextureSwapChainGL(session, &colorDesc, &layerData.ColorTexture[eye])))
      {
        // 作成したテクスチャチェインの長さを取得する
        int length(0);
        if (OVR_SUCCESS(ovr_GetTextureSwapChainLength(session, layerData.ColorTexture[eye], &length)))
        {
          // テクスチャチェインの個々の要素について
          for (int i = 0; i < length; ++i)
          {
            // テクスチャのパラメータを設定する
            GLuint texId;
            ovr_GetTextureSwapChainBufferGL(session, layerData.ColorTexture[eye], i, &texId);
            glBindTexture(GL_TEXTURE_2D, texId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
          }
        }

        // Oculus Rift 表示用の FBO のデプスバッファとして使うテクスチャを作成する
        glGenTextures(1, &oculusDepth[eye]);
        glBindTexture(GL_TEXTURE_2D, oculusDepth[eye]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, textureSize.w, textureSize.h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      }
#  else
      // 描画データに視野を設定する
      layerData.EyeFov.Fov[eye] = fov;

      // 描画データにビューポートを設定する
      layerData.EyeFov.Viewport[eye].Pos = OVR::Vector2i(0, 0);
      layerData.EyeFov.Viewport[eye].Size = textureSize;

      // Oculus Rift 表示用の FBO のカラーバッファとして使うテクスチャセットを作成する
      ovrSwapTextureSet *colorTexture;
      ovr_CreateSwapTextureSetGL(session, GL_SRGB8_ALPHA8, textureSize.w, textureSize.h, &colorTexture);
      layerData.EyeFov.ColorTexture[eye] = colorTexture;

      // Oculus Rift 表示用の FBO のデプスバッファとして使うテクスチャセットを作成する
      ovrSwapTextureSet *depthTexture;
      ovr_CreateSwapTextureSetGL(session, GL_DEPTH_COMPONENT32F, textureSize.w, textureSize.h, &depthTexture);
      layerData.EyeFovDepth.DepthTexture[eye] = depthTexture;

      // Oculus Rift のレンズ補正等の設定値を取得する
      eyeRenderDesc[eye] = ovr_GetRenderDesc(session, ovrEyeType(eye), fov);
#  endif
    }

#  if OVR_PRODUCT_VERSION > 0
    // 姿勢のトラッキングにおける原点を目の位置に設定する
    // (原点を床の高さに設定するときは ovrTrackingOrigin_FloorLevel を指定する)
    ovr_SetTrackingOriginType(session, ovrTrackingOrigin_EyeLevel);

    // HMD の現在位置を基準にする
    // (ovrTrackingOrigin_FloorLevel のときは高さはリセットされない)
    ovr_RecenterTrackingOrigin(session);

    // ミラー表示用の FBO を作成する
    const ovrMirrorTextureDesc mirrorDesc =
    {
      OVR_FORMAT_R8G8B8A8_UNORM_SRGB,   // Format
      mirrorWidth = width,              // Width
      mirrorHeight = height,            // Height
      0                                 // Flags
    };

    // ミラー表示用の FBO のカラーバッファとして使うテクスチャを作成する
    if (OVR_SUCCESS(ovr_CreateMirrorTextureGL(session, &mirrorDesc, &mirrorTexture)))
    {
      // 作成したテクスチャのテクスチャ名を得る
      GLuint texId;
      if (OVR_SUCCESS(ovr_GetMirrorTextureBufferGL(session, mirrorTexture, &texId)))
      {
        // 作成したテクスチャをミラー表示用の FBO にカラーバッファとして組み込む
        glGenFramebuffers(1, &mirrorFbo);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFbo);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texId, 0);
        glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
      }
    }
#  else
    // ミラー表示用の FBO を作成する
    if (OVR_SUCCESS(ovr_CreateMirrorTextureGL(session, GL_SRGB8_ALPHA8, width, height, reinterpret_cast<ovrTexture **>(&mirrorTexture))))
    {
      glGenFramebuffers(1, &mirrorFbo);
      glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFbo);
      glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mirrorTexture->OGL.TexId, 0);
      glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
      glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    }
#  endif

    // Oculus Rift のレンダリング用の FBO を作成する
    glGenFramebuffers(ovrEye_Count, oculusFbo);

    // Oculus Rift にレンダリングするときは sRGB カラースペースを使う
    glEnable(GL_FRAMEBUFFER_SRGB);

    // Oculus Rift への表示ではスワップ間隔を待たない
    glfwSwapInterval(0);
#else
    // スワップ間隔を待つ
    glfwSwapInterval(1);
#endif

    // ビューポートと投影変換行列を初期化する
    resize(window, width, height);
  }

  //
  // デストラクタ
  //
  virtual ~Window()
  {
    // ウィンドウが作成されていなければ戻る
    if (!window) return;

#if defined(USE_OCULUS_RIFT)
    // ミラー表示用の FBO を削除する
    if (mirrorFbo) glDeleteFramebuffers(1, &mirrorFbo);

    // ミラー表示に使ったテクスチャを開放する
    if (mirrorTexture)
    {
#  if OVR_PRODUCT_VERSION > 0
      ovr_DestroyMirrorTexture(session, mirrorTexture);
#  else
      glDeleteTextures(1, &mirrorTexture->OGL.TexId);
      ovr_DestroyMirrorTexture(session, reinterpret_cast<ovrTexture *>(mirrorTexture));
#  endif
    }

    // Oculus Rift のレンダリング用の FBO を削除する
    glDeleteFramebuffers(ovrEye_Count, oculusFbo);

    // Oculus Rift 表示用の FBO を削除する
    for (int eye = 0; eye < ovrEye_Count; ++eye)
    {
#  if OVR_PRODUCT_VERSION > 0
      // レンダリングターゲットに使ったテクスチャを開放する
      if (layerData.ColorTexture[eye])
      {
        ovr_DestroyTextureSwapChain(session, layerData.ColorTexture[eye]);
        layerData.ColorTexture[eye] = nullptr;
      }

      // デプスバッファとして使ったテクスチャを開放する
      glDeleteTextures(1, &oculusDepth[eye]);
      oculusDepth[eye] = 0;
#  else
      // レンダリングターゲットに使ったテクスチャを開放する
      auto *const colorTexture(layerData.EyeFov.ColorTexture[eye]);
      for (int i = 0; i < colorTexture->TextureCount; ++i)
      {
        const auto *const ctex(reinterpret_cast<ovrGLTexture *>(&colorTexture->Textures[i]));
        glDeleteTextures(1, &ctex->OGL.TexId);
      }
      ovr_DestroySwapTextureSet(session, colorTexture);

      // デプスバッファとして使ったテクスチャを開放する
      auto *const depthTexture(layerData.EyeFovDepth.DepthTexture[eye]);
      for (int i = 0; i < depthTexture->TextureCount; ++i)
      {
        const auto *const dtex(reinterpret_cast<ovrGLTexture *>(&depthTexture->Textures[i]));
        glDeleteTextures(1, &dtex->OGL.TexId);
      }
      ovr_DestroySwapTextureSet(session, depthTexture);
#  endif
    }

    // Oculus Rift のセッションを破棄する
    ovr_Destroy(session);
    session = nullptr;
#endif

    // ウィンドウを破棄する
    glfwDestroyWindow(window);
  }

#if defined(USE_OCULUS_RIFT)
  //
  // Oculus Rift による描画開始
  //
  bool start()
  {
#  if OVR_PRODUCT_VERSION > 0
    // セッションの状態を取得する
    ovrSessionStatus sessionStatus;
    ovr_GetSessionStatus(session, &sessionStatus);

    // アプリケーションが終了を要求しているときはウィンドウのクローズフラグを立てる
    if (sessionStatus.ShouldQuit) glfwSetWindowShouldClose(window, GL_TRUE);

    // Oculus Rift に表示されていないときは戻る
    if (!sessionStatus.IsVisible) return false;

    // 現在の状態をトラッキングの原点にする
    if (sessionStatus.ShouldRecenter) ovr_RecenterTrackingOrigin(session);

    // Oculus Remote の Back ボタンを押したら視点の位置をリセットする
    ovrInputState inputState;
    if (OVR_SUCCESS(ovr_GetInputState(session, ovrControllerType_Remote, &inputState)))
    {
      if (inputState.Buttons & ovrButton_Back) ovr_RecenterTrackingOrigin(session);
    }

    // HmdToEyeOffset などは実行時に変化するので毎フレーム ovr_GetRenderDesc() で ovrEyeRenderDesc を取得する
    const ovrEyeRenderDesc eyeRenderDesc[] =
    {
      ovr_GetRenderDesc(session, ovrEyeType(0), hmdDesc.DefaultEyeFov[0]),
      ovr_GetRenderDesc(session, ovrEyeType(1), hmdDesc.DefaultEyeFov[1])
    };

    // Oculus Rift のスクリーンのヘッドトラッキング位置からの変位を取得する
    const ovrVector3f hmdToEyeOffset[] =
    {
      eyeRenderDesc[0].HmdToEyeOffset,
      eyeRenderDesc[1].HmdToEyeOffset
    };

    // 視点の姿勢情報を取得する
    ovr_GetEyePoses(session, ++frameIndex, ovrTrue, hmdToEyeOffset, layerData.RenderPose, &layerData.SensorSampleTime);
#  else
    // フレームのタイミング計測開始
    const auto ftiming(ovr_GetPredictedDisplayTime(session, 0));

    // sensorSampleTime の取得は可能な限り ovr_GetTrackingState() の近くで行う
    layerData.EyeFov.SensorSampleTime = ovr_GetTimeInSeconds();

    // ヘッドトラッキングの状態を取得する
    const auto hmdState(ovr_GetTrackingState(session, ftiming, ovrTrue));

    // Oculus Rift のスクリーンのヘッドトラッキング位置からの変位を取得する
    const ovrVector3f hmdToEyeViewOffset[] =
    {
      eyeRenderDesc[0].HmdToEyeViewOffset,
      eyeRenderDesc[1].HmdToEyeViewOffset
    };

    // 視点の姿勢情報を求める
    ovr_CalcEyePoses(hmdState.HeadPose.ThePose, hmdToEyeViewOffset, eyePose);
#  endif

    return true;
  }

  //
  // Oculus Rift の描画する目の指定
  //
  void select(int eye, GLfloat *screen, GLfloat *position, GLfloat *orientation)
  {
#  if OVR_PRODUCT_VERSION > 0
    // Oculus Rift にレンダリングする FBO に切り替える
    if (layerData.ColorTexture[eye])
    {
      // FBO のカラーバッファに使う現在のテクスチャのインデックスを取得する
      int curIndex;
      ovr_GetTextureSwapChainCurrentIndex(session, layerData.ColorTexture[eye], &curIndex);

      // FBO のカラーバッファに使うテクスチャを取得する
      GLuint curTexId;
      ovr_GetTextureSwapChainBufferGL(session, layerData.ColorTexture[eye], curIndex, &curTexId);

      // FBO を設定する
      glBindFramebuffer(GL_FRAMEBUFFER, oculusFbo[eye]);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, curTexId, 0);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, oculusDepth[eye], 0);

      // ビューポートを設定する
      const auto &vp(layerData.Viewport[eye]);
      glViewport(vp.Pos.x, vp.Pos.y, vp.Size.w, vp.Size.h);
    }

    // Oculus Rift の片目の位置と回転を取得する
    const auto &p(layerData.RenderPose[eye].Position);
    const auto &o(layerData.RenderPose[eye].Orientation);
#  else
    // レンダーターゲットに描画する前にレンダーターゲットのインデックスをインクリメントする
    auto *const colorTexture(layerData.EyeFov.ColorTexture[eye]);
    colorTexture->CurrentIndex = (colorTexture->CurrentIndex + 1) % colorTexture->TextureCount;
    auto *const depthTexture(layerData.EyeFovDepth.DepthTexture[eye]);
    depthTexture->CurrentIndex = (depthTexture->CurrentIndex + 1) % depthTexture->TextureCount;

    // レンダーターゲットを切り替える
    glBindFramebuffer(GL_FRAMEBUFFER, oculusFbo[eye]);
    const auto &ctex(reinterpret_cast<ovrGLTexture *>(&colorTexture->Textures[colorTexture->CurrentIndex]));
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ctex->OGL.TexId, 0);
    const auto &dtex(reinterpret_cast<ovrGLTexture *>(&depthTexture->Textures[depthTexture->CurrentIndex]));
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dtex->OGL.TexId, 0);

    // ビューポートを設定する
    const auto &vp(layerData.EyeFov.Viewport[eye]);
    glViewport(vp.Pos.x, vp.Pos.y, vp.Size.w, vp.Size.h);

    // Oculus Rift の片目の位置と回転を取得する
    const auto &p(eyePose[eye].Position);
    const auto &o(eyePose[eye].Orientation);
#  endif

    // Oculus Rift のスクリーンの大きさを返す
    screen[0] = this->screen[eye][0];
    screen[1] = this->screen[eye][1];
    screen[2] = this->screen[eye][2];
    screen[3] = this->screen[eye][3];

    // Oculus Rift の位置を反転して返す
    position[0] = -p.x;
    position[1] = -p.y;
    position[2] = -p.z;
    position[3] = 1.0f;

    // Oculus Rift の方向を反転して返す
    orientation[0] = -o.x;
    orientation[1] = -o.y;
    orientation[2] = -o.z;
    orientation[3] = o.w;
  }

  //
  // 図形の描画を完了する
  //
  void Window::commit(int eye)
  {
#if OVR_PRODUCT_VERSION > 0
    // GL_COLOR_ATTACHMENT0 に割り当てられたテクスチャが wglDXUnlockObjectsNV() によって
    // アンロックされるために次のフレームの処理において無効な GL_COLOR_ATTACHMENT0 が
    // FBO に結合されるのを避ける
    glBindFramebuffer(GL_FRAMEBUFFER, oculusFbo[eye]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);

    // 保留中の変更を layerData.ColorTexture[eye] に反映しインデックスを更新する
    ovr_CommitTextureSwapChain(session, layerData.ColorTexture[eye]);
#endif
  }

  //
  // Time Warp 処理に使う投影変換行列の成分の設定
  //
  void timewarp(const GgMatrix &projection)
  {
    // TimeWarp に使う変換行列の成分を設定する
#  if OVR_PRODUCT_VERSION < 1
    auto &posTimewarpProjectionDesc(layerData.EyeFovDepth.ProjectionDesc);
    posTimewarpProjectionDesc.Projection22 = (projection.get()[4 * 2 + 2] + projection.get()[4 * 3 + 2]) * 0.5f;
    posTimewarpProjectionDesc.Projection23 = projection.get()[4 * 2 + 3] * 0.5f;
    posTimewarpProjectionDesc.Projection32 = projection.get()[4 * 3 + 2];
#  endif
  }
#endif

  //
  // ウィンドウの識別子の取得
  //
  GLFWwindow *get() const
  {
    return window;
  }

  //
  // ウィンドウを閉じるべきかを判定する
  //
  bool shouldClose()
  {
    // ウィンドウを閉じるか ESC キーがタイプされていれば真を返す
    return glfwWindowShouldClose(window) || glfwGetKey(window, GLFW_KEY_ESCAPE);
  }

  //
  // ビューポートをもとに戻す
  //
  void restoreViewport()
  {
    glViewport(0, 0, width, height);
  }

  //
  // カラーバッファを入れ替えてイベントを取り出す
  //
  void swapBuffers()
  {
    // エラーチェック
    ggError(__FILE__, __LINE__);

#if defined(USE_OCULUS_RIFT)
#  if OVR_PRODUCT_VERSION > 0
    // 描画データを Oculus Rift に転送する
    const auto *const layers(&layerData.Header);
    if (OVR_FAILURE(ovr_SubmitFrame(session, frameIndex, nullptr, &layers, 1)))
#  else
    // Oculus Rift 上の描画位置と拡大率を求める
    ovrViewScaleDesc viewScaleDesc;
    viewScaleDesc.HmdSpaceToWorldScaleInMeters = 1.0f;
    viewScaleDesc.HmdToEyeViewOffset[0] = eyeRenderDesc[0].HmdToEyeViewOffset;
    viewScaleDesc.HmdToEyeViewOffset[1] = eyeRenderDesc[1].HmdToEyeViewOffset;

    // 描画データを更新する
    layerData.EyeFov.RenderPose[0] = eyePose[0];
    layerData.EyeFov.RenderPose[1] = eyePose[1];

    // 描画データを Oculus Rift に転送する
    const auto *const layers(&layerData.Header);
    if (OVR_FAILURE(ovr_SubmitFrame(session, 0, &viewScaleDesc, &layers, 1)))
#  endif
    {
      // 転送に失敗したら Oculus Rift の設定を最初からやり直す必要があるらしい
      // けどめんどくさいのでウィンドウを閉じてしまう
      NOTIFY("Oculus Rift has been disconnected");
      glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    // レンダリング結果をミラー表示用のフレームバッファにも転送する
    glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
#  if OVR_PRODUCT_VERSION > 0
    const auto w(mirrorWidth), h(mirrorHeight);
#  else
    const auto w(mirrorTexture->OGL.Header.TextureSize.w);
    const auto h(mirrorTexture->OGL.Header.TextureSize.h);
#  endif
    glBlitFramebuffer(0, h, w, 0, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    // 残っている OpenGL コマンドを実行する
    glFlush();
#else
    // カラーバッファを入れ替える
    glfwSwapBuffers(window);
#endif

    // シフトキーとコントロールキーの状態をリセットする
    shift_key = control_key = false;

    // イベントを取り出す
    glfwPollEvents();

    // マウスの位置を調べる
    glfwGetCursorPos(window, &mouse_x, &mouse_y);
    const GLfloat x(static_cast<GLfloat>(mouse_x));
    const GLfloat y(static_cast<GLfloat>(mouse_y));

    // 左ボタンドラッグ
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1)) trackball_left.motion(x, y);

    // 右ボタンドラッグ
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2)) trackball_right.motion(x, y);
  }

  //
  // ウィンドウのサイズ変更時の処理
  //
  static void resize(GLFWwindow *window, int width, int height)
  {
    // このインスタンスの this ポインタを得る
    Window *const instance(static_cast<Window *>(glfwGetWindowUserPointer(window)));

    if (instance)
    {
      // ウィンドウのサイズを保存する
      instance->width = width;
      instance->height = height;

      // トラックボール処理の範囲を設定する
      instance->trackball_left.region(width, height);
      instance->trackball_right.region(width, height);

#if !defined(USE_OCULUS_RIFT)
      // ウィンドウのアスペクト比を保存する
      instance->aspect = static_cast<GLfloat>(width) / static_cast<GLfloat>(height);

      // ウィンドウ全体に描画する
      instance->restoreViewport();
#endif
    }
  }

  //
  // キーボードをタイプした時の処理
  //
  static void keyboard(GLFWwindow *window, int key, int scancode, int action, int mods)
  {
    // このインスタンスの this ポインタを得る
    Window *const instance(static_cast<Window *>(glfwGetWindowUserPointer(window)));

    if (instance)
    {
      if (action == GLFW_PRESS)
      {
        switch (key)
        {
        case GLFW_KEY_R:
          // マウスホイールの回転量をリセットする
          instance->wheel_rotation = 0.0;
          instance->shift_wheel_rotation = 0.0;
          instance->control_wheel_rotation = 0.0;

          // 矢印キーの設定値をリセットする
          instance->arrow[0] = instance->arrow[1] = 0;
          instance->shift_arrow[0] = instance->shift_arrow[1] = 0;
          instance->control_arrow[0] = instance->control_arrow[1] = 0;

        case GLFW_KEY_O:
          // トラックボールをリセットする
          instance->trackball_left.reset();
          instance->trackball_right.reset();
          break;

        case GLFW_KEY_SPACE:
          break;

        case GLFW_KEY_BACKSPACE:
        case GLFW_KEY_DELETE:
          break;

        case GLFW_KEY_LEFT_SHIFT:
        case GLFW_KEY_RIGHT_SHIFT:
          instance->shift_key = true;
          break;

        case GLFW_KEY_LEFT_CONTROL:
        case GLFW_KEY_RIGHT_CONTROL:
          instance->control_key = true;
          break;

        case GLFW_KEY_UP:
          if (instance->shift_key)
            instance->shift_arrow[1]++;
          else if (instance->control_key)
            instance->control_arrow[1]++;
          else
            instance->arrow[1]++;
          break;

        case GLFW_KEY_DOWN:
          if (instance->shift_key)
            instance->shift_arrow[1]--;
          else if (instance->control_key)
            instance->control_arrow[1]--;
          else
            instance->arrow[1]--;
          break;

        case GLFW_KEY_RIGHT:
          if (instance->shift_key)
            instance->shift_arrow[0]++;
          else if (instance->control_key)
            instance->control_arrow[0]++;
          else
            instance->arrow[0]++;
          break;

        case GLFW_KEY_LEFT:
          if (instance->shift_key)
            instance->shift_arrow[0]--;
          else if (instance->control_key)
            instance->control_arrow[0]--;
          else
            instance->arrow[0]--;
          break;

        default:
          break;
        }
      }
    }
  }

  //
  // マウスボタンを操作したときの処理
  //
  static void mouse(GLFWwindow *window, int button, int action, int mods)
  {
    // このインスタンスの this ポインタを得る
    Window *const instance(static_cast<Window *>(glfwGetWindowUserPointer(window)));

    if (instance)
    {
      // マウスの現在位置を得る
      const GLfloat x(static_cast<GLfloat>(instance->mouse_x));
      const GLfloat y(static_cast<GLfloat>(instance->mouse_y));

      switch (button)
      {
      case GLFW_MOUSE_BUTTON_1:
        if (action)
        {
          // 左ドラッグ開始
          instance->trackball_left.start(x, y);
        }
        else
        {
          // 左ドラッグ終了
          instance->trackball_left.stop(x, y);
        }
        break;

      case GLFW_MOUSE_BUTTON_2:
        if (action)
        {
          // 右ドラッグ開始
          instance->trackball_right.start(x, y);
        }
        else
        {
          // 右ドラッグ終了
          instance->trackball_right.stop(x, y);
        }
        break;

      case GLFW_MOUSE_BUTTON_3:
        break;

      default:
        break;
      }
    }
  }

  //
  // マウスホイールを操作した時の処理
  //
  static void wheel(GLFWwindow *window, double x, double y)
  {
    // このインスタンスの this ポインタを得る
    Window *const instance(static_cast<Window *>(glfwGetWindowUserPointer(window)));

    if (instance)
    {
      if (instance->shift_key)
        instance->shift_wheel_rotation += y;
      else if (instance->control_key)
        instance->control_wheel_rotation += y;
      else
      {
        instance->wheel_rotation += y;
        if (instance->wheel_rotation < -100.0)
          instance->wheel_rotation = -100.0;
        else if (instance->wheel_rotation > 49.0)
          instance->wheel_rotation = 49.0;
      }
    }
  }

  //
  // ウィンドウの幅を得る
  //
  GLsizei getWidth() const
  {
    return width;
  }

  //
  // ウィンドウの高さを得る
  //
  GLsizei getHeight() const
  {
    return height;
  }

  //
  // ウィンドウのサイズを得る
  //
  void getSize(GLsizei *size) const
  {
    size[0] = getWidth();
    size[1] = getHeight();
  }

  //
  // ウィンドウのアスペクト比を得る
  //
  GLfloat getAspect() const
  {
    return aspect;
  }

  //
  // 矢印キーの現在の X 値を得る
  //
  GLfloat getArrowX() const
  {
    return static_cast<GLfloat>(arrow[0]);
  }

  //
  // 矢印キーの現在の Y 値を得る
  //
  GLfloat getArrowY() const
  {
    return static_cast<GLfloat>(arrow[1]);
  }

  //
  // 矢印キーの現在の値を得る
  //
  void getArrow(GLfloat *arrow) const
  {
    arrow[0] = getArrowX();
    arrow[1] = getArrowY();
  }

  //
  // シフトキーを押しながら矢印キーの現在の X 値を得る
  //
  GLfloat getShiftArrowX() const
  {
    return static_cast<GLfloat>(shift_arrow[0]);
  }

  //
  // シフトキーを押しながら矢印キーの現在の Y 値を得る
  //
  GLfloat getShiftArrowY() const
  {
    return static_cast<GLfloat>(shift_arrow[1]);
  }

  //
  // シフトキーを押しながら矢印キーの現在の値を得る
  //
  void getShiftArrow(GLfloat *shift_arrow) const
  {
    shift_arrow[0] = getShiftArrowX();
    shift_arrow[1] = getShiftArrowY();
  }

  //
  // コントロールキーを押しながら矢印キーの現在の X 値を得る
  //
  GLfloat getControlArrowX() const
  {
    return static_cast<GLfloat>(control_arrow[0]);
  }

  //
  // コントロールキーを押しながら矢印キーの現在の Y 値を得る
  //
  GLfloat getControlArrowY() const
  {
    return static_cast<GLfloat>(control_arrow[1]);
  }

  //
  // コントロールキーを押しながら矢印キーの現在の値を得る
  //
  void getControlArrow(GLfloat *control_arrow) const
  {
    control_arrow[0] = getControlArrowX();
    control_arrow[1] = getControlArrowY();
  }

  //
  // マウスの X 座標を得る
  //
  GLfloat getMouseX() const
  {
    return static_cast<GLfloat>(mouse_x);
  }

  //
  // マウスの Y 座標を得る
  //
  GLfloat getMouseY() const
  {
    return static_cast<GLfloat>(mouse_y);
  }

  //
  // マウスの現在位置を得る
  //
  void getMouse(GLfloat *position) const
  {
    position[0] = getMouseX();
    position[1] = getMouseY();
  }

  //
  // マウスホイールの現在の回転角を得る
  //
  GLfloat getWheel() const
  {
    return static_cast<GLfloat>(wheel_rotation);
  }

  //
  // シフトを押しながらマウスホイールの現在の回転角を得る
  //
  GLfloat getShiftWheel() const
  {
    return static_cast<GLfloat>(shift_wheel_rotation);
  }

  //
  // コントロールを押しながらマウスホイールの現在の回転角を得る
  //
  GLfloat getControlWheel() const
  {
    return static_cast<GLfloat>(control_wheel_rotation);
  }

  //
  // 左ボタンによるトラックボールの回転変換行列を得る
  //
  const GgMatrix getLeftTrackball() const
  {
    return trackball_left.getMatrix();
  }

  //
  // 右ボタンによるトラックボールの回転変換行列を得る
  //
  const GgMatrix getRightTrackball() const
  {
    return trackball_right.getMatrix();
  }
};
