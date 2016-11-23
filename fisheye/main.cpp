// ウィンドウ関連の処理
#include "Window.h"

// 平面展開の設定一覧
#include "ExpansionShader.h"

// OpenCV によるビデオキャプチャ
#include "CamCv.h"

//
// 設定
//

// 背景画像の取得に使用するデバイス
//#define CAPTURE_INPUT 0               // 0 番のキャプチャデバイスから入力
//#define CAPTURE_INPUT "sp360.mp4"     // Kodak SP360 4K の Fish Eye 画像
#define CAPTURE_INPUT "theta.mp4"     // THETA S の Equirectangular 画像

// 背景画像を展開する手法 (ExpansionShader.h 参照)
//constexpr int shader_selection(6);    // Kodak SP360 4K
//constexpr int shader_selection(7);    // THETA S の Dual Fisheye 画像
constexpr int shader_selection(2);    // THETA S の Equirectangular 画像

// 背景画像の展開に使用するバーテックスシェーダのソースファイル名
const char *const capture_vsrc(shader_type[shader_selection].vsrc);

// 背景画像の展開に使用するフラグメントシェーダのソースファイル名
const char *const capture_fsrc(shader_type[shader_selection].fsrc);

// 背景画像の取得に使用するカメラの解像度 (0 ならカメラから取得)
const int capture_width(shader_type[shader_selection].width);
const int capture_height(shader_type[shader_selection].height);

// 背景画像の取得に使用するカメラのフレームレート (0 ならカメラから取得)
constexpr int capture_fps(0);

// 背景画像の関心領域
const float *const capture_circle(shader_type[shader_selection].circle);

// 背景画像の描画に用いるメッシュの格子点数
constexpr int screen_samples(1271);

// 背景色は表示されないが合成時に 0 にしておく必要がある
constexpr GLfloat background[] = { 0.0f, 0.0f, 0.0f, 0.0f };

//
// メイン
//

int main()
{
  // カメラの使用を開始する
  CamCv camera;
  if (!camera.open(CAPTURE_INPUT, capture_width, capture_height, capture_fps))
  {
    std::cerr << "Can't open capture device.\n";
    return EXIT_FAILURE;
  }
  camera.start();

  // ウィンドウを作成する
  Window window;

  // ウィンドウが開けたかどうか確かめる
  if (!window.get())
  {
    // ウィンドウが開けなかった
    std::cerr << "Can't open GLFW window.\n";
    return EXIT_FAILURE;
  }

  // 背景描画用のシェーダプログラムを読み込む
  const GLuint expansion(ggLoadShader(capture_vsrc, capture_fsrc));
  if (!expansion)
  {
    // シェーダが読み込めなかった
    std::cerr << "Can't create program object.\n";
    return EXIT_FAILURE;
  }

  // uniform 変数の場所を指定する
  const GLuint gapLoc(glGetUniformLocation(expansion, "gap"));
  const GLuint screenLoc(glGetUniformLocation(expansion, "screen"));
  const GLuint focalLoc(glGetUniformLocation(expansion, "focal"));
  const GLuint rotationLoc(glGetUniformLocation(expansion, "rotation"));
  const GLuint circleLoc(glGetUniformLocation(expansion, "circle"));
  const GLuint imageLoc(glGetUniformLocation(expansion, "image"));

  // 背景用のテクスチャを作成する
  //   ポリゴンでビューポート全体を埋めるので背景は表示されない。
  //   GL_CLAMP_TO_BORDER にしておけばテクスチャの外が GL_TEXTURE_BORDER_COLOR になるので、これが背景色になる。
  const GLuint image([]() { GLuint image; glGenTextures(1, &image); return image; } ());
  glBindTexture(GL_TEXTURE_2D, image);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, camera.getWidth(), camera.getHeight(), 0, GL_BGR, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, background);

  // 背景描画のためのメッシュを作成する
  //   頂点座標値を vertex shader で生成するので VBO は必要ない
  const GLuint mesh([]() { GLuint mesh; glGenVertexArrays(1, &mesh); return mesh; } ());

  // 隠面消去を設定する
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  // ウィンドウが開いている間繰り返す
  while (!window.shouldClose())
  {
    // 背景画像の展開に用いるシェーダプログラムの使用を開始する
    glUseProgram(expansion);

    // スクリーンの矩形の格子点数
    //   標本点の数 (頂点数) n = x * y とするとき、これにアスペクト比 a = x / y をかければ、
    //   a * n = x * x となるから x = sqrt(a * n), y = n / x; で求められる。
    //   この方法は頂点属性を持っていないので実行中に標本点の数やアスペクト比の変更が容易。
    const GLsizei slices(static_cast<GLsizei>(sqrt(window.getAspect() * screen_samples)));
    const GLsizei stacks(screen_samples / slices - 1); // 描画するインスタンスの数なので先に 1 を引いておく。

    // スクリーンの格子間隔
    //   クリッピング空間全体を埋める四角形は [-1, 1] の範囲すなわち縦横 2 の大きさだから、
    //   それを縦横の (格子数 - 1) で割って格子の間隔を求める。
    glUniform2f(gapLoc, 2.0f / (slices - 1), 2.0f / stacks);

    // スクリーンのサイズと中心位置
    //   screen[0] = (right - left) / 2
    //   screen[1] = (top - bottom) / 2
    //   screen[2] = (right + left) / 2
    //   screen[3] = (top + bottom) / 2
    const GLfloat screen[] = { window.getAspect(), 1.0f, 0.0f, 0.0f };
    glUniform4fv(screenLoc, 1, screen);

    // スクリーンまでの焦点距離
    //   window.getWheel() は [-100, 49] の範囲を返す。
    //   したがって焦点距離 focal は [1 / 3, 1] の範囲になる。
    //   これは焦点距離が長くなるにしたがって変化が大きくなる。
    glUniform1f(focalLoc, -50.0f / (window.getWheel() - 50.0f));

    // 背景に対する視線の回転行列
    glUniformMatrix4fv(rotationLoc, 1, GL_TRUE, window.getLeftTrackball().get());

    // テクスチャの半径と中心位置
    //   circle[0] = イメージサークルの x 方向の半径
    //   circle[1] = イメージサークルの y 方向の半径
    //   circle[2] = イメージサークルの中心の x 座標
    //   circle[3] = イメージサークルの中心の y 座標
    const GLfloat circle[] =
    {
      capture_circle[0] + window.getShiftWheel() * 0.001f,
      capture_circle[1] + window.getShiftWheel() * 0.001f,
      capture_circle[2] + (window.getShiftArrowX() - window.getControlArrowX()) * 0.001f,
      capture_circle[3] + (window.getShiftArrowY() + window.getControlArrowY()) * 0.001f
    };
    glUniform4fv(circleLoc, 1, circle);

    // キャプチャした画像を背景用のテクスチャに転送する
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, image);
    camera.transmit();

    // テクスチャユニットを指定する
    glUniform1i(imageLoc, 0);

    // メッシュを描画する
    glBindVertexArray(mesh);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, slices * 2, stacks);

    // カラーバッファを入れ替えてイベントを取り出す
    window.swapBuffers();
  }
}
