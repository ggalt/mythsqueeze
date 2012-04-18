#ifndef MYTHSQUEEZE_H
#define MYTHSQUEEZE_H

// MythTV headers
#include <mythtv/verbosedefs.h>
#include <mythtv/mythcontext.h>
#include <mythtv/libmythui/mythmainwindow.h>
#include <mythtv/mythdirs.h>
#include <mythtv/libmythui/mythscreentype.h>
#include <mythtv/libmythui/mythuitext.h>
#include <mythtv/libmythui/mythuibuttonlist.h>
#include <mythtv/libmythui/mythuibutton.h>
#include <mythtv/libmythui/mythuiimage.h>
#include <mythtv/libmythui/mythdialogbox.h>
#include <mythtv/libmythui/mythprogressdialog.h>
#include <mythtv/libmythui/mythfontproperties.h>
#include <mythtv/libmythui/mythpainter.h>
#include <mythtv/libmythui/mythimage.h>



/** \class MythSqueeze
 *
 */
class MythSqueeze : public MythScreenType
{
    Q_OBJECT

public:

    MythSqueeze(MythScreenStack *parent, const char *name);
    ~MythSqueeze();

    bool Create(void);
    void InitPlayer( void );
    bool keyPressEvent(QKeyEvent *e);
    void customEvent(QEvent*);

private:
    // main display coverflow (for current playlist)
    MythImage *m_CoverFlow;

    // Artist, Album, Genre and Year selection coverflows
    MythUIImage       *m_slimDisplay;
};

#endif // MYTHSQUEEZE_H
