#version 150 core

//
// 視線の回転を行う
//

// 視点とテクスチャの画像平面までの距離
const float distance = 5.0;

// スクリーンの格子間隔
uniform vec2 gap;

// スクリーンの大きさと中心位置
uniform vec4 screen;

// スクリーンまでの焦点距離
uniform float focal;

// スクリーンを回転する変換行列
uniform mat4 rotation;

// 背景テクスチャの半径と中心位置
uniform vec4 circle;

// 背景テクスチャ
uniform sampler2D image;

// 背景テクスチャのサイズ
vec2 size = textureSize(image, 0);

// 背景テクスチャのテクスチャ空間上のスケール
vec2 scale = vec2(-0.5 * size.y / size.x, 0.5) / circle.st;

// 背景テクスチャのテクスチャ空間上の中心位置
vec2 center = circle.pq + 0.5;

// テクスチャ座標
out vec2 texcoord;

void main(void)
{
  // 頂点位置
  //   各頂点において gl_VertexID が 0, 1, 2, 3, ... のように割り当てられるから、
  //     x = gl_VertexID >> 1      = 0, 0, 1, 1, 2, 2, 3, 3, ...
  //     y = 1 - (gl_VertexID & 1) = 1, 0, 1, 0, 1, 0, 1, 0, ...
  //   のように GL_TRIANGLE_STRIP 向けの頂点座標値が得られる。
  //   y に gl_InstaceID を足せば glDrawArrayInstanced() のインスタンスごとに y が変化する。
  //   これに格子の間隔 gap をかけて 1 を引けば縦横 [-1, 1] の範囲の点群 position が得られる。
  int x = gl_VertexID >> 1;
  int y = gl_InstanceID + 1 - (gl_VertexID & 1);
  vec2 position = vec2(x, y) * gap - 1.0;

  // 頂点位置をそのままラスタライザに送ればクリッピング空間全面に描く
  gl_Position = vec4(position, 0.0, 1.0);

  // 視線ベクトル
  //   position にスクリーンの大きさ screen.st をかけて中心位置 screen.pq を足せば、
  //   スクリーン上の点の位置 p が得られるから、原点にある視点からこの点に向かう視線は、
  //   焦点距離 focal を Z 座標に用いて (p, -focal) となる。
  //   これを回転したあと正規化して、その方向の視線単位ベクトルを得る。
  vec2 p = position * screen.st + screen.pq;
  vec4 vector = normalize(rotation * vec4(p, -focal, 0.0));

  // テクスチャ座標 (vector.z の代わりに scale の符号を反転している)
  texcoord = vector.xy * scale / vector.z + center;
}
