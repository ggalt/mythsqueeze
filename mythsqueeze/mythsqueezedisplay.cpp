#include <QtDebug>

#include "mythsqueezedisplay.h"

#ifdef SQUEEZEDISPLAY_DEBUG
#define DEBUGF(...) qDebug() << this->objectName() << Q_FUNC_INFO << __VA_ARGS__;
#else
#define DEBUGF(...)
#endif
#define HPADDING 2
#define WPADDING 5

MythSqueezeDisplay::MythSqueezeDisplay( MythUIVideo *squeezeDisplay, QObject *parent) :
    QObject(parent)
{
    setObjectName("SqueezeDisplay");
    vertTransTimer = new QTimeLine( 350, this );
    horzTransTimer = new QTimeLine( 700, this );
    bumpTransTimer = new QTimeLine( 100, this );
    vertTransTimer->setFrameRange( 0, 0 );
    horzTransTimer->setFrameRange( 0, 0 );
    bumpTransTimer->setFrameRange( 0, 0 );
    isTransition = false;
    transitionDirection = transNONE;
    xOffsetOld = xOffsetNew = 0;
    yOffsetOld = yOffsetNew = 0;
    m_scrollSpeed = 33;
    m_scrollInterval = 5000;
    ScrollOffset = 0;
    scrollState = NOSCROLL;
    m_Brightness = 255;
    line1Alpha = 0;
    line1fm=NULL;
    displayImage=NULL;
    m_squeezeDisplay = squeezeDisplay;
    displayLabel = m_squeezeDisplay->GetArea().toQRect();
}

MythSqueezeDisplay::~MythSqueezeDisplay(void)
{
    if(line1fm!=NULL)
        delete line1fm;
    if(vertTransTimer!=NULL)
        delete vertTransTimer;
    if(horzTransTimer!=NULL)
        delete horzTransTimer;
    if(bumpTransTimer!=NULL)
        delete bumpTransTimer;
    if(!displayImage->isNull())
        delete displayImage;
}

void MythSqueezeDisplay::Init(QColor txtcolGen, QColor dispBgrdColor)
{
    textcolorLine1 = m_textcolorGeneral = txtcolGen;
    m_displayBackgroundColor = dispBgrdColor;
    Init();
}

void MythSqueezeDisplay::Init(void)
{
    resetDimensions();
    connect( &scrollTimer, SIGNAL( timeout() ), this, SLOT(slotUpdateScrollOffset()) );
    connect( vertTransTimer, SIGNAL(frameChanged(int)), this, SLOT(slotUpdateTransition(int)));
    connect( horzTransTimer, SIGNAL(frameChanged(int)), this, SLOT(slotUpdateTransition(int)));
    connect( bumpTransTimer, SIGNAL(frameChanged(int)), this, SLOT(slotUpdateTransition(int)));
    connect( vertTransTimer, SIGNAL(finished()), this, SLOT(slotTransitionFinished()));
    connect( horzTransTimer, SIGNAL(finished()), this, SLOT(slotTransitionFinished()));
    connect( bumpTransTimer, SIGNAL(finished()), this, SLOT(slotTransitionFinished()));
}

void MythSqueezeDisplay::resetDimensions(void)
{
    /*  Display is divided into a small top line (line0) and a large bottom line (line1)
        If Line0 = A, Line1 = 3A, with the gap at the top of Line0 and the bottom of Line1
        being 0.25A and the gap between Line0 and Line1 being .5A.  A = total height of the
        display rectangle divided by 5.  The display rectangle is always a ratio of 1X10, which
        corresponds to the original SqueezeBox 3 display of 32X320.
      */

    // first establish display parameters for entire display area
    // Leave a blank area all around

    int drawwidth = displayLabel.width()-2*WPADDING;
    int drawheight = displayLabel.height()-2*HPADDING;

    // we want a ratio of at least a 1 X 10 for the display
    if(drawwidth/drawheight >= 10) {  // too wide
        int newWidth = drawheight*10;
        m_displayRect = QRect(WPADDING+((drawwidth-newWidth)/2),HPADDING,newWidth,drawheight);
    } else {
        int newHeight = drawwidth/10;
        m_displayRect = QRect(WPADDING,HPADDING+((drawheight-newHeight)/2),drawwidth,newHeight);
    }

    DEBUGF("Display Rect: " << m_displayRect);
    DEBUGF("Label Rect: " << displayLabel);

    fullDisplayClipping = QRegion(m_displayRect);

    // let's make sure we're not too small, and if we are, artificially enlarge the size
    if(m_displayRect.height() < 32) {   // probably too small to use, so for a size of 320x32
        DEBUGF(QString("original display rectangle is too small, with height of %1, main rect is %2 high").arg(m_displayRect.height()).arg(displayLabel.height()));
        m_displayRect.setHeight(32);
        m_displayRect.setWidth(320);
    }

    // establish size of each display line
    int line0height = m_displayRect.height()/4;
    line0Bounds = QRect(m_displayRect.x(),m_displayRect.y()+line0height/4, m_displayRect.width()-WPADDING,line0height);
    line0Clipping = QRegion(line0Bounds.x(), m_displayRect.y(), line0Bounds.width(), m_displayRect.height()/3 );
    line1Bounds = QRect(m_displayRect.x(),m_displayRect.y()+((line0height*7)/4),m_displayRect.width()-WPADDING,line0height*2);
    line1Clipping = QRegion(line1Bounds.x(), line0Clipping.boundingRect().bottom(),line1Bounds.width(), (2*m_displayRect.height())/3);
    center0Bounds = QRect(m_displayRect.x(),m_displayRect.y()+m_displayRect.height()/9, m_displayRect.width()-WPADDING, m_displayRect.height()/3);
    center1Bounds = QRect(m_displayRect.x(),center0Bounds.bottom()+m_displayRect.height()/9, m_displayRect.width()-WPADDING, m_displayRect.height()/3);

    // establish font
    small.setFamily( "Helvetica" );
    medium.setFamily( "Helvetica" );
    large.setFamily( "Helvetica" );
    small.setPixelSize(4);
    medium.setPixelSize(4);
    large.setPixelSize(4);

    for( int i = 5; QFontInfo(small).pixelSize() < line0Bounds.height(); i++)
        small.setPixelSize(i);

    for(int i=5; QFontInfo(medium).pixelSize() < displayLabel.height()/3; i++)
        medium.setPixelSize(i);

    for(int i = 5; QFontInfo(large).pixelSize() < line1Bounds.height(); i++)
        large.setPixelSize(i);

    // establish information for scrolling
    if(line1fm)
        delete line1fm;
    line1fm = new QFontMetrics(large);
    Line1FontWidth = line1fm->width('W');
    scrollStep = (Line1FontWidth < 40 ? 1 : Line1FontWidth / 40);
    if(line1fm->descent() > line1Clipping.boundingRect().bottom() - line1Bounds.bottom()) {   // make sure the descent isn't outside of the clipping area
        line1Bounds.moveTop(line1Bounds.top()-(line1fm->descent()-(line1Clipping.boundingRect().bottom() - line1Bounds.bottom())));
    }

    vertTransTimer->setFrameRange( 0, line1Bounds.height() );
    horzTransTimer->setFrameRange( 0, m_displayRect.width() );
    bumpTransTimer->setFrameRange( 0, Line1FontWidth );

    // establish volume and time progress bars
    volFillRect = volRect = QRect( m_displayRect.x(), displayLabel.height()/2 - line0Bounds.height()/2, m_displayRect.width()-(2*WPADDING),line0Bounds.height());
    progFillRect = progRect = QRect(0,line0Bounds.top()+ line0Bounds.height()/4,line0Bounds.width(),line0Bounds.height()/2);    // note, "left" and "width" are irrelevant here, only "top" and "height" will remain constant
    radius = volRect.height()/4;

    // establish display image
    if(displayImage) {
        delete displayImage;
    }
    displayImage = new QImage(displayLabel.width(),displayLabel.height(),QImage::Format_ARGB32 );
    displayImage->fill((uint)m_displayBackgroundColor.rgb());
    DEBUGF("Display Image Dimensions" << displayImage->rect());
}

void MythSqueezeDisplay::slotResetSlimDisplay(void)
{
    DEBUGF("");
    scrollTimer.stop();
    ScrollOffset = 0;
    line1Alpha = 0;
    boundingRect = line1fm->boundingRect( line1Bounds, Qt::AlignLeft | Qt::AlignHCenter, activeDevice->getDisplayBuffer()->line1 );
    if( boundingRect.width() > line1Bounds.width() ) {
        scrollState = PAUSE_SCROLL;
        scrollTextLen = boundingRect.width() - line1Bounds.width();
        scrollTimer.setInterval( m_scrollInterval );
        scrollTimer.start();
    }
    else {
        scrollState = NOSCROLL;
    }
    PaintSqueezeDisplay(activeDevice->getDisplayBuffer());
}

void MythSqueezeDisplay::slotUpdateSlimDisplay( void )
{
    DEBUGF("");
    boundingRect = line1fm->boundingRect( line1Bounds, Qt::AlignLeft | Qt::AlignHCenter, activeDevice->getDisplayBuffer()->line1 );
    lineWidth = line1fm->width( activeDevice->getDisplayBuffer()->line1 );

    // set the point at which we draw the second iteration of line1 text
    pointLine1_2 = QPoint( lineWidth + ( displayLabel.width() - line1Bounds.width()) + (Line1FontWidth*2), line1Bounds.bottom() );

    if(  lineWidth > line1Bounds.width() ) {
        if( scrollState == NOSCROLL ) {
            StopScroll();
            scrollTextLen = lineWidth - line1Bounds.width();
            scrollTimer.setInterval( m_scrollInterval );
            scrollTimer.start();
        }
    }
    else {
        ScrollOffset = 0;
        scrollState = NOSCROLL;
        scrollTimer.stop();
    }
    PaintSqueezeDisplay(activeDevice->getDisplayBuffer());
}

void MythSqueezeDisplay::LeftArrowEffect( void )
{
    StopScroll();
    isTransition = true;
    transitionDirection = transRIGHT;
    LoadTransitionBuffer();
    if( activeDevice->getDisplayBuffer()->line0 == "Squeezebox Home" ) {  // are we at the "left-most" menu option?
        bumpTransTimer->start();
    }
    else {
        horzTransTimer->start();
    }
}

void MythSqueezeDisplay::RightArrowEffect( void )
{
    StopScroll();
    isTransition = true;
    transitionDirection = transLEFT;
    LoadTransitionBuffer();
    horzTransTimer->start();
}

void MythSqueezeDisplay::UpArrowEffect( void )
{
    StopScroll();
    isTransition = true;
    transitionDirection = transDOWN;
    LoadTransitionBuffer();
    vertTransTimer->start();
}

void MythSqueezeDisplay::DownArrowEffect( void )
{
    StopScroll();
    isTransition = true;
    transitionDirection = transUP;
    LoadTransitionBuffer();
    vertTransTimer->start();
}

void MythSqueezeDisplay::StopScroll( void )
{
    scrollTimer.stop();
    ScrollOffset = 0;
    line1Alpha = 0;
    scrollState = PAUSE_SCROLL;
}

void MythSqueezeDisplay::slotUpdateScrollOffset(void)
{
    if( scrollState == PAUSE_SCROLL ) {
        scrollTimer.stop();
        scrollState = SCROLL;
        scrollTimer.setInterval( m_scrollSpeed );
        DEBUGF("SCROLL STARTED AT " << m_scrollSpeed);
        scrollTimer.start();
    }
    else if( scrollState == SCROLL ) {
        //        ScrollOffset += Line1FontWidth;
        ScrollOffset += scrollStep;
        //        if( ScrollOffset >= ( lineWidth - Line1Rect.width()) ) {
        if( ScrollOffset >= lineWidth ) {
            scrollState = FADE_OUT;
            line1Alpha = 0;
        }
    }
    else if( scrollState == FADE_IN ) {
        line1Alpha -= 15;
        if( line1Alpha <= 0 ) {
            line1Alpha = 0;
            ScrollOffset = 0;
            scrollTimer.stop();
            scrollState = PAUSE_SCROLL;
            scrollTimer.setInterval( m_scrollInterval );
            scrollTimer.start();
        }
    }
    else if( scrollState == FADE_OUT ) {
        line1Alpha += 7;
//        ScrollOffset += Line1FontWidth; // keep scrolling while fading
        ScrollOffset += scrollStep;
        if( line1Alpha >= 255 ) {
            line1Alpha = 255;
            ScrollOffset = 0;
            scrollState = FADE_IN;
        }
    }
    PaintSqueezeDisplay(activeDevice->getDisplayBuffer());
}

void MythSqueezeDisplay::slotUpdateTransition(int frame)
{
    switch( transitionDirection ) {
    case transLEFT:
        xOffsetOld = 0 - frame;
        xOffsetNew = m_displayRect.width() - frame;
        yOffsetOld = yOffsetNew = 0;
        break;
    case transRIGHT:
        xOffsetOld = 0 + frame;
        xOffsetNew = frame - m_displayRect.width();
        yOffsetOld = yOffsetNew = 0;
        break;
    case transUP:
        xOffsetOld = xOffsetNew = 0;
        yOffsetOld = 0 - frame;
        yOffsetNew = line1Bounds.height() - frame;
        break;
    case transDOWN:
        xOffsetOld = xOffsetNew = 0;
        yOffsetOld = 0 + frame;
        yOffsetNew = frame - line1Bounds.height();
        break;
    case transNONE:
    default:
        xOffsetOld = xOffsetNew = 0;
        yOffsetOld = yOffsetNew = 0;
        break;
    }
    PaintSqueezeDisplay(activeDevice->getDisplayBuffer());
}

void MythSqueezeDisplay::slotTransitionFinished(void)
{
    vertTransTimer->stop();
    horzTransTimer->stop();
    bumpTransTimer->stop();
    isTransition = false;
    xOffsetOld = xOffsetNew = 0;
    yOffsetOld = yOffsetNew = 0;
    transitionDirection = transNONE;
    slotResetSlimDisplay();
}

void MythSqueezeDisplay::LoadTransitionBuffer(void)
{
    DisplayBuffer *temp = activeDevice->getDisplayBuffer();
    transBuffer.center0 = temp->center0;
    transBuffer.center1 = temp->center1;
    transBuffer.displayUpdateType = temp->displayUpdateType;
    transBuffer.line0 = temp->line0;
    transBuffer.line1 = temp->line1;
    transBuffer.overlay0 = temp->overlay0;
    transBuffer.overlay1 = temp->overlay1;
}

bool MythSqueezeDisplay::Slimp3Display( QString txt )
{
    QRegExp rx( "\037" );      // the CLI overlay for the Slimp3 display uses 0x1F (037 octal) to delimit the segments of the counter
    if( rx.indexIn( txt ) != -1 ) {
        DEBUGF(true);
        return true;
    }
    else {
        DEBUGF(false);
        return false;
    }
}

void MythSqueezeDisplay::PaintSqueezeDisplay(DisplayBuffer *buf)
{
    DEBUGF("Colors" << m_displayBackgroundColor << m_textcolorGeneral );
    int playedCount = 0;
    int totalCount = 1; // this way we never get a divide by zero error
    QString timeText = "";


    QPainter p( displayImage );
    QBrush b( m_displayBackgroundColor );
    m_textcolorGeneral.setAlpha( m_Brightness );
    textcolorLine1.setAlpha( m_Brightness - line1Alpha );

    QBrush c( m_textcolorGeneral );
    QBrush e( c ); // non-filling brush
    e.setStyle( Qt::NoBrush );
    p.setBackground( b );
    p.setBrush( c );
    p.setPen( m_textcolorGeneral );
    p.setFont( large );
    p.eraseRect( displayImage->rect() );

    DEBUGF("LINE 0: " << line0Bounds << " LINE 1:" << line1Bounds);

    // draw Line 0  NOTE: Only transitions left or right, but NOT up and down
    if( buf->line0.length() > 0 ) {
        p.setFont( small );
        if( isTransition && ( transitionDirection == transLEFT || transitionDirection == transRIGHT)) {
            p.drawText(line0Bounds.left()+xOffsetOld, line0Bounds.bottom(), transBuffer.line0);
            p.drawText(line0Bounds.left()+xOffsetNew, line0Bounds.bottom(), buf->line0);
        }
        else
            p.drawText(line0Bounds.x(), line0Bounds.bottom(), buf->line0);
    }

    // draw Line 1
    if( buf->line1.length() > 0 ) {
        if( buf->line0.left( 8 ) == "Volume (" ) {   // it's the volume, so emulate a progress bar
            qreal volume = buf->line0.mid( 8, buf->line0.indexOf( ')' ) - 8 ).toInt();
            volFillRect.setWidth( (qreal)volRect.width() * ( volume / (qreal)100 ) );
            p.setBrush( e );  // non-filling brush so we end up with an outline of a rounded rectangle
            p.drawRoundedRect( volRect, radius, radius );
            p.setBrush( c );
            if( volume > 1 ) // if it's too small, we get a funny line at the start of the progress bar
                p.drawRoundedRect( volFillRect, radius, radius );
        }
        else {
            QBrush cLine1( textcolorLine1 );
            p.setBrush( cLine1 );
            p.setPen( textcolorLine1 );
            p.setFont( large );
            p.setClipRegion( line1Clipping );
            if( isTransition ) {
                p.drawText( line1Bounds.x() + xOffsetOld, line1Bounds.bottom() + yOffsetOld, transBuffer.line1);
                p.drawText( line1Bounds.x() + xOffsetNew, line1Bounds.bottom() + yOffsetNew, buf->line1 );
            } else {
                //                p.drawText( line1Bounds.x(), line1Bounds.bottom(), buf->line1 );
                p.drawText( line1Bounds.x() - ScrollOffset, line1Bounds.bottom(), buf->line1 );
                if( scrollState != NOSCROLL )
                    p.drawText(pointLine1_2.x() - ScrollOffset, line1Bounds.bottom(), buf->line1 );
            }
            p.setClipRegion( fullDisplayClipping );
            p.setBrush( c );
            p.setPen( m_textcolorGeneral );
        }
    }

    // deal with "overlay0" (the right-hand portion of the display) this can be time (e.g., "-3:08") or number of items (e.g., "(2 of 7)")
    if( buf->overlay0.length() > 0 ) {
        if( Slimp3Display( buf->overlay0 ) ) {
            QRegExp rx( "\\W(\\w+)\\W");
            //            QRegExp rx( "\037(\\w+)\037" );
            QStringList el;
            int pos = 0;

            while ((pos = rx.indexIn(buf->overlay0, pos)) != -1) {
                el << rx.cap(1);
                pos += rx.matchedLength();
            }

            rx.indexIn( buf->overlay0 );
            QStringList::iterator it = el.begin();
            while( it != el.end() ) {
                QString s = *it;
                if( s.left( 12 ) == "leftprogress" ) { // first element
                    int inc = s.at( 12 ).digitValue();
                    playedCount += inc;
                    totalCount += 4;
                }
                else if( s.left( 14 ) == "middleprogress" ) { // standard element
                    int inc = s.at( 14 ).digitValue();
                    playedCount += inc;
                    totalCount += 4;
                }
                else if( s.left( 10 ) == "solidblock" ) { // full block
                    playedCount += 4;
                    totalCount += 4;
                }
                else if( s.left( 13 ) == "rightprogress" ) { // end element
                    int inc = s.at( 13 ).digitValue();
                    playedCount += inc;
                    totalCount += 4;
                }
                ++it;
            }
            QChar *data = buf->overlay0.data();
            for( int i = ( buf->overlay0.length() - 8 ); i < buf->overlay0.length(); i++ ) {
                if( *( data + i ) == ' ' ) {
                    timeText = buf->overlay0.mid( i + 1 );
                }
            }
        }
        else if( buf->overlay0.contains( QChar( 8 ) ) ) {
            QChar elapsed = QChar(8);
            QChar remaining = QChar(5);
            QChar *data = buf->overlay0.data();
            for( int i = 0; i < buf->overlay0.length(); i++, data++ ) {
                if( *data == elapsed ) {
                    playedCount++;
                    totalCount++;
                }
                else if( *data == remaining )
                    totalCount++;
                else if( *data == ' ' ) {
                    timeText = buf->overlay0.mid( i + 1 );
                }
            }
        }
        else {
            timeText = buf->overlay0;
        }
        p.setFont( small );
        QFontMetrics fm = p.fontMetrics();
        //        p.setClipping(false);
        p.setClipRegion(line0Clipping );
        if( isTransition ) {
            p.drawText( ( line0Bounds.right() + xOffsetNew ) - fm.width(timeText), line0Bounds.bottom(), timeText );
        }
        else {
            p.drawText( line0Bounds.right() - fm.width(timeText), line0Bounds.bottom(), timeText );
        }
        if( totalCount > 1 ) {  // make sure we received data on a progress bar, otherwise, don't draw
            progRect.setLeft( ( qreal )( line0Bounds.x() + fm.width( buf->line0.toUpper() ) + xOffsetOld ) );
            progRect.setRight( ( qreal )( line0Bounds.right() + xOffsetOld - ( qreal )( 3 * fm.width( timeText ) / 2 ) ) );
            progFillRect.setLeft( progRect.left() );
            progFillRect.setWidth( ( playedCount * progRect.width() ) / totalCount );
            p.setBrush( e );  // non-filling brush so we end up with an outline of a rounded rectangle
            p.drawRoundedRect( progRect, radius, radius );
            p.setBrush( c );
            if( playedCount > 1 ) // if it's too small, we get a funny line at the start of the progress bar
                p.drawRoundedRect( progFillRect, radius, radius );
        }
    }

    // deal with "overlay1" (the right-hand portion of the display)
//    if( buf->overlay1.length() > 0 ) {
//        DEBUGF( "Don't know what to do with overlay1 yet" );
//    }
//     if we've received a "center" display, it means we're powered down, so draw them
        if( buf->center0.length() > 0 ) {
            p.setFont( medium );
            QFontMetrics fm = p.fontMetrics();
            QPoint start = QPoint( center0Bounds.x() + (center0Bounds.width()/2) - (fm.width( buf->center0 )/2 ), center0Bounds.bottom() );
            p.drawText( start, buf->center0 );
        }

        if( buf->center1.length() > 0 ) {
            p.setFont( medium );
            QFontMetrics fm = p.fontMetrics();
            QPoint start = QPoint( center1Bounds.x() + (center1Bounds.width()/2)- ( fm.width( buf->center1 )/2 ), center1Bounds.bottom() );
            p.drawText( start, buf->center1 );
        }
//    displayLabel.setPixmap(QPixmap::fromImage( *displayImage) );
        QPixmap pix = QPixmap::fromImage(*displayImage);
        m_squeezeDisplay->UpdateFrame(&pix);
}

