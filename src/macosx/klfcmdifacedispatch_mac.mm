/***************************************************************************
 *   file klfsingleapplication_mac.mm
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
/* $Id$ */


// Qt5 always uses Cocoa
#define QT_MAC_USE_COCOA 1



#include <QtGlobal>

#ifdef QT_MAC_USE_COCOA

#include <Cocoa/Cocoa.h>

#include <QString>
#include <QTemporaryFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QDir>



//#include <NSRunningApplication.h>

#include <klfdefs.h>
#include "../klfmainwin.h"

#include <klfmacdefs.h>


static bool klf_has_other_app = false;
static QString klf_otherinstance_app_path = QString();


KLF_EXPORT bool klf_mac_find_open_klf()
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  MAC_AUTORELEASE_BLOCK ;

  // use NSRunningApplication
  NSArray *applist =
    [NSRunningApplication runningApplicationsWithBundleIdentifier:@"org.klatexformula.klatexformula"];

  klf_otherinstance_app_path = QString();

  klf_has_other_app = false;

  NSUInteger k = 0;
  while (k < [applist count]) {
    NSRunningApplication *app = [applist objectAtIndex:0];
    if ([app processIdentifier] == [[NSRunningApplication currentApplication] processIdentifier]) {
      // skip this current process!
      ++k;
      continue;
    }
    // we found our app
    klf_has_other_app = true;
    break;
  }

  // return our result
  return klf_has_other_app;
}



KLF_EXPORT bool klf_mac_dispatch_commands(const QList<QUrl>& commands)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

  if (klf_has_other_app == false) {
    qWarning()<<KLF_FUNC_NAME<<": Other KLatexFormula instance not found or not initialized!";
    return false;
  }

  klfDbg("Commands: "<<commands) ;

  //  // create an NSArray with our url list...

  //   NSURL **urlarray = new id[commands.size()];

  //   for (int i = 0; i < commands.size(); ++i) {
  //     NSString *url_s = [NSString stringWithCString:commands[i].toEncoded().constData()
  // 				encoding:NSASCIIStringEncoding];
  //     urlarray[i] = [NSURL URLWithString:url_s];
  //   }

  //   NSArray *urls = [NSArray arrayWithObjects:urlarray count:commands.size()];

  QTemporaryFile tempf(QDir::tempPath()+"/_klf_XXXXXX.klfcommands");
  tempf.setAutoRemove(false);
  tempf.open();
  { // write file contents with a text stream
    QTextStream str(&tempf);
    str << QByteArray("KLFCmdIface/URL: ")+KLF_VERSION_STRING+"\n";

    for (int k = 0; k < commands.size(); ++k) {
      str << commands[k].toEncoded() + "\n";
    }

    str << "_autoremovethisfile\n";
  }
  tempf.close();

  QString appPath;
  if (!klf_otherinstance_app_path.isEmpty()) {
    appPath = klf_otherinstance_app_path;
  } else {
    // find our app path
    appPath = QDir(QCoreApplication::applicationDirPath()+"/../..").canonicalPath();
  }
  if (appPath.endsWith("/"))
    appPath = appPath.mid(0, appPath.length()-1);

  klfDbg("Dispatching open file="<<tempf.fileName()<<", to application="<<appPath) ;

  NSString * appPath_s = [NSString stringWithCString:appPath.toUtf8().constData() encoding:NSUTF8StringEncoding];

  // now send this file to be opened by running app instance
  NSString * ss = [NSString stringWithCString:tempf.fileName().toLocal8Bit().constData()
			    encoding:NSASCIIStringEncoding];
  BOOL result = [[NSWorkspace sharedWorkspace] openFile:ss withApplication:appPath_s];
  bool res = true;
  if (!result)
    res = false;

  /*  BOOL result = [[NSWorkspace sharedWorkspace]
      openURLs:urls
      withAppBundleIdentifier:@"/Applications/klatexformula-3.3.0alpha.app"
      options:NSWorkspaceLaunchDefault
      additionalEventParamDescriptor:nil
      launchIdentifiers:nil];
  */

  //  delete urlarray;

  if (!res) {
    klfDbg("FAIL.") ;
    return false;
  }
  klfDbg("OK.") ;
  return true;
}



#endif //QT_MAC_USE_COCOA
