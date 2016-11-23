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
  // コピーコンストラクタを封じる
  Camera(const Camera &c);

  // 代入を封じる
  Camera &operator=(const Camera &w);

protected:

  // キャプチャした画像
  GLubyte *buffer;

  // キャプチャした画像の幅と高さ
  GLsizei width, height;

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

  // コンストラクタ
  Camera()
  {
    // 画像がまだ取得されていないことを記録しておく
    buffer = nullptr;

    // スレッドが停止状態であることを記録しておく
    run = false;
  }

  // デストラクタ
  virtual ~Camera()
  {
  }

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
      // キャプチャデバイスをロックする
      mtx.lock();

      // キャプチャスレッドのループを止めて
      run = false;

      // ロックを解除し
      mtx.unlock();

      // 合流する
      thr.join();
    }
  }

  // 画像の幅を得る
  int getWidth() const
  {
    return width;
  }

  // 画像の高さを得る
  int getHeight() const
  {
    return height;
  }

  // Ovrvision Pro の露出を上げる
  virtual void increaseExposure() {};

  // Ovrvision Pro の露出を下げる
  virtual void decreaseExposure() {};

  // Ovrvision Pro の利得を上げる
  virtual void increaseGain() {};

  // Ovrvision Pro の利得を下げる
  virtual void decreaseGain() {};

  // カメラをロックして画像をテクスチャに転送する
  void transmit()
  {
    // カメラのロックを試みる
    if (mtx.try_lock())
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
};
