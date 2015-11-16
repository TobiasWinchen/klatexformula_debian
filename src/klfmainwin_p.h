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
/* $Id: klfmainwin_p.h 909 2014-08-10 17:58:33Z phfaist $ */

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
#include <QTextCodec>

#include <klfutil.h>
#include <klflatexpreviewthread.h>
#include "klflibview.h"
#include "klfmain.h"
#include "klfsettings.h"
#include "klfmime.h"

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



// -------------------------------------------

#ifdef Q_WS_MAC
void klf_mac_win_show_without_activating(QWidget *w);
#endif


/** \internal */
class KLFMainWinPopup : public QLabel
{
  Q_OBJECT
public:
  KLFMainWinPopup(KLFMainWin *mainwin)
    : QLabel(mainwin, Qt::Window|Qt::FramelessWindowHint|Qt::CustomizeWindowHint|
	     Qt::WindowStaysOnTopHint
#ifdef Q_WS_X11
	     |Qt::X11BypassWindowManagerHint
#endif
	     ),
      _mainWin(mainwin)
  {
    setObjectName("KLFMainWinPopup");
    setFocusPolicy(Qt::NoFocus);
    QWidget *frmMain = mainwin->findChild<QWidget*>("frmMain");
    if (frmMain)
      setGeometry(QRect(mainwin->geometry().topLeft()+QPoint(25,mainwin->height()*2/3),
			QSize(frmMain->width()+15, 0)));
    setProperty("klfTopLevelWidget", QVariant(true));
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(mainwin->window()->styleSheet());
    setMargin(0);
    QFont f = font();
    f.setPointSize(QFontInfo(f).pointSize() - 1);
    setFont(f);
    setWordWrap(true);

#ifdef Q_WS_X11
    setAttribute(Qt::WA_X11DoNotAcceptFocus, true);
#endif
    setAttribute(Qt::WA_ShowWithoutActivating, true);

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

#ifdef Q_WS_MAC
    //    klf_mac_win_show_without_activating(this);
    _mainWin->activateWindow();
#endif
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
  KLFMainWin *_mainWin;

  QStringList msgKeys;
  QStringList messages;
};





// -------------------------------------------------


// from klfbackend.cpp
QByteArray klf_escape_ps_string(const QString& v);


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
	  << "text/plain"
	  << "application/pdf" // new!
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
    if (file.endsWith(".klfcommands")) {
      return true;
    }
    if (file.endsWith(".pdf")) {
      /// \todo ONLY IF FILE HAS THE RELEVANT META-INFO. .....
      return true;
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
    if (data.startsWith("%PDF")) {
      if (data.contains("/KLFApplication("))
	return true;
      return false;
    }
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

    if (ext == "klfcommands") {
      // execute commands
      return mainWin()->executeURLCommandsFromFile(file);
    }

    if (ext == "pdf") {
      // read from PDF
      return openPDF(file);
    }

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

    if (mimetype == "text/plain") {
      if (!mainWin()->currentInputLatex().isEmpty()) {
	// do NOT overwrite current editing text with local copy/paste operations
	// inside editor!! (i.e. normal editing inside editor will generate an event here)
	// Simply put, only paste if the editor is empty!
	return false;
      }
      //  mainWin()->loadDefaultStyle(); // this can be annoying if pasting into empty editor...
      mainWin()->slotSetLatex(QString::fromLocal8Bit(data));
      return true;
    }

    if (mimetype == "application/pdf") {
      // can only open file via GS
      return false;
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
      KLFImageLatexMetaInfo metainfo(&img);
      
      QString latex = metainfo.loadField("InputLatex");

      KLFStyle style;
      style.fg_color = read_color(metainfo.loadField("InputFgColor"));
      style.bg_color = read_color(metainfo.loadField("InputBgColor"));
      style.mathmode = metainfo.loadField("InputMathMode");
      style.preamble = metainfo.loadField("InputPreamble");
      style.dpi = metainfo.loadField("InputDPI").toInt();

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

  bool openPDF(const QString& fname)
  {
    if (!QFile::exists(fname)) {
      klfWarning("File "<<fname<<" does not exist!") ;
      return false;
    }

    // Execute gs to extract meta-info
    KLFBackend::klfSettings s = mainWin()->backendSettings();
    KLFFilterProcess p("gs PDF meta-info extraction", &s);

    p.addArgv(s.gsexec);
    p.setProgramCwd(".");
    p.addArgv(QStringList() << "-q" << "-dNOPROMPT" << "-dNODISPLAY" << "-dNOPAUSE");
    p.setOutputStdout(true);
    //    p.setOutputStderr(true);
    
    QByteArray psmetacode =
      klf_escape_ps_string(fname) + " (r) file runpdfbegin Trailer /Info knownoget_safe pop "
      " {exch " // 'Name, Value' becomes 'Value, Name'
      " 64 string cvs print (\\n) print " // print the name, converted to string, along with a newline
      " dup " // twice the name
      " length 16 string cvs print (\\n) print " // print the string length followed by a newline
      " print (\\n) print }" // and print the string itself, followed by a newline.
      " forall\n";
    klfDbg("PS code is "<<QString::fromLatin1(psmetacode)) ;
    QByteArray metaoutput;
    bool ok = p.run(psmetacode, QString(), &metaoutput);
    klfDbg("got output "<<metaoutput) ;
    if (!ok) {
      klfWarning("Failed to read PDF meta-information from file "<<fname<<".\n"
		 "\tError message is "<<p.resultErrorString());
      QMessageBox::critical(mainWin(), tr("Error"), tr("Can't read latex equation information from PDF file."));
      return false;
    }

    // parse all info
    QMap<QString,QString> pdfmeta;

    QBuffer buf(&metaoutput);
    if (!buf.open(QIODevice::ReadOnly)) {
      klfWarning("Can't open internal buffer!");
      return false;
    }

    QTextCodec *u_codec = QTextCodec::codecForName("UTF-16BE");

    QByteArray pdfmetaline;
    while (!(pdfmetaline = buf.readLine()).isEmpty()) {
      QByteArray key = pdfmetaline.trimmed();
      if (key.isEmpty())
	continue;
      // now, read the value length
      bool intok;
      QByteArray vallenstr = buf.readLine().trimmed();
      int vallen = vallenstr.toInt(&intok);
      if (!intok) {
	klfWarning("Internal error: expected value length information after key "<<key<<": "<<vallenstr) ;
	continue;
      }
      QByteArray value = buf.read(vallen);
      /*QByteArray discardline =*/
      buf.readLine(); // discard the following newline.
      /*klfDbg("discarding "<<discardline) ;*/
      // now we got value.
      klfDbg("key="<<key<<" value="<<value) ;
      // get now the string value.
      QString valstr;
      klfDbg("Value[0]=="<<(uchar)value[0]<<" value[1]=="<<(uchar)value[1]<<" 0376="<<(int)(uchar)0376<<".");
      if ((uchar)value[0] == (uchar)0376 && (uchar)value[1] == (uchar)0377) {
	klfDbg("Unicode marks.");
	// unicode marks
	valstr = u_codec->toUnicode(value);
      } else {
	valstr = QString::fromLocal8Bit(value);
      }
      klfDbg("valstr is "<<klfDataToEscaped(valstr.toLocal8Bit()));
      
      pdfmeta[QString::fromLatin1(key)] = valstr;
    }

    if (!pdfmeta.contains("KLFApplication")) {
      klfWarning("The PDF file "<<fname<<" doesn't seem to have KLatexFormula meta-information.");
      return false;
    }

    // now, act according to read meta-information

    QString latex = pdfmeta.value("KLFInputLatex");
    KLFStyle style;
    style.fg_color = read_color(pdfmeta.value("KLFInputFgColor"));
    style.bg_color = read_color(pdfmeta.value("KLFInputBgColor"));
    style.mathmode = pdfmeta.value("KLFInputMathMode");
    style.preamble = pdfmeta.value("KLFInputPreamble");
    style.dpi = pdfmeta.value("KLFInputDPI").toInt();

    mainWin()->slotLoadStyle(style);
    mainWin()->slotSetLatex(latex);
    return true;
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


/** \todo WRITEME!!!!!!!!! */
class KLFKLFOutputSaver : public QObject, public KLFAbstractOutputSaver
{
  Q_OBJECT
public:
};


class KLFTexDataOpener : public QObject, public KLFAbstractDataOpener
{
  Q_OBJECT
public:
  KLFTexDataOpener(KLFMainWin *mainwin)
    : QObject(mainwin), KLFAbstractDataOpener(mainwin)
  {
  }

  virtual ~KLFTexDataOpener()
  {
  }

  virtual QStringList supportedMimeTypes()
  {
    return QStringList();
  }

  virtual bool canOpenFile(const QString& file)
  {
    QFileInfo fi(file);
    if (fi.suffix() == "tex" || fi.suffix() == "latex" || fi.suffix() == "klftex")
      return true;
    // not a TeX file
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

    // filter out and parse our special comments
    // the rest is LaTeX code to paste

    QFile f(file);
    if (!f.open(QIODevice::ReadOnly)) {
      QMessageBox::critical(mainWin(), tr("Error"), tr("Can't open file %1. Read Error.").arg(file));
      return false;
    }

    QByteArray latexdata;
    QByteArray styledata;
    KLFStyle style;

    QByteArray line;
    while (!(line = f.readLine()).isNull()) {
      // see if it is one of our special comments
      if (line.startsWith("%%KLF:style: ")) {
	// extract style data line
	styledata += line.mid(strlen("%%KLF:style: "));
      } else if (line.startsWith("%%KLF:")) {
	// ignore this unused KLF comment
      } else {
	// LaTeX line
	latexdata += line;
      }
    }

    bool ok = klfLoad(styledata, &style); //,"XML");  ... format is guessed from data
    if (!ok) {
      klfWarning("Unable to load style from comment data: "<<styledata) ;
    }

    mainWin()->slotLoadStyle(style);
    mainWin()->slotSetLatex(QString::fromLocal8Bit(latexdata));

    return true;
  }

  virtual bool openData(const QByteArray& /*data*/, const QString& /*mimetype*/)
  {
    return false;
  }

  
};


class KLFTexOutputSaver : public QObject, public KLFAbstractOutputSaver
{
  Q_OBJECT
public:
  KLFTexOutputSaver(QObject *parent)
    : QObject(parent), KLFAbstractOutputSaver()
  {
  }

  virtual ~KLFTexOutputSaver()
  {
  }

  virtual QStringList supportedMimeFormats(KLFBackend::klfOutput * output)
  {
    Q_UNUSED(output);

    KLF_ASSERT_NOT_NULL(output, "output pointer is NULL!", return QStringList() );

    return QStringList() << QLatin1String("text/tex");
  }

  /** Returns the human-readable, (possibly translated,) label to display in save dialog that
   * the user can select to save in this format.
   *
   * \param key is a mime-type returned by \ref supportedMimeFormats().
   */
  virtual QString formatTitle(const QString& key)
  {
    Q_UNUSED(key);
    return tr("LaTeX source");
  }

  virtual QStringList formatFilePatterns(const QString& key)
  {
    Q_UNUSED(key);
    return QStringList() << "*.klftex" << "*.tex";
  }

  virtual bool saveToFile(const QString& key, const QString& fileName, const KLFBackend::klfOutput& output)
  {
    Q_UNUSED(key);

    QByteArray data = "%%KLF:LaTeX-save\n";
    data += "%%KLF:date: "+QDateTime::currentDateTime().toString(Qt::ISODate) + "\n%%KLF: \n";

    data += output.input.latex.toUtf8();
    if (!data.endsWith("\n"))
      data += "\n";
    data += "%%KLF: \n";
    data += "%%KLF: ";

    // save style now as a LaTeX comment
    KLFStyle style(output.input);
    style.userScript = QFileInfo(style.userScript).fileName(); // only save file name as script path may differ
    QByteArray styledata = klfSave(&style, "XML");
    styledata = "\n"+styledata;
    styledata.replace("\n", "\n%%KLF:style: ");

    data += styledata + "\n";


    // and write to file:

    QFile f(fileName);
    bool r = f.open(QIODevice::WriteOnly);
    if (!r) {
      QMessageBox::critical(NULL, tr("Error"), tr("Failed to write file %1").arg(fileName));
      qWarning()<<KLF_FUNC_NAME<<": Failed to write to file "<<fileName;
      return false;
    }
    f.write(data);
    return true;
  }

};




class KLFUserScriptOutputSaver : public QObject, public KLFAbstractOutputSaver
{
  Q_OBJECT
public:
  KLFUserScriptOutputSaver(const QString& userscript, QObject *parent)
    : QObject(parent), KLFAbstractOutputSaver(), pUserScript(userscript, NULL)
  {
  }

  virtual ~KLFUserScriptOutputSaver()
  {
  }

  virtual QStringList supportedMimeFormats(KLFBackend::klfOutput * output)
  {
    return pUserScript.availableMimeTypes(output);
  }

  /** Returns the human-readable, (possibly translated,) label to display in save dialog that
   * the user can select to save in this format.
   *
   * \param key is a mime-type returned by \ref supportedMimeFormats().
   */
  virtual QString formatTitle(const QString& key)
  {
    int i = pUserScript.info().findMimeType(key);
    return pUserScript.info().outputFormatDescription(i);
  }

  virtual QStringList formatFilePatterns(const QString& key)
  {
    int i = pUserScript.info().findMimeType(key);
    return QStringList() << "*."+pUserScript.info().outputFilenameExtension(i);
  }

  virtual bool saveToFile(const QString& key, const QString& fileName, const KLFBackend::klfOutput& output)
  {
    QByteArray data = pUserScript.getData(key, output);
    if (!data.size()) {
      klfWarning("User Script "<<pUserScript.info().scriptName()<<": Error occurred while trying to get data.") ;
      return false;
    }

    QFile f(fileName);
    bool r = f.open(QIODevice::WriteOnly);
    if (!r) {
      QMessageBox::critical(NULL, tr("Error"), tr("Failed to write file %1").arg(fileName));
      qWarning()<<KLF_FUNC_NAME<<": Failed to write to file "<<fileName;
      return false;
    }
    f.write(data);
    return true;
  }

private:
  KLFExportUserScript pUserScript;
};








// --------------------------------------------------------------------------


class KLFMainWinPrivate : public QObject
{
  Q_OBJECT
public:
  KLF_PRIVATE_QOBJ_HEAD(KLFMainWin, QObject)
  {
    pMacOSXMenu = NULL;
    pCheckForUpdatesAction = NULL;
  }


  KLFLibBrowser *mLibBrowser;
  KLFLatexSymbols *mLatexSymbols;
  KLFStyleManager *mStyleManager;
  KLFSettings *mSettingsDialog;
  KLFAboutDialog *mAboutDialog;
  KLFWhatsNewDialog *mWhatsNewDialog;

  KLFMainWinPopup *mPopup;

  QMenuBar *pMacOSXMenu;
  QAction *pCheckForUpdatesAction;

  QShortcut * mShortcutNextParenType;
  QShortcut * mShortcutNextParenModifierType;

  /** \internal */
  struct HelpLinkAction {
    HelpLinkAction(const QString& p, QObject *obj, const char *func, bool param)
      : path(p), reciever(obj), memberFunc(func), wantParam(param) { }
    QString path;
    QObject *reciever;
    QByteArray memberFunc;
    bool wantParam;
  };
  QList<HelpLinkAction> mHelpLinkActions;

  KLFLibResourceEngine *mHistoryLibResource;

  KLFStyleList styles;

  bool try_load_style_list(const QString& fileName);

  QMenu *mStyleMenu;

  bool loadedlibrary;
  bool firstshow;

  KLFBackend::klfSettings settings; // settings we pass to KLFBackend
  bool settings_altered;

  KLFBackend::klfOutput output; // output from KLFBackend

  /** If TRUE, then the output contained in _output is up-to-date, meaning that we favor displaying
   * _output.result instead of the image given by mPreviewBuilderThread. */
  bool evaloutput_uptodate;
  /** The Thread that will create real-time previews of formulas. */
  KLFLatexPreviewThread *pLatexPreviewThread;
  KLFContLatexPreview *pContLatexPreview;

  QLabel *mExportMsgLabel;
  void showExportMsgLabel(const QString& msg, int timeout = 3000);
  int pExportMsgLabelTimerId;

  KLFUserScriptSettings * pUserScriptSettings;

  QHash<QString,QWidget*> userScriptInputWidgets;
  QString userScriptCurrentInfo;
  QWidget * getUserScriptInputWidget(const QString& uifile);
  QVariantMap collectUserScriptInput() const;

  /**
   * Use \ref currentInputState() instead for "public" use.
   *
   * Returns the input corresponding to the current GUI state. If \c isFinal is TRUE, then
   * the input data may be "remembered" as used (the exact effect depends on the setting), eg.
   * math mode is memorized into combo box choices. Typically \c isFinal is TRUE when called
   * from slotEvaluate() and FALSE when called to update the preview builder thread. */
  KLFBackend::klfInput collectInput(bool isFinal);

  QList<QAction*> pExportProfileQuickMenuActionList;

  bool ignore_close_event;

  //   /** "last" window status flags are used in eventFilter() to detect individual dialog
  //    * geometries resetting */
  //   QHash<QWidget*,bool> pLastWindowShownStatus;
  //   QHash<QWidget*,QRect> pLastWindowGeometries;
  //   /** "saved" window status flags are used in hideEvent() to save the individual dialog visible
  //    * states, as the "last" status flags will be overridden by all the windows hiding. */
  //   QHash<QWidget*,bool> pSavedWindowShownStatus;

  QString widgetstyle;

  void getMissingCmdsFor(const QString& symbol, QStringList * missingCmds, QString *guiText,
			 bool wantHtmlText = true);

  QList<KLFAbstractOutputSaver*> pOutputSavers;
  QList<KLFAbstractDataOpener*> pDataOpeners;

  KLFCmdIface *pCmdIface;

  QVariantMap parseLatexEditPosParenInfo(KLFLatexEdit *editor, int pos);

  struct LatexFontDef
  {
    QString identifier;
    QString title;
    QString latexdefs;

    LatexFontDef(const QString& identifier_ = QString(), const QString& title_ = QString(),
                 const QString& latexdefs_ = QString())
      : identifier(identifier_), title(title_), latexdefs(latexdefs_)
    {
    }
  };

  QList<LatexFontDef> pLatexFontDefs;

  void reloadLatexFontDefs();

signals:

  void saveApplicationState(QSettings * settings);



public slots: // .. but in private API

  // private : only as slot to an action containing the style # as user data
  void slotLoadStyleAct();

  void slotDetailsSideWidgetShown(bool shown);

  /** controls the enabled state of the 'see larger preview button' widget */
  void slotSetViewControlsEnabled(bool enabled);
  /** controls the enabled state of the DRAG/COPY/SAVE & Format widgets */
  void slotSetSaveControlsEnabled(bool enabled);

  void slotOpenHistoryLibraryResource();

  void refreshShowCorrectClearButton();

  void refreshExportTypesMenu();

  void refreshStylePopupMenus();

  void slotLibraryButtonRefreshState(bool on);
  void slotSymbolsButtonRefreshState(bool on);

  void slotPresetDPISender();

  void showRealTimeReset();
  void showRealTimePreview(const QImage& preview, const QImage& largePreview);
  void showRealTimeError(const QString& errorstr, int errcode);

  void updatePreviewThreadInput();
  void updatePreviewThreadSettings();

  void displayError(const QString& errormsg);

  void slotNewSymbolTyped(const QString& symbol);
  void slotPopupClose();
  void slotPopupAction(const QUrl& helpLinkUrl);
  void slotPopupAcceptAll();

  void slotEditorContextMenuInsertActions(const QPoint& pos, QList<QAction*> *actionList);
  void slotInsertMissingPackagesFromActionSender();
  void slotChangeParenFromActionSender();
  void slotChangeParenFromVariantMap(const QVariantMap& data);

  void slotCycleParenModifiers(bool forward = true);
  void slotCycleParenModifiersBack() { slotCycleParenModifiers(false); }
  void slotCycleParenTypes(bool forward = true);
  void slotCycleParenTypesBack() { slotCycleParenTypes(false); }

  void slotEncloseRegionInDelimiterFromActionSender();
  void slotEncloseRegionInDelimiter(const QVariantMap& vmap);

  void latexEditReplace(int pos, int len, const QString& text);

  void slotUserScriptSet(int index);
  void slotUserScriptShowInfo();
  void slotUserScriptDisableInputs(KLFUserScriptInfo * info);
};





#endif
