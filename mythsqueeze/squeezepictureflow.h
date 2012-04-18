#ifndef SQUEEZEPICTUREFLOW_H
#define SQUEEZEPICTUREFLOW_H

#include <QPaintEvent>
#include <QPainter>
#include <QList>
#include <QListIterator>
#include <QHash>
#include <QHashIterator>
#include <QStringList>
#include <QColor>
#include <QWidget>

#include "pictureflow.h"
#include "squeezedefines.h"
#include "slimimagecache2.h"

class SqueezePictureFlow : public PictureFlow
{
    Q_OBJECT
public:
    explicit SqueezePictureFlow(QWidget* parent, picflowType flags = (picflowType)(AUTOSELECTON | TRACKSELECT));
    ~SqueezePictureFlow();

    void LoadAlbumList(QList<Album> list);
    void LoadAlbumList(QList<TrackData> list);
    QList<Album> *GetAlbumList(void) {return &albumList; }
    QHash<QString,int> *GetAlbumKeyTextJumpList(void) {return &m_albumKeyTextJumpList;}
    QHash<QString,int> *GetTitleJumpList(void) {return &m_titleJumpList;}

    void setBackgroundColor(const QColor& c);
    void clear();
    void resetDimensions(QWidget *win);

    bool IsReady(void) { return isReady; }

    picflowType GetFlags(void) {return m_Flags;}
    Album GetCenterAlbum(void) {return albumList.at(centerIndex());}

    void JumpTo(QString textkey);

signals:
    void NextSlide();
    void PrevSlide();
    void SelectSlide(int);
    void CoverFlowReady(void);

public slots:
    void FetchCovers(void);
    void GoToNextSlide(void) {showNext();}
    void GoToPrevSlide(void) {showPrevious();}
    void FlipToSlide(int slide);

protected:
    //  void keyPressEvent(QKeyEvent* event);
    void mousePressEvent(QMouseEvent* event);
//    void mouseReleaseEvent(QMouseEvent *event);
    void paintEvent (QPaintEvent *e);
//    void resizeEvent(QResizeEvent *event);

private:
    QList<Album> albumList;
    QHash<QString,int> m_titleJumpList;
    QHash<QString,int> m_albumKeyTextJumpList;     //
    QColor titleColor;
    picflowType m_Flags;
    bool isReady;
};

#endif // SQUEEZEPICTUREFLOW_H
