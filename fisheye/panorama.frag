#version 150 core
#extension GL_ARB_explicit_attrib_location : enable

//
// 正距円筒図法のテクスチャをサンプリング
//

// 背景テクスチャの半径と中心位置
uniform vec4 circle;

// 背景テクスチャ
uniform sampler2D image;

// 背景テクスチャのサイズ
vec2 size = textureSize(image, 0);

// 背景テクスチャのテクスチャ空間上のスケール
vec2 scale = vec2(-0.15915494, -0.31830989) / circle.st;

// 背景テクスチャのテクスチャ空間上の中心位置
vec2 center = circle.pq + 0.5;

// 視線ベクトル
in vec4 vector;

// フラグメントの色
layout (location = 0) out vec4 fc;

void main(void)
{
  // 視線ベクトルを正規化する
  vec4 orientation = normalize(vector);

  // テクスチャ座標を求める
  vec2 u = orientation.xy;
  vec2 v = vec2(orientation.z, length(orientation.xz));
  vec2 texcoord = atan(u, v) * scale + center;

  // 画素の陰影を求める
  fc = texture(image, texcoord);
}
