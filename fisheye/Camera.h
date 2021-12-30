#pragma once

//
// カメラ関連の処理
//

// 補助プログラム
#include "gg.h"
using namespace gg;

// キャプチャを非同期で行う
#include <thread>
#include <mutex>

//
// カメラ関連の処理を担当するクラス
//
class Camera
{
protected:

  // キャプチャした画像
  GLubyte *buffer;

  // キャプチャした画像の幅と高さ
  GLsizei width, height;

  // ムービーファイルのフレーム数
  double frames;

  // キャプチャした画像のフレーム間隔
  double interval;

  // キャプチャされる画像のフォーマット
  GLenum format;

  // スレッド
  std::thread thr;

  // ミューテックス
  std::mutex mtx;

  // 実行状態
  bool run;

  // フレームをキャプチャする
  virtual void capture() {};

public:

  // ムービーファイルのインポイントとアウトポイント
  double in, out;

  // コンストラクタ
  Camera()
    : buffer(nullptr)
    , width(640), height(480)
    , in(-1.0), out(-1.0), frames(-1.0)
    , interval(10.0), format(GL_BGR)
    , run(false)
  {
  }

  // コピーコンストラクタを封じる
  Camera(const Camera &c) = delete;

  // デストラクタ
  virtual ~Camera()
  {
    stop();
  }

  // 代入を封じる
  Camera &operator=(const Camera &w) = delete;

  // スレッドを起動する
  void start()
  {
    // スレッドが起動状態であることを記録しておく
    run = true;

    // スレッドを起動する
    thr = std::thread([this](){ this->capture(); });
  }

  // スレッドを停止する
  void stop()
  {
    // キャプチャスレッドが実行中なら
    if (run)
    {
      // キャプチャデバイスをロックして
      mtx.lock();

      // キャプチャスレッドのループを止めて
      run = false;

      // データの転送が完了したことにして
      buffer = nullptr;

      // ロックを解除し
      mtx.unlock();

      // 合流する
      thr.join();
    }
  }

  // カメラをロックして画像をテクスチャに転送する
  void transmit()
  {
    // カメラのロックを試みる
    if (run && mtx.try_lock())
    {
      // 新しいデータが到着していたら
      if (buffer)
      {
        // データをテクスチャに転送する
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, buffer);

        // データの転送完了を記録する
        buffer = nullptr;
      }

      // 左カメラのロックを解除する
      mtx.unlock();
    }
  }

  // キャプチャ中なら true
  const bool &running() const
  {
    return run;
  }

  // 画像の幅を得る
  const int &getWidth() const
  {
    return width;
  }

  // 画像の高さを得る
  const int &getHeight() const
  {
    return height;
  }

  // ムービーファイルのフレーム数を取得する
  const double &getFrames() const
  {
    return frames;
  }

  // 画像のフレームレートを得る
  double getFps() const
  {
    return 1000.0 / interval;
  }

  // 露出を上げる
  virtual void increaseExposure() {}

  // 露出を下げる
  virtual void decreaseExposure() {}

  // 利得を上げる
  virtual void increaseGain() {}

  // 利得を下げる
  virtual void decreaseGain() {}
};
