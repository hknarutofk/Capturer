#include "gifcapturer.h"
#include <QFileDialog>
#include <QKeyEvent>
#include <QDebug>
#include <QDateTime>
#include <QStandardPaths>
#include "detectwidgets.h"
#include "logging.h"

GifCapturer::GifCapturer(QWidget * parent)
    : Selector(parent)
{
    process_ = new QProcess(this);
    record_menu_ = new RecordMenu();

    connect(record_menu_, &RecordMenu::STOP, this, &GifCapturer::exit);
}

void GifCapturer::record()
{
    status_ == INITIAL ? start() : exit();
}

void GifCapturer::setup()
{
    record_menu_->start();

    status_ = LOCKED;
    hide();

    auto native_pictures_path = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    current_time_str_ = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz");

    filename_ = native_pictures_path + QDir::separator() + "Capturer_gif_" + current_time_str_ + ".gif";

    QStringList args;
    auto selected_area = selected();
    auto temp_dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    temp_video_path_ = temp_dir + QDir::separator() + "Capturer_gif_" + current_time_str_ + ".mp4";
    temp_palette_path_ = temp_dir + QDir::separator() + "Capturer_palette_" + current_time_str_ + ".png";

#ifdef __linux__
    QString display = QProcessEnvironment::systemEnvironment().value("DISPLAY");
    args << "-video_size"   << QString("%1x%2").arg(selected_area.width()).arg(selected_area.height())
         << "-framerate"    << QString("%1").arg(framerate_)
         << "-f"            << "x11grab"
         << "-i"            << QString("%1+%2,%3").arg(display).arg(selected_area.x()).arg(selected_area.y())
         << temp_video_path_;    
#elif _WIN32
    args << "-f"            << "gdigrab"
         << "-framerate"    << QString("%1").arg(framerate_)
         << "-offset_x"     << QString("%1").arg(selected_area.x())
         << "-offset_y"     << QString("%1").arg(selected_area.y())
         << "-video_size"   << QString("%1x%2").arg(selected_area.width()).arg(selected_area.height())
         << "-i"            << "desktop"
         << temp_video_path_;
#endif
    LOG(DEBUG) << args;
    process_->start("ffmpeg", args);

}

void GifCapturer::exit()
{
    process_->write("q\n\r");

    bool result = process_->waitForFinished();
    LOG(DEBUG) << result;
    QStringList args;
    args << "-y"
         << "-i"    << temp_video_path_
         << "-vf"   << QString("fps=%1,palettegen").arg(fps_)
         << temp_palette_path_;
    process_->start("ffmpeg", args);
    process_->waitForFinished();

    args.clear();
    args << "-i" << temp_video_path_
         << "-i" << temp_palette_path_
         << "-filter_complex" << QString("fps=%1,paletteuse").arg(fps_)
         << filename_;

    LOG(DEBUG) << args;
    process_->start("ffmpeg", args);

    record_menu_->stop();
    emit SHOW_MESSAGE("Capturer<GIF>", tr("Path: ") + filename_);

    Selector::exit();
}


void GifCapturer::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Escape) {
        Selector::exit();
    }

    if(event->key() == Qt::Key_Return) {
        setup();
    }
}

