#include <cvx/viz/image/view.hpp>
#include <cvx/viz/image/tools.hpp>
#include <cvx/viz/image/widget.hpp>

#include "tools_p.hpp"

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>


#include <sstream>
#include <QMdiArea>
#include <QSettings>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QCoreApplication>
#include <QKeyEvent>
#include <QDebug>
#include <QStyle>
#include <QApplication>
#include <QMessageBox>
#include <QToolBar>
#include <QMenu>
#include <QComboBox>
#include <QFileDialog>
#include <QMimeData>

using namespace std ;

namespace cvx { namespace viz {

QSamplingTool *QImageView::addSampleTool() {
    sampleToolAct = new QAction(QIcon(":/images/eye-drop.png"), tr("Sample Tool"), this);
    sampleToolAct->setStatusTip(tr("View pixel values under cursor"));
    connect(sampleToolAct, SIGNAL(triggered()), this, SLOT(setTool()));
    sampleToolAct->setCheckable(true) ;

    toolGroupAct->addAction(sampleToolAct) ;
    imageToolBar->addAction(sampleToolAct) ;

    QSamplingTool *tool = new QSamplingTool(this) ;
    registerTool(sampleToolAct, tool) ;
    return tool ;
}

QRectTool *QImageView::addRectTool() {
    rectToolAct = new QAction(QIcon(":/images/rect-tool.png"), tr("Rectangle Tool"), this);
    rectToolAct->setStatusTip(tr("Select a rectangular region"));
    connect(rectToolAct, SIGNAL(triggered()), this, SLOT(setTool()));
    rectToolAct->setCheckable(true) ;

    toolGroupAct->addAction(rectToolAct) ;
    imageToolBar->addAction(rectToolAct) ;

    QRectTool *tool = new QRectTool(this) ;
    registerTool(rectToolAct, tool) ;
    return tool ;
}

QPolygonTool *QImageView::addPolygonTool() {
    polyToolAct = new QAction(QIcon(":/images/polygon-tool.png"), tr("Polygon Tool"), this);
    polyToolAct->setStatusTip(tr("Select points"));
    connect(polyToolAct, SIGNAL(triggered()), this, SLOT(setTool()));
    polyToolAct->setCheckable(true) ;

    toolGroupAct->addAction(polyToolAct) ;
    imageToolBar->addAction(polyToolAct) ;

    QPolygonTool *tool = new QPolygonTool(this) ;
    registerTool(polyToolAct, tool) ;
    return tool ;
}

void QImageView::onImageLoaded() {
    emit imageLoaded(pathName) ;
}


void QImageView::createActions()
{
    zoomInAct = new QAction(QIcon(":/images/zoom-in.png"), tr("Zoom In"), this);
    zoomInAct->setShortcut(tr("Ctrl++"));
    zoomInAct->setStatusTip(tr("Zoom In"));
    connect(zoomInAct, SIGNAL(triggered()), this, SLOT(setTool()));
    zoomInAct->setCheckable(true);

    zoomOutAct = new QAction(QIcon(":/images/zoom-out.png"), tr("Zoom Out"), this);
    zoomOutAct->setShortcut(tr("Ctrl+-"));
    zoomOutAct->setStatusTip(tr("Zoom Out"));
    connect(zoomOutAct, SIGNAL(triggered()), this, SLOT(setTool()));
    zoomOutAct->setCheckable(true);

    zoomFitAct = new QAction(QIcon(":/images/zoom-fit.png"), tr("Zoom to Fit"), this);
    zoomFitAct->setStatusTip(tr("Zoom to Fit"));
    connect(zoomFitAct, SIGNAL(triggered()), this, SLOT(zoomFit()));

    zoomRectAct = new QAction(QIcon(":/images/zoom-rect.png"), tr("Zoom to Rectangle"), this);
    zoomRectAct->setStatusTip(tr("Zoom to Rectangle"));
    connect(zoomRectAct, SIGNAL(triggered()), this, SLOT(setTool()));
    zoomRectAct->setCheckable(true);

    panToolAct = new QAction(QIcon(":/images/pan-tool.png"), tr("Pan view"), this);
    panToolAct->setStatusTip(tr("Pan image view"));
    connect(panToolAct, SIGNAL(triggered()), this, SLOT(setTool()));
    panToolAct->setCheckable(true) ;

    toolGroupAct = new QActionGroup(this) ;

    toolGroupAct->addAction(zoomInAct) ;
    toolGroupAct->addAction(zoomOutAct) ;
    toolGroupAct->addAction(zoomRectAct) ;
    toolGroupAct->addAction(zoomFitAct) ;
    toolGroupAct->addAction(panToolAct) ;

    QMenu *zoomMenu = new QMenu() ;
    zoomMenu->addAction(zoomInAct) ;
    zoomMenu->addAction(zoomOutAct) ;
    zoomMenu->addAction(zoomRectAct) ;
}

void QImageView::createToolBar()
{
    imageToolBar = new QToolBar("Image") ;

    imageToolBar->setObjectName("ImageTB") ;

    imageToolBar->addAction(zoomFitAct) ;
    imageToolBar->addSeparator() ;
    zoomCombo = new QComboBox(imageToolBar) ;
    zoomCombo->setFocusPolicy(Qt::ClickFocus) ;

    zoomCombo->addItem("100%") ;
    zoomCombo->addItem("To Fit") ;
    zoomCombo->addItem("2%") ;
    zoomCombo->addItem("5%") ;
    zoomCombo->addItem("10%") ;
    zoomCombo->addItem("25%") ;
    zoomCombo->addItem("33%") ;
    zoomCombo->addItem("50%") ;
    zoomCombo->addItem("75%") ;
    zoomCombo->addItem("100%") ;
    zoomCombo->addItem("150%") ;
    zoomCombo->addItem("x2") ;
    zoomCombo->addItem("x3") ;
    zoomCombo->addItem("x4") ;
    zoomCombo->addItem("x5") ;
    zoomCombo->addItem("x6") ;
    zoomCombo->addItem("x7") ;
    zoomCombo->addItem("x8") ;
    zoomCombo->addItem("x9") ;
    zoomCombo->addItem("x10") ;
    zoomCombo->addItem("x11") ;
    zoomCombo->addItem("x12") ;
    zoomCombo->addItem("x13") ;
    zoomCombo->addItem("x14") ;
    zoomCombo->addItem("x15") ;
    zoomCombo->addItem("x16") ;
    zoomCombo->setMaxVisibleItems(zoomCombo->count()) ;

    imageToolBar->addWidget(zoomCombo) ;
    connect(zoomCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(setZoom()));

    imageToolBar->addAction(zoomInAct) ;
    imageToolBar->addAction(zoomOutAct) ;
    imageToolBar->addAction(zoomRectAct) ;

    imageToolBar->addSeparator() ;

    imageToolBar->addAction(panToolAct) ;

    panToolAct->setChecked(true) ;

    addToolBar(imageToolBar) ;

}


void QImageView::registerTool(QAction *act, QImageTool *tool)
{
    tool->registerWithView(pWidget) ;
    tools[act] = tool ;
}


void QImageView::createTools()
{
    registerTool(panToolAct, new QPanTool(this)) ;
    registerTool(zoomInAct, new QZoomInTool(this)) ;
    registerTool(zoomOutAct, new QZoomOutTool(this)) ;
    registerTool(zoomRectAct, new QZoomRectTool(this)) ;

}

QImageView::QImageView(QWidget *parent): QMainWindow(parent)
{

    pWidget = new QImageWidget(this) ;
    pWidget->setMinimumSize(200, 200) ;

    setCentralWidget(pWidget);

    createActions() ;
    createTools() ;
    createToolBar() ;

    installEventFilter(this) ;

    connect(pWidget->scene(), SIGNAL(fileDropped(const QString &)), this, SLOT(load(const QString &))) ;

    connect(pWidget, SIGNAL(zoomChanged(int)), this, SLOT(updateZoomCombo(int))) ;

    idx = -1 ;
}

cv::Mat QImageView::image() const {
    return pWidget->image() ;
}

void QImageView::setFilePaths(const QStringList &list)
{
    entries_ = list ;
}

void QImageView::setImage(const cv::Mat &im, const QString &name)
{
    pWidget->setImage(im) ;
    setFileName(name) ;
}


void QImageView::setImage(const QImage &img) {
    pWidget->setImage(img) ;
}

bool QImageView::hasImage() const { return pWidget->hasImage() ; }


void QImageView::fitToWindow()
{
    pWidget->zoomToRect(QRectF(0, 0, pWidget->image().cols, pWidget->image().rows)) ;
    updateZoomCombo(pWidget->getZoom()) ;
}

void QImageView::fitToRect(const QRect &rect)
{
    pWidget->zoomToRect(rect) ;
    updateZoomCombo(pWidget->getZoom()) ;
}


void QImageView::setTool()
{
    QImageTool *tool = tools.value(qobject_cast<QAction *>(sender()), nullptr) ;

    if ( tool ) pWidget->setTool(tool) ;
}

void QImageView::updateZoomCombo(int idx)
{
    if ( idx == 7 ) zoomCombo->setCurrentIndex(0) ;
    else zoomCombo->setCurrentIndex(idx + 2) ;
}

void QImageView::setZoom()
{
    int idx = zoomCombo->currentIndex() ;

    if ( idx == 0 ) pWidget->setZoom(7) ;
    else if ( idx == 1 ) fitToWindow() ;
    else pWidget->setZoom(idx-2) ;

}

void QImageView::zoomIn()
{
    pWidget->zoomRel(1) ;

}

void QImageView::zoomOut()
{
    pWidget->zoomRel(-1) ;
}

void QImageView::zoomRect()
{

}

void QImageView::zoomFit()
{
    fitToWindow() ;

}

bool QImageView::eventFilter(QObject *o, QEvent *e)
{

    if ( o == this && e->type() == QEvent::KeyRelease )
    {
        QKeyEvent *kpe = (QKeyEvent *)e ;

        if ( kpe->key() == Qt::Key_PageUp )
        {
            previous() ;
            return true ;
        }
        else if (  kpe->key() == Qt::Key_PageDown )
        {
            next() ;
            return true ;
        }
        else if ( kpe->key() == Qt::Key_Home )
        {
            first() ;
            return true ;
        }
        else if ( kpe->key() == Qt::Key_End )
        {
            last() ;
            return true ;
        }
        else if ( kpe->key() == Qt::Key_T )
        {
            imageToolBar->setVisible(!imageToolBar->isVisible());
            //            fitToContents() ;
            return true ;
        }
        else return false ;
    }


    return QMainWindow::eventFilter(o, e) ;
}

void QImageView::previous()
{
    if ( idx <= 0 ) return ;

    QString path ;

    while ( idx > 0 ) {
        idx -- ;
        path =  entries_[idx] ;
        if ( QFileInfo(path).exists() ) {
            load(path) ;
            break ;
        }
        else
            entries_.removeAt(idx) ;
    }

}

void QImageView::next()
{
    QString path ;
    while ( idx < entries_.size()-1 ) {
        idx ++ ;
        path = entries_[idx] ;
        if ( QFileInfo(path).exists() ) {
            load(path) ;
            break ;
        }
        else
            entries_.removeAt(idx) ;
    }

}

void QImageView::first()
{
    idx = 0 ;

    QString path ;

    while ( idx < entries_.size() )  {
        path = entries_[idx] ;
        if ( QFileInfo(path).exists() ) {
            load(path) ;
            break ;
        }

        entries_.removeAt(idx) ;
    }
}

void QImageView::last()
{

    idx = entries_.size() - 1 ;

    QString path ;

    while ( idx >= 0 ) {
        path = entries_[idx] ;
        if ( QFileInfo(path).exists() ) {
            load(path) ;
            break ;
        }
        else {
            entries_.removeAt(idx) ;
            idx = entries_.size() - 1 ;
        }
    }

}

void QImageView::addTool(QAction *action, QImageTool *tool)
{
    connect(action, &QAction::triggered, this, &QImageView::setTool);
    action->setCheckable(true) ;

    registerTool(action, tool) ;
    toolGroupAct->addAction(action) ;
    imageToolBar->addAction(action) ;

}




bool QImageView::load(const QString &fname)
{
    cv::Mat img ;

    QApplication::setOverrideCursor(Qt::WaitCursor);

    img = cv::imread((const char *)fname.toUtf8(), -1) ;

    QApplication::restoreOverrideCursor();

    if ( img.data == NULL ) {
        QMessageBox::warning(this, "Image Load Failed",
                             QString("Error loading \"%1\"").arg(QFileInfo(fname).fileName())) ;

        return false ;
    }

    QApplication::restoreOverrideCursor();

    pathName = QFileInfo(fname).canonicalFilePath()  ;

    pWidget->setImage(img) ;

    onImageLoaded() ;

    return true ;
}

QString QImageView::strippedName(const QString &fullFileName)
{
    return QFileInfo(pathName).fileName();
}

bool QImageView::save()
{
    return saveFile(pathName) ;
}

bool QImageView::saveAs()
{
    QFileDialog dlg(this) ;

    QFileInfo finfo(fileName()) ;

    QDir dir = finfo.dir() ;

    dlg.setDirectory(dir) ;
    dlg.selectFile(finfo.fileName()) ;

    if (  dlg.exec() )
    {
        QString outName = dlg.selectedFiles()[0] ;

        return saveFile(outName);
    }
    else return false ;
}

bool QImageView::saveFile(const QString &fileNameSave )
{
    QFile file(fileNameSave);

    QApplication::setOverrideCursor(Qt::WaitCursor);

    qreal sw = pWidget->scene()->width() ;
    qreal sh = pWidget->scene()->height() ;

    if ( pWidget->hasImage() )
    {
        const cv::Mat &orig = image() ;

        if ( !cv::imwrite((const char *)fileNameSave.toUtf8(), orig) )
        {
            QMessageBox::critical(this, "Save As ...", QString("Failed to save image: %1").arg(fileNameSave)) ;
            QApplication::restoreOverrideCursor();
            return false ;
        }
    }


    QApplication::restoreOverrideCursor();

    return true;
}

void QImageView::closeEvent(QCloseEvent *e)
{

    QMainWindow::closeEvent(e) ;
}


bool QImageView::copy(QMimeData *data)
{
    if ( pWidget->hasImage() )
    {
        const cv::Mat &im =  image() ;

        data->setImageData(QImageWidget::imageToQImage(im)) ;
    }

    return true ;

}

}
              }

