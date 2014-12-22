/***************************************************************************
 *   file klfmainwin_p.h
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
/* $Id: klfmainwin_p.h 631 2011-04-14 10:14:00Z phfaist $ */

#ifndef KLFMAINWIN_P_H
#define KLFMAINWIN_P_H


/** \file
 * This file contains internal definitions for file klfmainwin.cpp.
 * \internal
 */

#include <QDialog>
#include <QFile>
#include <QString>
#include <QByteArray>
#include <QLabel>
#include <QImageReader>
#include <QBuffer>
#include <QFileInfo>
#include <QMessageBox>

#include <klfutil.h>
#include "klflibview.h"
#include "klfmain.h"
#include "klfsettings.h"

#include "klfmainwin.h"

#include <ui_klfaboutdialog.h>
#include <ui_klfwhatsnewdialog.h>

/** \internal */
class KLFHelpDialogCommon
{
public:
  KLFHelpDialogCommon(const QString& baseFName) : pBaseFName(baseFName) { }
  virtual ~KLFHelpDialogCommon() { }

  virtual void addExtraText(const QString& htmlSnipplet)
  {
    pHtmlExtraSnipplets << htmlSnipplet;
  }

  virtual QString getFullHtml()
  {
    QString html;
    QString fn = klfFindTranslatedDataFile(":/data/"+pBaseFName, ".html");

    QFile f(fn);
    f.open(QIODevice::ReadOnly);
    html = QString::fromUtf8(f.readAll());

    klfDbg( "read from file="<<fn<<" the HTML code=\n"<<html ) ;

    const QString marker = QLatin1String("<!--KLF_MARKER_INSERT_SPOT-->");
    int k;
    for (k = 0; k < pHtmlExtraSnipplets.size(); ++k)
      html.replace(marker, pHtmlExtraSnipplets[k]+"\n"+marker);

    klfDbg( "new HTML is:\n"<<html ) ;
    
    // replace some general recognized "macros"
    html.replace("<!--KLF_VERSION-->", KLF_VERSION_STRING);

    return html;
  }

protected:
  QString pBaseFName;
  QStringList pHtmlExtraSnipplets;
};


/** \internal */
class KLFAboutDialog : public QDialog, public KLFHelpDialogCommon
{
  Q_OBJECT
signals:
  void linkActivated(const QUrl&);
public:
  KLFAboutDialog(QWidget *parent) : QDialog(parent), KLFHelpDialogCommon("about")
  {
    u = new Ui::KLFAboutDialog;
    u->setupUi(this);

    connect(u->txtDisplay, SIGNAL(anchorClicked(const QUrl&)),
	    this, SIGNAL(linkActivated(const QUrl&)));
  }
  virtual ~KLFAboutDialog() { }

  virtual void show()
  {
    u->txtDisplay->setHtml(getFullHtml());
    QDialog::show();
    // refresh our style sheet
    setStyleSheet(styleSheet());
  }

private:
  Ui::KLFAboutDialog *u;
};


/** \internal */
class KLFWhatsNewDialog : public QDialog, public KLFHelpDialogCommon
{
  Q_OBJECT
signals:
  void linkActivated(const QUrl&);
public:
  KLFWhatsNewDialog(QWidget *parent)
    : QDialog(parent),
      KLFHelpDialogCommon(QString("whats-new-%1.%2").arg(klfVersionMaj()).arg(klfVersionMin()))
  {
    u = new Ui::KLFWhatsNewDialog;
    u->setupUi(this);

    connect(u->txtDisplay, SIGNAL(anchorClicked(const QUrl&)),
	    this, SIGNAL(linkActivated(const QUrl&)));
  }
  virtual ~KLFWhatsNewDialog() { }

  virtual void show()
  {
    u->txtDisplay->setHtml(getFullHtml());
    QDialog::show();
    // refresh our style sheet
    setStyleSheet(styleSheet());
  }

private:
  Ui::KLFWhatsNewDialog *u;
};



/** \internal */
class KLFMainWinPopup : public QLabel
{
  Q_OBJECT
public:
  KLFMainWinPopup(KLFMainWin *mainwin)
    : QLabel(mainwin, Qt::Window|Qt::FramelessWindowHint|Qt::CustomizeWindowHint|
	     Qt::WindowStaysOnTopHint|Qt::X11BypassWindowManagerHint)
  {
    setObjectName("KLFMainWinPopup");
    setFocusPolicy(Qt::NoFocus);
    QWidget *frmMain = mainwin->findChild<QWidget*>("frmMain");
    if (frmMain)
      setGeometry(QRect(mainwin->geometry().topLeft()+QPoint(25,mainwin->height()*2/3),
			QSize(frmMain->width()+15, 0)));
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setProperty("klfTopLevelWidget", QVariant(true));
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(mainwin->window()->styleSheet());
    setMargin(0);
    QFont f = font();
    f.setPointSize(QFontInfo(f).pointSize() - 1);
    setFont(f);
    setWordWrap(true);

    connect(this, SIGNAL(linkActivated(const QString&)),
	    this, SLOT(internalLinkActivated(const QString&)));
    connect(this, SIGNAL(linkActivated(const QUrl&)),
	    mainwin, SLOT(helpLinkAction(const QUrl&)));
  }

  virtual ~KLFMainWinPopup() { }

  bool hasMessages() { return (bool)msgKeys.size(); }

  QStringList messageKeys() { return msgKeys; }

signals:
  void linkActivated(const QUrl& url);

public slots:

  /** Displays the message \c msgText in the label. \c msgKey is arbitrary
   * and may be used to remove the message with removeMessage() later on.
   */
  void addMessage(const QString& msgKey, const QString& msgText)
  {
    klfDbg("adding message "<<msgKey<<" -> "<<msgText);
    msgKeys << msgKey;
    messages << ("<p>"+msgText+"</p>");
    updateText();
  }

  void removeMessage(const QString& msgKey)
  {
    int i = msgKeys.indexOf(msgKey);
    if (i < 0)
      return;
    msgKeys.removeAt(i);
    messages.removeAt(i);
    updateText();
  }

  void show()
  {
    QLabel::show();
    setStyleSheet(styleSheet());
  }

private slots:
  void internalLinkActivated(const QString& url)
  {
    emit linkActivated(QUrl::fromEncoded(url.toLatin1()));
  }

  void updateText()
  {
    setText("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\""
	    " \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
	    "<html><head><meta name=\"qrichtext\" content=\"1\" />"
	    "<style type=\"text/css\">\n"
	    "body { margin: 2px 10px; }\n"
	    "p { white-space: pre-wrap; padding: 0px; margin: 0px 0px 2px 0px; }\n"
	    "a, a:link { text-decoration: underline; color: #0000af }\n"
	    "a:hover { color: #0000ff }\n"
	    "a:active { color: #ff0000 }\n"
	    "}\n"
	    "</style>"
	    "</head>"
	    "<body>\n"
	    + messages.join("\n") +
	    "<p style=\"margin-top: 5px\">"
	    "<a href=\"klfaction:/popup?acceptAll\">"+tr("Accept [<b>Alt-Enter</b>]")+"</a> | "+
	    " <a href=\"klfaction:/popup_close\">"+tr("Close [<b>Esc</b>]")+"</a> | " +
	    " <a href=\"klfaction:/popup?configDontShow\">"+tr("Don't Show Again")+"</a></p>"+
	    "</body></html>" );
    resize(width(), sizeHint().height());
  }

private:
  QStringList msgKeys;
  QStringList messages;
};





// -------------------------------------------------


class KLFBasicDataOpener : public QObject, public KLFAbstractDataOpener
{
  Q_OBJECT
public:
  KLFBasicDataOpener(KLFMainWin *mainwin) : QObject(mainwin), KLFAbstractDataOpener(mainwin) { }
  virtual ~KLFBasicDataOpener() { }

  virtual QStringList supportedMimeTypes()
  {
    QStringList types;
    types << "image/png"
	  << "image/jpeg"
	  << "text/uri-list"
	  << "application/x-klf-libentries"
	  << "application/x-klatexformula"
	  << "application/x-klatexformula-db" ;
    int k;
    QList<QByteArray> fmts = QImageReader::supportedImageFormats();
    for (k = 0; k < fmts.size(); ++k) {
      types << "image/"+fmts[k].toLower();
      types << "image/x-"+fmts[k].toLower();
    }
    klfDbg("mime types: "<<types) ;
    return types;
  }

  virtual bool canOpenFile(const QString& file)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    klfDbg("file="<<file) ;

    QFile f(file);
    bool r = f.open(QIODevice::ReadOnly);
    if (!r) {
      klfDbg(" ... file cannot be accessed.") ;
      return false;
    }
    bool isimage = false;
    if (isKlfImage(&f, &isimage)) {
      klfDbg(" ... is KLF-saved image.") ;
      return true;
    }
    if (isimage) { // no try going further, we can't otherwise read image ...
      klfDbg(" ... is a non-KLF image.") ;
      return false;
    }

    QString libscheme = KLFLibBasicWidgetFactory::guessLocalFileScheme(file);
    if (!libscheme.isEmpty()) {
      klfDbg(" ... libscheme="<<libscheme) ;
      return true; // it is a library file
    }
    // by default, fail
    klfDbg(" ... not recognized.") ;
    return false;
  }

  virtual bool canOpenData(const QByteArray& data)
  {
    QBuffer buf;
    buf.setData(data);
    buf.open(QIODevice::ReadOnly);
    bool isimg = false;
    if (isKlfImage(&buf, &isimg))
      return true;
    if (isimg) // no try going further, we can't otherwise read image ...
      return false;

    // try to read beginning with a QDataStream to look for a known x-klf-libentries header
    QDataStream stream(&buf);
    stream.setVersion(QDataStream::Qt_4_4);
    QString headerstr;
    stream >> headerstr;
    if (headerstr == QLatin1String("KLF_LIBENTRIES"))
      return true;
    // *** see note in openData() for library formats. ***
    // don't try to open .klf or .klf.db formats.

    // otherwise, we can't recognize data, fail
    return false;
  }

  virtual bool openFile(const QString& file)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    klfDbg("file="<<file);
    QFileInfo fi(file);
    if (!fi.exists() || !fi.isReadable()) {
      klfDbg(" ... file is not readable") ;
      return false;
    }
    QString ext = fi.suffix().trimmed().toLower();

    { // try to open image
      QFile f(file);
      bool r = f.open(QIODevice::ReadOnly);
      if (!r) {
	klfDbg(" ... file cannot be opened") ;
	return false;
      }
      bool isimage = false;
      if (tryOpenImageData(&f, &isimage)) {
	klfDbg(" ... loaded image data!") ;
	return true;
      }
      if (isimage) { // no try going further, we can't otherwise read image ...
	klfDbg(" ... is non-KLF image...") ;
	return false;
      }
    }

    // open image failed, try the other formats...

    // try to load library file
    bool result = mainWin()->openLibFile(file);

    klfDbg("mainWin()->openLibFile("<<file<<") returned "<<result) ;
    if (result)
      return true;

    // and otherwise, fail
    return false;
  }

  virtual bool openData(const QByteArray& data, const QString& mimetype)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    klfDbg("mimetype is "<<mimetype) ;

    QBuffer buf;
    buf.setData(data);
    buf.open(QIODevice::ReadOnly);

    if (mimetype == "text/uri-list") {
      klfDbg("Opening uri-list") ;
      QByteArray line;
      QStringList flist;
      char sbuf[1024];
      qint64 len;
      while ((len = buf.readLine(sbuf, sizeof(sbuf))) > 0) {
	line = QByteArray(sbuf, len);
	line = line.trimmed(); // gnome uses \r\n, and URLs don't have whitespace.
	QUrl url = QUrl::fromEncoded(line);
	if (url.scheme() != "file") {
	  qWarning()<<KLF_FUNC_NAME<<": can't open URL "<<url.scheme()<<" (can only open local files)";
	  continue;
	}
	flist << klfUrlLocalFilePath(url);
	klfDbg("... added file "<<flist.last()) ;
      }
      if (flist.isEmpty()) {
	klfDbg("file list is empty.") ;
	return true;
      }
      return  mainWin()->openFiles(flist);
    }

    bool isimage = false;
    if (tryOpenImageData(&buf, &isimage))
      return true;
    if (isimage) // no try going further, we can't otherwise read image ...
      return false;

    // drop application/x-klf-libentries
    if (tryOpenKlfLibEntries(&buf))
      return true;

    // otherwise if we paste/drop library entries, the case is more delicate, as it can't be
    // just opened in the library browser, as they are not in a file. So for now, fail to open the data
    // Possibly, todo: suggest user to create a resource/sub-resource to drop the data into
    return false;
  }

private:
  bool prepareImageReader(QImageReader *imagereader, QImage *img, bool *isImage)
  {
    *isImage = false;
    if (!imagereader->canRead()) {
      klfDbg("format "<<imagereader->format()<<": canRead() returned FALSE!") ;
      return false;
    }
    *img = imagereader->read();
    if (img->isNull()) {
      klfDbg("read() returned a null qimage!") ;
      return false;
    }
    *isImage = true;
    klfDbg("format: '"<<imagereader->format().constData()<<"': text keys set: "<<img->textKeys()
	   <<"; error="<<imagereader->error()
	   <<"; quality="<<imagereader->quality()
	   <<"; supportsOption(description)="<<imagereader->supportsOption(QImageIOHandler::Description)
	   <<"; text('AppVersion')="<<img->text("AppVersion")
	   ) ;
    if (!img->text("AppVersion").startsWith("KLatexFormula ") && !img->text("AppVersion").startsWith("KLF ")) {
      klfDbg("AppVersion is not set to 'KL[atex]F[ormula] [...]' in image metadata.") ;
      return false;
    }
    // it is a klf-saved image
    return true;
  }
  bool isKlfImage(QIODevice *device, bool *isImage)
  {
    QList<QByteArray> formatlist = QImageReader::supportedImageFormats();
    int k;
    QImage img;
    QImageReader imagereader;
    for (k = 0; k < formatlist.size(); ++k) {
      device->seek(0);
      imagereader.setFormat(formatlist[k]);
      imagereader.setDevice(device);
      if (prepareImageReader(&imagereader, &img, isImage))
	return true;
    }
    return false;
  }
  bool tryOpenImageData(QIODevice *device, bool *isimage)
  {
    QList<QByteArray> formatlist = QImageReader::supportedImageFormats();
    int k;
    QImageReader imagereader;
    QImage img;
    for (k = 0; k < formatlist.size(); ++k) {
      device->seek(0);
      imagereader.setFormat(formatlist[k]);
      imagereader.setDevice(device);
      if (!prepareImageReader(&imagereader, &img, isimage))
	continue;

      // read meta-information
      QString latex = img.text("InputLatex");
      KLFStyle style;
      style.fg_color = read_color(img.text("InputFgColor"));
      style.bg_color = read_color(img.text("InputBgColor"));
      style.mathmode = img.text("InputMathMode");
      style.preamble = img.text("InputPreamble");
      style.dpi = img.text("InputDPI").toInt();

      mainWin()->slotLoadStyle(style);
      mainWin()->slotSetLatex(latex);
      return true;
    }
    return false;
  }
  unsigned long read_color(const QString& text)
  {
    //                               1          2          3     4         5
    QRegExp rx1("\\s*rgba?\\s*\\(\\s*(\\d+),\\s*(\\d+),\\s*(\\d+)(\\s*,\\s*(\\d+))?\\s*\\)\\s*");
    if (rx1.exactMatch(text)) {
      if (rx1.cap(4).isEmpty())
	return qRgb(rx1.cap(1).toInt(), rx1.cap(2).toInt(), rx1.cap(3).toInt());
      else
	return qRgba(rx1.cap(1).toInt(), rx1.cap(2).toInt(), rx1.cap(3).toInt(), rx1.cap(5).toInt());
    }
    // try named color format
    QColor c(text);
    return c.rgba();
  }

  bool tryOpenKlfLibEntries(QIODevice *dev)
  {
    // if ONE entry -> load that latex and style
    // if more entries -> warn user, and fail
    QDataStream stream(dev);
    stream.setVersion(QDataStream::Qt_4_4);
    QString headerstr;
    stream >> headerstr;
    if (headerstr != QLatin1String("KLF_LIBENTRIES"))
      return false;
    QVariantMap properties;
    KLFLibEntryList entries;
    stream >> properties >> entries;
    if (entries.size() > 1) {
      QMessageBox::critical(mainWin(), tr("Error"),
			    tr("The data you have request to open contains multiple formulas.\n"
			       "You may only open one formula into the LaTeX code editor."));
      return false;
    }
    if (entries.size() == 0) {
      QMessageBox::critical(mainWin(), tr("Error"),
			    tr("The data you have request to open contains no formulas."));
      return false;
    }
    mainWin()->restoreFromLibrary(entries[0], KLFLib::RestoreAll);
    return true;
  }
};






class KLFAddOnDataOpener : public QObject, public KLFAbstractDataOpener
{
  Q_OBJECT
public:
  KLFAddOnDataOpener(KLFMainWin *mainwin) : QObject(mainwin), KLFAbstractDataOpener(mainwin) { }
  virtual ~KLFAddOnDataOpener() { }

  virtual QStringList supportedMimeTypes()
  {
    return QStringList();
  }

  virtual bool canOpenFile(const QString& file)
  {
    if (QFileInfo(file).suffix() == "rcc")
      return true;
    QFile f(file);
    bool r = f.open(QIODevice::ReadOnly);
    if (!r) { // can't open file
      return false;
    }
    // check if file is RCC file (begins with 'qres')
    if (f.read(4) == "qres")
      return true;
    // not a Qt RCC file
    return false;
  }

  virtual bool canOpenData(const QByteArray& /*data*/)
  {
    // Dropped files are opened by the basic data opener, which handles "text/uri-list"
    // by calling the main window's openFiles()
    return false;
  }

  virtual bool openFile(const QString& file)
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
    klfDbg("file="<<file);

    if (!canOpenFile(file)) {
      klfDbg("file is not openable by us: "<<file);
      return false;
    }

    mainWin()->settingsDialog()->show();
    mainWin()->settingsDialog()->showControl(KLFSettings::ManageAddOns);
    mainWin()->settingsDialog()->importAddOn(file, true);

    return true;
  }

  virtual bool openData(const QByteArray& /*data*/, const QString& /*mimetype*/)
  {
    return false;
  }
};




#endif
