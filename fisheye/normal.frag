#version 150 core
#extension GL_ARB_explicit_attrib_location : enable

//
// テクスチャ座標の位置の画素色をそのまま使う
//

// 背景テクスチャ
uniform sampler2D image;

// テクスチャ座標
in vec2 texcoord;

// フラグメントの色
layout (location = 0) out vec4 fc;

void main(void)
{
  // 画素の陰影を求める
  fc = texture(image, texcoord);
}
