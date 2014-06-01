/***************************************************************************
 *   file klfguiutil.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2011 by Philippe Faist
 *   philippe.faist at bluewin.ch
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/* $Id: klfguiutil.cpp 709 2011-08-26 14:26:33Z phfaist $ */

#include <math.h>

#include <QApplication>
#include <QDesktopWidget>
#include <QIcon>
#include <QPushButton>
#include <QDebug>

#include "klfutil.h"
#include "klfrelativefont.h"
#include "klfguiutil.h"


// ----------------------------------------------


KLFProgressReporter::KLFProgressReporter(int min, int max, QObject *parent)
  : QObject(parent)
{
  pMin = min;
  pMax = max;
  pFinished = false;
}
KLFProgressReporter::~KLFProgressReporter()
{
  if (!pFinished) {  // make sure finished() is emitted.
    emit progress(pMax); // some connected clients just wait for maximum value progress
    emit finished();
  }
}

void KLFProgressReporter::doReportProgress(int value)
{
  if (pFinished) {
    qWarning()<<KLF_FUNC_NAME<<": Operation is already finished!";
    return;
  }
  emit progress(value);
  if (value >= pMax) {
    emit finished();
    pFinished = true;
  }
}



// ---------------------------------------------------------



KLFProgressDialog::KLFProgressDialog(QString labelText, QWidget *parent)
  : QProgressDialog(parent)
{
  setup(false);
  init(labelText);
}
KLFProgressDialog::KLFProgressDialog(bool canCancel, QString labelText, QWidget *parent)
  : QProgressDialog(parent)
{
  setup(canCancel);
  init(labelText);
}
KLFProgressDialog::~KLFProgressDialog()
{
}

void KLFProgressDialog::setup(bool canCancel)
{
  pProgressReporter = NULL;
  setAutoClose(true);
  setAutoReset(true);
  setModal(true);
  //  setWindowModality(Qt::ApplicationModal);
  setWindowIcon(QIcon(":/pics/klatexformula-16.png"));
  setWindowTitle(tr("Progress"));
  QPushButton *cbtn = new QPushButton(tr("Cancel"), this);
  setCancelButton(cbtn);
  cbtn->setEnabled(canCancel);
}
void KLFProgressDialog::init(const QString& labelText)
{
  setDescriptiveText(labelText);
}

void KLFProgressDialog::setDescriptiveText(const QString& labelText)
{
  setLabelText(labelText);
  setFixedSize((int)(sizeHint().width()*1.3), (int)(sizeHint().height()*1.1));
}
void KLFProgressDialog::startReportingProgress(KLFProgressReporter *progressReporter,
					       const QString& descriptiveText)
{
  reset();
  setDescriptiveText(descriptiveText);
  setRange(progressReporter->min(), progressReporter->max());
  setValue(0);

  // disconnect any previous progress reporter object
  if (pProgressReporter != NULL)
    disconnect(pProgressReporter, 0, this, SLOT(setValue(int)));
  // and connect to this new one
  connect(progressReporter, SIGNAL(progress(int)), this, SLOT(setValue(int)));
}

void KLFProgressDialog::startReportingProgress(KLFProgressReporter *progressReporter)
{
  reset();
  setRange(progressReporter->min(), progressReporter->max());
  setValue(0);
  // disconnect any previous progress reporter object
  if (pProgressReporter != NULL)
    disconnect(pProgressReporter, 0, this, SLOT(setValue(int)));
  // and connect to this new one
  connect(progressReporter, SIGNAL(progress(int)), this, SLOT(setValue(int)));
}

void KLFProgressDialog::setValue(int value)
{
  //  KLF_DEBUG_BLOCK(KLF_FUNC_NAME);
  klfDbg("value="<<value);
  QProgressDialog::setValue(value);
}

void KLFProgressDialog::paintEvent(QPaintEvent *event)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME);
  QProgressDialog::paintEvent(event);
}


// --------------------------


static Qt::WindowFlags klfpleasewait_flagsForSettings(bool alwaysAbove)
{
  Qt::WindowFlags f =  Qt::Window|Qt::SplashScreen|Qt::FramelessWindowHint;
  if (alwaysAbove)
    f |= Qt::WindowStaysOnTopHint|Qt::X11BypassWindowManagerHint;
  return f;
}

KLFPleaseWaitPopup::KLFPleaseWaitPopup(const QString& text, QWidget *parent, bool alwaysAbove)
  : QLabel(text, ((parent!=NULL)?parent->window():NULL), klfpleasewait_flagsForSettings(alwaysAbove)),
    pParentWidget(parent), pDisableUi(false), pGotPaintEvent(false), pDiscarded(false)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  KLFRelativeFont *relfont = new KLFRelativeFont(this);
  relfont->setRelPointSize(+2);

  setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
  setWindowModality(Qt::ApplicationModal);
  // let this window be styled by skins
  setAttribute(Qt::WA_StyledBackground, true);
  setProperty("klfTopLevelWidget", QVariant(true));

  setFrameStyle(QFrame::Panel|QFrame::Sunken);

  QWidget *pw = parentWidget(); // the one set in QLabel constructor, this is the top-level window
  if (pw != NULL)
    setStyleSheet(pw->window()->styleSheet());

  int w = qMax( (int)(sizeHint().width() *1.3) , 500 );
  int h = qMax( (int)(sizeHint().height()*1.1) , 100 );
  setFixedSize(w, h);
  setWindowOpacity(0.94);
}
KLFPleaseWaitPopup::~KLFPleaseWaitPopup()
{
  if (pDisableUi && pParentWidget != NULL)
    pParentWidget->setEnabled(true);
}

void KLFPleaseWaitPopup::setDisableUi(bool disableUi)
{
  pDisableUi = disableUi;
}

void KLFPleaseWaitPopup::showPleaseWait()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  QSize desktopSize;
  QDesktopWidget *dw = QApplication::desktop();
  if (dw != NULL) {
    desktopSize = dw->screenGeometry(this).size();
  } else {
    desktopSize = QSize(1024, 768); // assume some default, worst case widget is more left and higher
  }
  move(desktopSize.width()/2 - width()/2, desktopSize.height()/2 - height()/2);
  show();
  setStyleSheet(styleSheet());

  if (pDisableUi && pParentWidget != NULL)
    pParentWidget->setEnabled(false);

  while (!pGotPaintEvent)
    qApp->processEvents();
}

void KLFPleaseWaitPopup::mousePressEvent(QMouseEvent */*event*/)
{
  hide();
  pDiscarded = true;
}

void KLFPleaseWaitPopup::paintEvent(QPaintEvent *event)
{
  pGotPaintEvent = true;
  QLabel::paintEvent(event);
}



// --------------------------


KLFDelayedPleaseWaitPopup::KLFDelayedPleaseWaitPopup(const QString& text, QWidget *callingWidget)
  : KLFPleaseWaitPopup(text, callingWidget), pDelay(1000)
{
  timer.start();
}
KLFDelayedPleaseWaitPopup::~KLFDelayedPleaseWaitPopup()
{
}
void KLFDelayedPleaseWaitPopup::setDelay(int ms)
{
  pDelay = ms;
}
void KLFDelayedPleaseWaitPopup::process()
{
  if (!pleaseWaitShown() && timer.elapsed() > pDelay)
    showPleaseWait();
  qApp->processEvents();
}



// ------------------------------------------------


KLFEnumComboBox::KLFEnumComboBox(QWidget *parent)
  : QComboBox(parent)
{
  setEnumValues(QList<int>(), QStringList());
  connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(internalCurrentIndexChanged(int)));
}

KLFEnumComboBox::KLFEnumComboBox(const QList<int>& enumValues, const QStringList& enumTitles,
				 QWidget *parent)
  : QComboBox(parent)
{
  setEnumValues(enumValues, enumTitles);
  connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(internalCurrentIndexChanged(int)));
}

KLFEnumComboBox::~KLFEnumComboBox()
{
}

void KLFEnumComboBox::setEnumValues(const QList<int>& enumValues, const QStringList& enumTitles)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("enumValues="<<enumValues<<"; enumTitles="<<enumTitles);
  blockSignals(true);
  int savedCurrentIndex = currentIndex();
  if (enumValues.size() != enumTitles.size()) {
    qWarning()<<KLF_FUNC_NAME<<": enum value list and enum title list do not match!";
    return;
  }
  pEnumValueList = enumValues;
  clear();
  int k;
  for (k = 0; k < enumValues.size(); ++k) {
    pEnumValues[enumValues[k]] = enumTitles[k];
    insertItem(k, enumTitles[k], QVariant(enumValues[k]));
    pEnumCbxIndexes[enumValues[k]] = k;
  }
  if (savedCurrentIndex >= 0 && savedCurrentIndex < count())
    setCurrentIndex(savedCurrentIndex);
  blockSignals(false);
}

int KLFEnumComboBox::selectedValue() const
{
  return itemData(currentIndex()).toInt();
}

QString KLFEnumComboBox::enumText(int enumValue) const
{
  if (!pEnumValueList.contains(enumValue)) {
    qWarning()<<KLF_FUNC_NAME<<": "<<enumValue<<" is not a registered valid enum value!";
    return QString();
  }
  return pEnumValues[enumValue];
}

void KLFEnumComboBox::setSelectedValue(int val)
{
  if (!pEnumCbxIndexes.contains(val)) {
    qWarning()<<KLF_FUNC_NAME<<": "<<val<<" is not a registered valid enum value!";
    return;
  }
  setCurrentIndex(pEnumCbxIndexes[val]);
}

void KLFEnumComboBox::internalCurrentIndexChanged(int index)
{
  emit selectedValueChanged(itemData(index).toInt());
}


// ------------------------


KLFWaitAnimationOverlay::KLFWaitAnimationOverlay(QWidget *parent)
  : QLabel(parent)
{
  pAnimMovie = NULL;
  /*
    pAnimMovie = new QMovie(":/pics/wait_anim.mng", "MNG", this);
    pAnimMovie->setCacheMode(QMovie::CacheAll);
  */

  setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);

  hide();

  pAnimTimerId = -1;
  pIsWaiting = false;

  // default values
  pWidthPercent = 30;
  pHeightPercent = 70;
  pPositionXPercent = 50;
  pPositionYPercent = 50;

  setBackgroundColor(QColor(255,255,255,128));
}

KLFWaitAnimationOverlay::~KLFWaitAnimationOverlay()
{
}

QColor KLFWaitAnimationOverlay::backgroundColor() const
{
  return palette().color(QPalette::Window);
}

void KLFWaitAnimationOverlay::setWaitMovie(QMovie *movie)
{
  if (pAnimMovie != NULL) {
    delete pAnimMovie;
  }
  pAnimMovie = movie;
  pAnimMovie->setParent(this);
}

void KLFWaitAnimationOverlay::setWaitMovie(const QString& filename)
{
  QMovie *m = new QMovie(filename);
  m->setCacheMode(QMovie::CacheAll);
  setWaitMovie(m);
}


void KLFWaitAnimationOverlay::setBackgroundColor(const QColor& c)
{
  setStyleSheet(QString("background-color: rgba(%1,%2,%3,%4)")
		.arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alpha()));
}


void KLFWaitAnimationOverlay::startWait()
{
  if (pIsWaiting)
    return;

  pIsWaiting = true;
  if (pAnimMovie == NULL)
    return;

  pAnimMovie->jumpToFrame(0);
  setPixmap(pAnimMovie->currentPixmap());
  setGeometry(calcAnimationLabelGeometry());
  show();
  update();

  qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

  pAnimTimerId = startTimer(pAnimMovie->nextFrameDelay());
}

void KLFWaitAnimationOverlay::stopWait()
{
  if (!pIsWaiting)
    return;

  hide();

  if (pAnimTimerId >= 0)
    killTimer(pAnimTimerId);
  pAnimTimerId = -1;
  pIsWaiting = false;
}

void KLFWaitAnimationOverlay::timerEvent(QTimerEvent *event)
{
  if (event->timerId() == pAnimTimerId) {
    pAnimMovie->jumpToNextFrame();
    setPixmap(pAnimMovie->currentPixmap());
    repaint();
    return;
  }
}

QRect KLFWaitAnimationOverlay::calcAnimationLabelGeometry()
{
  QWidget * w = parentWidget();
  if (w == NULL) {
    qWarning()<<KLF_FUNC_NAME<<": this animation label MUST be used with a parent!";
    return QRect();
  }
  QRect g = w->geometry();
  QSize sz = QSize(w->width()*pWidthPercent/100,w->height()*pHeightPercent/100);

  klfDbg("parent geometry: "<<g<<"; our size="<<sz) ;

  return KLF_DEBUG_TEE( QRect(QPoint( (g.width()-sz.width())*pPositionXPercent/100,
				      (g.height()-sz.height())*pPositionYPercent/100),
			      sz) );
}



// -----------------------


KLF_EXPORT void klfDrawGlowedImage(QPainter *p, const QImage& foreground, const QColor& glowcol,
				   int r, bool also_draw_image)
{
  QImage fg = foreground;
  if (fg.format() != QImage::Format_ARGB32_Premultiplied &&
      fg.format() != QImage::Format_ARGB32)
    fg = fg.convertToFormat(QImage::Format_ARGB32);

  QRgb glow_color = glowcol.rgba();
  int ga = qAlpha(glow_color);

  QImage glow(fg.size(), QImage::Format_ARGB32_Premultiplied);
  int x, y;
  for (x = 0; x < fg.width(); ++x) {
    for (y = 0; y < fg.height(); ++y) {
      int a = qAlpha(fg.pixel(x,y)) * ga / 255;
      // glow format is argb32_premultiplied
      glow.setPixel(x,y, qRgba(qRed(glow_color)*a/255, qGreen(glow_color)*a/255, qBlue(glow_color)*a/255, a));
    }
  }
  // now draw that glowed image a few times moving around the interest point to do a glow effect
  for (x = -r; x <= r; ++x) {
    for (y = -r; y <= r; ++y) {
      if (x*x+y*y > r*r) // don't go beyond r pixels from (0,0)
	continue;
      p->drawImage(QPoint(x,y), glow);
    }
  }
  if (also_draw_image)
    p->drawImage(QPoint(0,0), fg);
}



// --------------------------

QImage klfImageScaled(const QImage& source, const QSize& newSize)
{
  QImage img = source.scaled(newSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  // set text attributes
  QStringList keys = source.textKeys();
  int k;
  for (k = 0; k < keys.size(); ++k) {
    img.setText(keys[k], source.text(keys[k]));
  }
  return img;
}


// --------------------

KLF_EXPORT QRect klf_get_window_geometry(QWidget *w)
{
#if defined(Q_WS_X11)
  QRect g = w->frameGeometry();
#else
  QRect g = w->geometry();
#endif
  return g;
}

KLF_EXPORT void klf_set_window_geometry(QWidget *w, QRect g)
{
  if ( ! g.isValid() )
    return;

  w->setGeometry(g);
}


KLFWindowGeometryRestorer::KLFWindowGeometryRestorer(QWidget *window)
  : QObject(window), pWindow(window)
{
  window->installEventFilter(this);
}

KLFWindowGeometryRestorer::~KLFWindowGeometryRestorer()
{
}

bool KLFWindowGeometryRestorer::eventFilter(QObject *obj, QEvent *event)
{
  if (obj == pWindow) {
    if (event->type() == QEvent::Hide) {
      // save geometry
      pWindow->setProperty("klf_saved_geometry", klf_get_window_geometry(pWindow));
    } else if (event->type() == QEvent::Show) {
      QVariant val;
      if ((val = pWindow->property("klf_saved_geometry")).isValid())
	klf_set_window_geometry(pWindow, val.value<QRect>());
    }
  }

  return false;
}



/** \internal
 * Important: remember that the keys in the hash windowShownStates may be a dangling pointer.
 * do _not_ reference it explicitely but use to find info for existing widgets!
 */
static QHash<QWidget*,bool> windowShownStates;

/*
inline bool has_ancestor_of_type(QObject *testobj, const char * type)
{
  klfDbg("testing if "<<testobj<<" is (grand-)child of object inheriting "<<type) ;
  if (testobj == NULL)
    return false;
  do {
    if (testobj->inherits(type)) {
      klfDbg("inherits "<<type<<"!") ;
      return true;
    }
  } while ((testobj = testobj->parent()) != NULL) ;
  klfDbg("no.") ;
  return false;
}
*/

KLF_EXPORT void klfHideWindows()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  //  / ** \todo don't _FORCE_ this setting, remember and restore it.... * /
  //  qApp->setQuitOnLastWindowClosed(false);

  // save the window states, while checking that not all the windows are already hidden
  QHash<QWidget*,bool> states;
  bool allalreadyhidden = true;
  QWidgetList wlist = QApplication::topLevelWidgets();
  foreach (QWidget *w, wlist) {
    //    if (w->inherits("QMenu"))
    //      continue;
    uint wflags = w->windowFlags();
    klfDbg("next widget in line: "<<w<<", wflags="<<wflags) ;
    if ((wflags & Qt::Window) == 0) {
      continue;
    }
    if (wflags & Qt::X11BypassWindowManagerHint) {
      /** \todo THIS IS A WORKAROUND to avoid "hiding" the system tray icon when hiding all windows (which
       *    is of course not what we want!). This also has the undesirable effect of not hiding all windows
       *    which have the x11-bypass-window-manager flag on. TODO: write some more intelligent code. */
      continue;
    }
    klfDbg("dealing with widget "<<w) ;
    bool shown = w->isVisible();
    states[w] = shown;
    if (shown) {
      klfDbg("hiding window "<<w<<", wflags="<<w->windowFlags()) ;
      w->hide();
      allalreadyhidden = false;
    }
  }
  if (!allalreadyhidden) {
    // don't overwrite the saved status list with an all-hidden state list
    windowShownStates = states;
  }
}

KLF_EXPORT void klfRestoreWindows()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  QWidgetList wlist = QApplication::topLevelWidgets();
  foreach (QWidget *w, wlist) {
    if (!windowShownStates.contains(w))
      continue;
    // restore this window
    if (!w->isVisible()) {
      klfDbg("Restoring window "<<w) ;
      w->setVisible(windowShownStates[w]);
    }
  }
}
