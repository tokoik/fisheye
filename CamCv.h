#pragma once

//
// OpenCV を使ったキャプチャ
//

// カメラ関連の処理
#include "Camera.h"

// OpenCV
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#if defined(_WIN32)
#  define CV_VERSION_STR CVAUX_STR(CV_MAJOR_VERSION) CVAUX_STR(CV_MINOR_VERSION) CVAUX_STR(CV_SUBMINOR_VERSION)
#  if defined(_DEBUG)
#    define CV_EXT_STR "d.lib"
#  else
#    define CV_EXT_STR ".lib"
#  endif
// #  pragma comment(lib, "opencv_core" CV_VERSION_STR CV_EXT_STR)
// #  pragma comment(lib, "opencv_highgui" CV_VERSION_STR CV_EXT_STR)
// #  pragma comment(lib, "opencv_videoio" CV_VERSION_STR CV_EXT_STR)
#endif

// OpenCV を使ってキャプチャするクラス
class CamCv
  : public Camera
{
  // OpenCV のキャプチャデバイス
  cv::VideoCapture camera;

  // OpenCV のキャプチャデバイスから取得したフレーム
  cv::Mat frame;

  // 現在のフレームの時刻
  double startTime;

  // 露出と利得
  int exposure, gain;

  // キャプチャデバイスを初期化する
  bool init(int initial_width, int initial_height, int initial_fps, const char *fourcc = "")
  {
    // カメラのコーデック・解像度・フレームレートを設定する
    if (fourcc[0] != '\0') camera.set(cv::CAP_PROP_FOURCC,
      cv::VideoWriter::fourcc(fourcc[0], fourcc[1], fourcc[2], fourcc[3]));
    if (initial_width > 0) camera.set(cv::CAP_PROP_FRAME_WIDTH, initial_width);
    if (initial_height > 0) camera.set(cv::CAP_PROP_FRAME_HEIGHT, initial_height);
    if (initial_fps > 0) camera.set(cv::CAP_PROP_FPS, initial_fps);

    // fps が 0 なら逆数をカメラの遅延に使う
    const double fps(camera.get(cv::CAP_PROP_FPS));
    if (fps > 0.0) interval = 1000.0 / fps;

    // ムービーファイルのインポイント・アウトポイントの初期値とフレーム数
    in = camera.get(cv::CAP_PROP_POS_FRAMES);
    out = frames = camera.get(cv::CAP_PROP_FRAME_COUNT);

    // 開始時刻
    startTime = glfwGetTime() * 1000.0;

    // カメラから最初のフレームをキャプチャできなかったらカメラは使えない
    if (!camera.grab()) return false;

    // キャプチャしたフレームのサイズを取得する
    width = static_cast<GLsizei>(camera.get(cv::CAP_PROP_FRAME_WIDTH));
    height = static_cast<GLsizei>(camera.get(cv::CAP_PROP_FRAME_HEIGHT));

    // macOS だと設定できても 0 が返ってくる
    if (width == 0) width = initial_width;
    if (height == 0) height = initial_height;

#if defined(DEBUG)
    char codec[5] = { 0, 0, 0, 0, 0 };
    getCodec(codec);
    std::cerr << "in:" << in << ", out:" << out
      << ", width:" << width << ", height:" << height
      << ", fourcc: " << codec << "\n";
#endif

    // カメラの利得と露出を取得する
    gain = static_cast<GLsizei>(camera.get(cv::CAP_PROP_GAIN));
    exposure = static_cast<GLsizei>(camera.get(cv::CAP_PROP_EXPOSURE) * 10.0);

    // キャプチャされるフレームのフォーマットを設定する
    format = GL_BGR;

    // フレームを取り出してキャプチャ用のメモリを確保する
    camera.retrieve(frame, 3);

    // フレームがキャプチャされたことを記録する
    buffer = frame.data;

    // カメラが使える
    return true;
  }

  // フレームをキャプチャする
  virtual void capture()
  {
    // スレッドが実行可の間
    while (run)
    {
      // フレームを取り出せたら true
      bool status(false);

      // ムービーファイルでないかムービーファイルの終端でなければ次のフレームを取り出して
      if ((frames <= 0.0 || camera.get(cv::CAP_PROP_POS_FRAMES) < out) && camera.grab())
      {
        // キャプチャデバイスをロックして
        mtx.lock();

        // 到着しているフレームを切り出して
        camera.retrieve(frame, 3);

        // フレームを更新し
        buffer = frame.data;

        // ロックを解除する
        mtx.unlock();

        // フレームが取り出せた
        status = true;
      }

      // 現在時刻
      const double now(glfwGetTime() * 1000.0);

      // 遅延時間
      double deferred(startTime + interval - now);

      // ムービーファイルから入力しているとき
      if (frames > 0.0)
      {
        // ムービーファイルの終端に到達していたら
        if (!status)
        {
          // インポイントまで巻き戻す
          camera.set(cv::CAP_PROP_POS_FRAMES, in);

          // 巻き戻した後のフレーム時刻
          const double pos(in * interval);

          // インポイントを現在時刻としたときの開始時刻を求める
          startTime = now - pos;

          // インポイントでは遅延させない
          deferred = 0.0;
        }
        else
        {
          // 現在のフレーム時刻
          const double pos(camera.get(cv::CAP_PROP_POS_MSEC));

          // 開始時刻から見た現在のフレームの遅延を求める
          deferred += pos;
        }
      }
      else
      {
        // ムービーファイルでなければ現在の時刻を開始時刻にする
        startTime = now;
      }

      // 遅延時間あれば
      if (deferred > 0.0)
      {
        // 待つ
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(deferred)));
      }
    }
  }

public:

  // コンストラクタ
  CamCv()
    : exposure(0), gain(0), startTime(0)
  {}

  // デストラクタ
  virtual ~CamCv()
  {
    // スレッドを停止する
    stop();

    // カメラを閉じる
    close();
  }

  // カメラが開けたかどうか調べる
  bool isOpened() const
  {
    return camera.isOpened();
  }

  // カメラから入力する
  bool open(int device, int width = 0, int height = 0, int fps = 0, const char *fourcc = "", int pref = cv::CAP_ANY)
  {
    // カメラを開く
    camera.open(device, pref);

    // カメラが使えればカメラを初期化する
    if (isOpened() && init(width, height, fps, fourcc)) return true;

    // カメラが使えない
    return false;
  }

  // ファイル／ネットワークから入力する
  bool open(const std::string &file, int width = 0, int height = 0, int fps = 0, const char *fourcc = "", int pref = cv::CAP_ANY)
  {
    // ファイル／ネットワークを開く
    camera.open(file, pref);

    // ファイル／ネットワークが使えれば初期化する
    if (isOpened() && init(width, height, fps, fourcc)) return true;

    // ファイル／ネットワークが使えない
    return false;
  }

  // カメラを閉じる
  void close()
  {
    camera.release();
  }

  // コーデックを調べる
  int getCodec() const
  {
    return static_cast<int>(camera.get(cv::CAP_PROP_FOURCC));
  }

  // コーデックを調べる
  void getCodec(char *fourcc) const
  {
    int cc(getCodec());
    for (int i = 0; i < 4; ++i)
    {
      fourcc[i] = static_cast<char>(cc & 0x7f);
      if (!isalnum(fourcc[i])) fourcc[i] = '?';
      cc >>= 8;
    }
  }

  // 現在のフレーム番号を得る
  double getPosition() const
  {
    return camera.get(cv::CAP_PROP_POS_FRAMES);
  }

  // 再生位置を指定する
  void setPosition(double frame)
  {
    camera.set(cv::CAP_PROP_POS_FRAMES, frame);
  }

  // 露出を設定する
  virtual void setExposure(int gain)
  {
    if (isOpened()) camera.set(cv::CAP_PROP_EXPOSURE, exposure * 0.1);
  }

  // 露出を上げる
  virtual void increaseExposure()
  {
    setExposure(++exposure);
  }

  // 露出を下げる
  virtual void decreaseExposure()
  {
    setExposure(--exposure);
  }

  // 利得を設定する
  virtual void setGain(int gain)
  {
    if (isOpened()) camera.set(cv::CAP_PROP_GAIN, gain);
  }

  // 利得を上げる
  virtual void increaseGain()
  {
    setGain(++gain);
  }

  // 利得を下げる
  virtual void decreaseGain()
  {
    setGain(--gain);
  }
};
