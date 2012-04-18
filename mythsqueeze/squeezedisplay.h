#ifndef SQUEEZEDISPLAY_H
#define SQUEEZEDISPLAY_H

#include <QLabel>
#include <QFont>
#include <QFontMetrics>
#include <QRect>
#include <QSize>
#include <QPoint>
#include <QRegion>
#include <QTimeLine>
#include <QPainter>
#include <QBrush>
#include <QColor>
#include <QTimer>
#include <QResizeEvent>

#include "slimdevice.h"
#include "squeezedefines.h"

class SqueezeDisplay : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QColor textcolorGeneral READ getTextColor WRITE setTextColor)
    Q_PROPERTY(QColor displayBackgroundColor READ getDisplayBackgroundColor WRITE setDisplayBackgroundColor)
    Q_PROPERTY(int scrollSpeed READ getScrollSpeed WRITE setScrollSpeed)
    Q_PROPERTY(int scrollInterval READ getScrollInterval WRITE setScrollInterval)
    Q_PROPERTY(int Brightness READ getBrightness WRITE setBrightness)

public:
    explicit SqueezeDisplay(QLabel *lbl, QObject *parent = 0);
    ~SqueezeDisplay(void);

    void Init(QColor txtcolGen, QColor dispBgrdColor);
    void Init(void);

    void SetActiveDevice(SlimDevice *ad) {activeDevice=ad;}

    void setTextColor(QColor c) {textcolorLine1 = m_textcolorGeneral = c;}
    QColor getTextColor() const {return m_textcolorGeneral;}
    void setDisplayBackgroundColor(QColor c) { m_displayBackgroundColor=c; }
    QColor getDisplayBackgroundColor() const {return m_displayBackgroundColor; }
    void setScrollSpeed(int s) {m_scrollSpeed=s;}
    int getScrollSpeed() const {return m_scrollSpeed;}
    void setScrollInterval(int s) {m_scrollInterval=s;}
    int getScrollInterval() const {return m_scrollInterval;}
    void setBrightness(int b) { m_Brightness=b; }
    int getBrightness() const {return m_Brightness;}

    void resetDimensions(void);
    void LeftArrowEffect(void);
    void RightArrowEffect(void);
    void UpArrowEffect(void);
    void DownArrowEffect(void);

    bool Slimp3Display( QString txt );

signals:
    void ErrorMsg(QString err);

public slots:
    void PaintSqueezeDisplay(DisplayBuffer *buf);
    void slotUpdateSlimDisplay(void);
    void slotResetSlimDisplay(void);

    void slotUpdateScrollOffset(void);
    void slotUpdateTransition(int frame);
    void slotTransitionFinished(void);

private:
    void StopScroll( void );
    void LoadTransitionBuffer(void);

    // for display of the slim device interface
    QLabel *displayLabel;
    QImage *displayImage;  // use a QImage not a QPixmap so we can use alpha blends
    SlimDevice *activeDevice;   // reference to active device, must be set manually
    DisplayBuffer *d;       // pointer to display buffer received from main program

    QFont small;
    QFont medium;
    QFont large;
    int lineWidth;
    QFontMetrics *line1fm;        // font metrics of line 1 (in "Large" text) NOTE: must be pointer because it has to be intialized
    int Line1FontWidth;     // width of the letter "W" (used in scrolling text)
    int scrollStep;         // distance we travel during each scroll step == to 1/40 of the width of the letter "W" (or 1, if less than 1)

    QColor m_textcolorGeneral;
    QColor textcolorLine1;
    QColor m_displayBackgroundColor;
    int m_Brightness;
    int line1Alpha;   // alpha blending figure for menu fade-in and fade-out

    QRect m_displayRect;          // area in which to display squeeze display text
    QRect line0Bounds;          // Rectangle in which line0 gets displayed
    QRect line1Bounds;          // Rectangle in which line1 gets displayed
    QRect center0Bounds;
    QRect center1Bounds;
    QRegion line0Clipping;      // Clipping region equal to line0Bounds
    QRegion line1Clipping;      // Clipping region equal to line1Bounds
    QRegion fullDisplayClipping;         // Clipping region equal to the full size of the display
    QPoint pointLine1_2;        // follow-on text for scrolling display

    QRect progRect;         // the rounded and outlined displayed rectangle that shows progress
    QRect progFillRect;     // the rounded and filled portion of the displayed rectangle that shows progress
    QRect volRect;          // the rounded and outlined displayed rectangle for showing the volume
    QRect volFillRect;      // the rounded and filled portion of the displayed rectangle for showing the volume
    qreal radius;           // the radius of the rounded display of the above rectangles

    int xOffsetOld;  // horizontal offset for menu transitions
    int yOffsetOld;  // vertical offset for menu transitions
    int xOffsetNew;
    int yOffsetNew;
    int ScrollOffset;   // used in scrolling
    int scrollTextLen;  // used in scrolling
    QRect boundingRect; // the rectangle occupied by the full Line1 text (which may be larger that the display rect, requiring scrolling)

    bool isTransition;    // are we currently transitioning?
    transitionType transitionDirection;  // -1 = left, -2 = down, +1 = right, +2 = down
    int m_scrollSpeed;
    int m_scrollInterval;
    scrollStatus scrollState;
    QTimeLine *transitionTimer;
    QTimeLine *vertTransTimer;
    QTimeLine *horzTransTimer;
    QTimeLine *bumpTransTimer;
    DisplayBuffer transBuffer;

    QTimer scrollTimer; // timer for scrolling of text too long to fit in display
    
};

#endif // SQUEEZEDISPLAY_H
