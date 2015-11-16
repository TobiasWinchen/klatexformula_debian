/***************************************************************************
 *   file klfliblegacyengine.h
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
/* $Id: klfliblegacyengine.h 911 2014-08-10 22:24:01Z phfaist $ */

#ifndef KLFLIBLEGACYENGINE_H
#define KLFLIBLEGACYENGINE_H


#include <QDateTime>
#include <QList>
#include <QDataStream>
#include <QPixmap>
#include <QMap>
#include <QMetaType>
#include <QFile>
#include <QTimer>

#include <klfstyle.h>
#include <klflib.h>
#include <klflibview.h> // scheme guesser


/** Legacy data structures for KLatexFormula
 *
 * \warning This class is NOT TO BE USED DIRECTLY, it is OBSOLETE. It has been replaced by the
 *   new KLFLib framework. The "old" "legacy" library format is read/written with KLFLibLegacyEngine.
 */
class KLF_EXPORT KLFLegacyData {
public:

  typedef KLFStyle::KLFLegacyStyle KLFLegacyStyle;


  // THESE VALUES MUST NOT CHANGE FROM ONE VERSION TO ANOTHER OF KLATEXFORMULA :
  enum {
    LibResource_History = 0,
    LibResource_Archive = 1,
    // user resources must be in the following range:
    LibResourceUSERMIN = 100,
    LibResourceUSERMAX = 99999
  };

  struct KLFLibraryResource {
    quint32 id;
    QString name;
  };

  struct KLFLibraryItem {
    quint32 id;
    static quint32 MaxId;

    QDateTime datetime;
    /** \note \c latex contains also information of category (first line, %: ...) and
     * tags (first/second line, after category: % ...) */
    QString latex;
    QPixmap preview;

    QString category;
    QString tags;

    KLFLegacyData::KLFLegacyStyle style;
  };

  typedef QList<KLFLibraryItem> KLFLibraryList;
  typedef QList<KLFLibraryResource> KLFLibraryResourceList;
  typedef QMap<KLFLibraryResource, KLFLibraryList> KLFLibrary;

private:

  KLFLegacyData();
};

// it is important to note that the >> operator imports in a compatible way to KLF 2.0
KLF_EXPORT QDataStream& operator<<(QDataStream& stream, const KLFLegacyData::KLFLibraryItem& item);
KLF_EXPORT QDataStream& operator>>(QDataStream& stream, KLFLegacyData::KLFLibraryItem& item);

KLF_EXPORT QDataStream& operator<<(QDataStream& stream, const KLFLegacyData::KLFLibraryResource& item);
KLF_EXPORT QDataStream& operator>>(QDataStream& stream, KLFLegacyData::KLFLibraryResource& item);

// exact matches, style included, but excluding ID and datetime
KLF_EXPORT bool operator==(const KLFLegacyData::KLFLibraryItem& a, const KLFLegacyData::KLFLibraryItem& b);

// is needed for QMap : these operators compare ID only.
KLF_EXPORT bool operator<(const KLFLegacyData::KLFLibraryResource a, const KLFLegacyData::KLFLibraryResource b);
KLF_EXPORT bool operator==(const KLFLegacyData::KLFLibraryResource a, const KLFLegacyData::KLFLibraryResource b);
// name comparision
KLF_EXPORT bool resources_equal_for_import(const KLFLegacyData::KLFLibraryResource a,
					   const KLFLegacyData::KLFLibraryResource b);



class KLFLibLegacyFileDataPrivate;

//! The Legacy Library support for the KLFLib framework
/** Implements a KLFLibResourceEngine resource engine for accessing (KLF<=3.1)-created libraries
 * (*.klf, default library files)
 *
 * Different legacy resources (in the *.klf file) are mapped to sub-resources (in KLFLibResourceEngine).
 */
class KLF_EXPORT KLFLibLegacyEngine : public KLFLibResourceSimpleEngine
{
  Q_OBJECT
public:
  /** Use this function as a constructor for a KLFLibLegacyEngine object.
   *
   * Opens the URL referenced by url and returns a pointer to a freshly instantiated
   * KLFLibLegacyEngine object, the parent of which is set to \c parent. Returns
   * NULL in case of an error.
   *
   */
  static KLFLibLegacyEngine * openUrl(const QUrl& url, QObject *parent = NULL);

  /** Use this function as a constructor. Creates a KLFLibLegacyEngine object,
   * with QObject parent \c parent, creating a fresh, empty .klf file.
   *
   * Returns NULL if creating the file failed.
   *
   * \c legacyResourceName is the name of an empty (legacy) resource (ie. sub-resource) to create
   * in the newly created file.
   *
   * A non-NULL returned object is linked to a file that was successfully created.
   * */
  static KLFLibLegacyEngine * createDotKLF(const QString& fileName, QString legacyResourceName,
					   QObject *parent = NULL);

  virtual ~KLFLibLegacyEngine();

  virtual uint compareUrlTo(const QUrl& other, uint interestFlags = 0xfffffff) const;

  virtual bool canModifyData(const QString& subRes, ModifyType modifytype) const;
  virtual bool canModifyProp(int propid) const;
  virtual bool canRegisterProperty(const QString& propName) const;

  virtual KLFLibEntry entry(const QString& resource, entryId id);
  virtual QList<KLFLibEntryWithId> allEntries(const QString& resource,
					      const QList<int>& wantedEntryProperties = QList<int>());

  virtual QStringList subResourceList() const;

  virtual bool canCreateSubResource() const;
  virtual bool canRenameSubResource(const QString& subResource) const;
  virtual bool canDeleteSubResource(const QString& subResource) const;

public slots:

  virtual bool createSubResource(const QString& subResource, const QString& subResourceTitle);
  virtual bool renameSubResource(const QString& subResource, const QString& subResourceName);
  virtual bool deleteSubResource(const QString& subResource);

  virtual bool save();
  virtual void setAutoSaveInterval(int intervalms);

  virtual QList<entryId> insertEntries(const QString& subResource, const KLFLibEntryList& entries);
  virtual bool changeEntries(const QString& subResource, const QList<entryId>& idlist,
			     const QList<int>& properties, const QList<QVariant>& values);
  virtual bool deleteEntries(const QString& subResource, const QList<entryId>& idlist);

  virtual bool saveTo(const QUrl& newPath);

protected:
  virtual bool saveResourceProperty(int propId, const QVariant& value);

protected slots:
  void updateResourceProperty(int propId);

private:
  KLFLibLegacyEngine(const QString& fileName, const QString& resname, const QUrl& url, QObject *parent);

  KLFLibLegacyFileDataPrivate *d;
};





class KLF_EXPORT KLFLibLegacyLocalFileSchemeGuesser : public QObject, public KLFLibLocalFileSchemeGuesser
{
public:
  KLFLibLegacyLocalFileSchemeGuesser(QObject *parent) : QObject(parent) { }

  QString guessScheme(const QString& fileName) const;
};


/** The associated factory to the KLFLibDBEngine engine. */
class KLF_EXPORT KLFLibLegacyEngineFactory : public KLFLibEngineFactory
{
  Q_OBJECT
public:
  KLFLibLegacyEngineFactory(QObject *parent = NULL);
  virtual ~KLFLibLegacyEngineFactory() { }

  virtual QStringList supportedTypes() const;
  virtual QString schemeTitle(const QString& scheme) const ;

  virtual uint schemeFunctions(const QString& scheme) const;

  virtual QString correspondingWidgetType(const QString& scheme) const;

  /** Create a library engine that opens resource stored at \c location */
  virtual KLFLibResourceEngine *openResource(const QUrl& location, QObject *parent = NULL);

  virtual KLFLibResourceEngine *createResource(const QString& scheme, const Parameters& parameters,
					       QObject *parent = NULL);
};





#endif
