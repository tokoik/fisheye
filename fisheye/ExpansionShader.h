#pragma once

//
// 平面展開に使うシェーダ
//

// シェーダのセットとパラメータ
struct ExpansionShader
{
  // バーテックスシェーダのソースプログラムのファイル名
  const char *vsrc;

  // フラグメントシェーダのソースプログラムのファイル名
  const char *fsrc;

  // カメラの解像度
  const int width, height;

  // イメージサークルの半径と中心位置
  const float circle[4];
};

// シェーダの種類
constexpr ExpansionShader shader_type[] =
{
  // 0: 通常のカメラ
  { "fixed.vert",     "normal.frag",    640,  480, 1.0f, 1.0f, 0.0f, 0.0f },

  // 1: 通常のカメラで視点を回転
  { "rectangle.vert", "normal.frag",    640,  480, 1.0f, 1.0f, 0.0f, 0.0f },

  // 2: 正距円筒図法の画像 (縦線を消すには GL_CLAMP_TO_BORDER を GL_REPEAT にしてください)
  { "panorama.vert",  "panorama.frag", 1280,  720, 1.0f, 1.0f, 0.0f, 0.0f },

  // 3: 180°魚眼カメラ : 3.1415927 / 2 (≒ 180°/ 2)
  { "fisheye.vert",   "normal.frag",   1280,  720, 1.570796327f, 1.570796327f, 0.0f, 0.0f },

  // 4: 180°魚眼カメラ (FUJINON FE185C046HA-1 + SENTECH STC-MCE132U3V) : 3.5779249 / 2 (≒ 205°/ 2)
  { "fisheye.vert",   "normal.frag",   1280, 1024, 1.797689129f, 1.797689129f, 0.0f, 0.0f },

  // 5: 206°魚眼カメラ (Kodak PIXPRO SP360 4K, 手振れ補正あり) : 3.5953783 / 2 (≒ 206°/ 2)
  { "fisheye.vert",   "normal.frag",   1440, 1440, 1.797689129f, 1.797689129f, 0.0f, 0.0f },

  // 6: 235°魚眼カメラ (Kodak PIXPRO SP360 4K, 手振れ補正なし) : 4.1015237 / 2 (≒ 235°/ 2)
  { "fisheye.vert",   "normal.frag",   1440, 1440, 2.050761871f, 2.050761871f, 0.0f, 0.0f },

  // 7: RICHO THETA の USB ライブストリーミング映像 : (手動調整で決めた値)
  { "theta.vert",     "theta.frag",    1280,  720, 1.003f, 1.003f, 0.0f, -0.002f },

  // 8: RICHO THETA の HDMI ライブストリーミング映像 : (手動調整で決めた値)
  { "theta.vert",     "theta.frag",    1920,  1080, 1.003f, 1.003f, 0.0f, -0.002f }
};
