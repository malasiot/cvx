#ifndef RGBD_VIEWER_HPP
#define RGBD_VIEWER_HPP

#include <cvx/viz/image/widget.hpp>
#include <QKeyEvent>

class RGBDWidget: public cvx::viz::QImageWidget {
    Q_OBJECT

public:
    RGBDWidget(QWidget *parent): cvx::viz::QImageWidget(parent) {
        installEventFilter(this);
    }

    bool eventFilter(QObject *obj, QEvent *event)   {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if ( keyEvent->key() == Qt::Key_1) {
                channel_ = 1 ;
                return true ;
            } else if ( keyEvent->key() == Qt::Key_2 ) {
                channel_ = 2 ;
                return true ;
            }
        }
        return QObject::eventFilter(obj, event);
    }

    int getChannel() const {
        return channel_ ;
    }
signals:

    void updateImage(const cv::Mat &) ;

private:
    QAtomicInt channel_ = 1 ;
};


Q_DECLARE_METATYPE(cv::Mat)

#endif
