/***************************************************************************
 *   file klflib.cpp
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
/* $Id: klflib.cpp 698 2011-08-08 08:28:12Z phfaist $ */

#include <QDebug>
#include <QString>
#include <QBuffer>
#include <QByteArray>
#include <QDataStream>
#include <QColor>
#include <QMimeData>

#include <klfutil.h>
#include "klflib_p.h"
#include "klflib.h"




// issue a warning if no default sub-resource is set
#define KLFLIBRESOURCEENGINE_WARN_NO_DEFAULT_SUBRESOURCE(func)		\
  if ((pFeatureFlags & FeatureSubResources) && pDefaultSubResource.isNull()) { \
    qWarning("KLFLibResourceEngine::" func "(id): sub-resources are supported feature but" \
	     " no default sub-resource is specified!"); }		\



// ----------------------


KLFLibEntry::KLFLibEntry(const QString& latex, const QDateTime& dt, const QImage& preview,
			 const QSize& previewsize, const QString& category, const QString& tags,
			 const KLFStyle& style)
  : KLFPropertizedObject("KLFLibEntry")
{
  initRegisteredProperties();
  setLatex(latex);
  setDateTime(dt);
  setPreview(preview);
  setPreviewSize(previewsize);
  setCategory(category);
  setTags(tags);
  setStyle(style);
}
KLFLibEntry::KLFLibEntry(const QString& latex, const QDateTime& dt, const QImage& preview,
			 const KLFStyle& style)
  : KLFPropertizedObject("KLFLibEntry")
{
  initRegisteredProperties();
  QString latexonly = stripCategoryTagsFromLatex(latex);
  QString category = categoryFromLatex(latex);
  QString tags = tagsFromLatex(latex);
  QSize previewsize = preview.size();
  setLatex(latexonly);
  setDateTime(dt);
  setPreview(preview);
  setPreviewSize(previewsize);
  setCategory(category);
  setTags(tags);
  setStyle(style);
}
KLFLibEntry::KLFLibEntry(const KLFLibEntry& copy)
  : KLFPropertizedObject("KLFLibEntry")
{
  initRegisteredProperties();
  setAllProperties(copy.allProperties());
}
KLFLibEntry::~KLFLibEntry()
{
}


int KLFLibEntry::setEntryProperty(const QString& propName, const QVariant& value)
{
  //   int propId = propertyIdForName(propName);
  //   if (propId < 0) {
  //     // register the property
  //     propId = registerProperty(propName);
  //     if (propId < 0)
  //       return -1;
  //   }
  //   // and set the property
  //   setProperty(propId, value);
  //   return propId;
  // call KLFPropertizedObject's setProperty() to do the job for us
  bool ok = setProperty(propName, value);
  if (!ok)
    return -1;
  return propertyIdForName(propName);
}

// private, static
void KLFLibEntry::initRegisteredProperties()
{
  KLF_FUNC_SINGLE_RUN ;
  
  registerBuiltInProperty(Latex, "Latex");
  registerBuiltInProperty(DateTime, "DateTime");
  registerBuiltInProperty(Preview, "Preview");
  registerBuiltInProperty(PreviewSize, "PreviewSize");
  registerBuiltInProperty(Category, "Category");
  registerBuiltInProperty(Tags, "Tags");
  registerBuiltInProperty(Style, "Style");
}


// static
QString KLFLibEntry::categoryFromLatex(const QString& latex)
{
  QString s = latex.section('\n', 0, 0, QString::SectionSkipEmpty);
  if (s[0] == '%' && s[1] == ':') {
    return s.mid(2).trimmed();
  }
  return QString::null;
}
// static
QString KLFLibEntry::tagsFromLatex(const QString& latex)
{
  QString s = latex.section('\n', 0, 0, QString::SectionSkipEmpty);
  if (s[0] == '%' && s[1] == ':') {
    // category is s.mid(2);
    s = latex.section('\n', 1, 1, QString::SectionSkipEmpty);
  }
  if (s[0] == '%') {
    return s.mid(1).trimmed();
  }
  return QString::null;
}

// static
QString KLFLibEntry::stripCategoryTagsFromLatex(const QString& latex)
{
  int k = 0;
  while (k < latex.length() && latex[k].isSpace())
    ++k;
  if (k == latex.length()) return "";
  if (latex[k] == '%') {
    ++k;
    if (k == latex.length()) return "";
    //strip category and/or tag:
    if (latex[k] == ':') {
      // strip category
      while (k < latex.length() && latex[k] != '\n')
	++k;
      ++k;
      if (k >= latex.length()) return "";
      if (latex[k] != '%') {
	// there isn't any tags, just category; return rest of string
	return latex.mid(k);
      }
      ++k;
      if (k >= latex.length()) return "";
    }
    // strip tag:
    while (k < latex.length() && latex[k] != '\n')
      ++k;
    ++k;
    if (k >= latex.length()) return "";
  }
  // k is the beginnnig of the latex string
  return latex.mid(k);
}

// static
QString KLFLibEntry::latexAddCategoryTagsComment(const QString& latex, const QString& category,
						 const QString& tags)
{
  QString s;

  if (!category.isEmpty())
    s = "%: "+category+"\n";

  if (!tags.isEmpty())
    s += "% "+tags+"\n";

  s += latex;
  return s;
}

// static
QString KLFLibEntry::normalizeCategoryPath(const QString& categoryPath)
{
  QString c = categoryPath.trimmed().split('/', QString::SkipEmptyParts).join("/");
  if (c.endsWith("/"))
    c.chop(1);
  return c;
}



// ------------------------------------------------------------



KLFLibEntrySorter::KLFLibEntrySorter(int propId, Qt::SortOrder order)
  : pCloneOf(NULL), pPropId(propId), pOrder(order)
{
}
KLFLibEntrySorter::KLFLibEntrySorter(const KLFLibEntrySorter *clone)
  : pCloneOf(clone), pPropId(clone->pPropId), pOrder(clone->pOrder)
{
}

KLFLibEntrySorter::~KLFLibEntrySorter()
{
}

void KLFLibEntrySorter::setPropId(int propId)
{
  if (pCloneOf != NULL) {
    qWarning()<<"Attempt to setPropId() in entry sorter that is a clone of "<<pCloneOf;
    return;
  }
  pPropId = propId;
}
void KLFLibEntrySorter::setOrder(Qt::SortOrder order)
{
  if (pCloneOf != NULL) {
    qWarning()<<"Attempt to setOrder() in entry sorter that is a clone of "<<pCloneOf;
    return;
  }
  pOrder = order;
}



QString KLFLibEntrySorter::entryValue(const KLFLibEntry& entry, int propId) const
{
  if (pCloneOf != NULL)
    return pCloneOf->entryValue(entry, propId);

  // return an internal string representation of the value of the property 'propId' in libentry 'entry'

  // user friendliness. sort by date when selecting preview.
  if (propId == KLFLibEntry::Preview)
    propId = KLFLibEntry::DateTime;

  if (propId == KLFLibEntry::PreviewSize) {
    QSize s = entry.previewSize();
    // eg. "0000000280,0000000180" for 280x180
    return QString("%1,%2").arg(s.width(), 10, 10, QChar('0')).arg(s.height(), 10, 10, QChar('0'));
  }
  if (propId == KLFLibEntry::DateTime) {
    return entry.property(KLFLibEntry::DateTime).toDateTime().toString("yyyy-MM-dd+hh:mm:ss.zzz");
  }
  return entry.property(propId).toString();
}

bool KLFLibEntrySorter::compareLessThan(const KLFLibEntry& a, const KLFLibEntry& b,
					int propId, Qt::SortOrder order) const
{
  if (pCloneOf != NULL)
    return pCloneOf->compareLessThan(a, b, propId, order);

  QString as = entryValue(a, propId);
  QString bs = entryValue(b, propId);
  if (order == Qt::AscendingOrder)
    return QString::localeAwareCompare(as, bs) < 0;
  return QString::localeAwareCompare(as, bs) > 0;
}

bool KLFLibEntrySorter::operator()(const KLFLibEntry& a, const KLFLibEntry& b) const
{
  if (pCloneOf != NULL)
    return pCloneOf->operator()(a, b);

  return compareLessThan(a, b, pPropId, pOrder);
}


// ---------------------------------------------------

KLFAbstractLibEntryMimeEncoder::KLFAbstractLibEntryMimeEncoder()
{
  registerEncoder(this);
}
KLFAbstractLibEntryMimeEncoder::~KLFAbstractLibEntryMimeEncoder()
{
}
void KLFAbstractLibEntryMimeEncoder::registerEncoder(KLFAbstractLibEntryMimeEncoder *encoder)
{
  staticEncoderList.append(encoder);
}

QList<KLFAbstractLibEntryMimeEncoder*> KLFAbstractLibEntryMimeEncoder::encoderList()
{
  return staticEncoderList;
}

// static
QStringList KLFAbstractLibEntryMimeEncoder::allEncodingMimeTypes()
{
  QStringList encTypes;
  int k;
  for (k = 0; k < staticEncoderList.size(); ++k) {
    encTypes << staticEncoderList[k]->supportedEncodingMimeTypes();
  }
  return encTypes;
}
// static
QStringList KLFAbstractLibEntryMimeEncoder::allDecodingMimeTypes()
{
  QStringList decTypes;
  int k;
  for (k = 0; k < staticEncoderList.size(); ++k) {
    decTypes << staticEncoderList[k]->supportedDecodingMimeTypes();
  }
  return decTypes;
}

// static
QMimeData *KLFAbstractLibEntryMimeEncoder::createMimeData(const KLFLibEntryList& entryList,
							  const QVariantMap& metaData)
{
  QMimeData *mime = new QMimeData;
  int k, j;
  for (k = 0; k < staticEncoderList.size(); ++k) {
    QStringList mimeTypeList = staticEncoderList[k]->supportedEncodingMimeTypes();
    for (j = 0; j < mimeTypeList.size(); ++j) {
      QByteArray data =
	staticEncoderList[k]->encodeMime(entryList, metaData, mimeTypeList[j]);
      if (data.isEmpty()) {
	klfDbg("Skipping mime type "<<mimeTypeList[k]<<" because it did not provide any data.");
      } else {
	mime->setData(mimeTypeList[j], data);
      }
    }
  }
  return mime;
}


// static
bool KLFAbstractLibEntryMimeEncoder::canDecodeMimeData(const QMimeData *mimeData)
{
  QStringList fmts = mimeData->formats();
  int k;
  for (k = 0; k < fmts.size(); ++k) {
    if (findDecoderFor(fmts[k], false) != NULL)
      return true;
  }
  return false;
}

// static
bool KLFAbstractLibEntryMimeEncoder::decodeMimeData(const QMimeData *mimeData,
						    KLFLibEntryList *entryListPtr,
						    QVariantMap *metaDataPtr)
{
  QStringList fmts = mimeData->formats();
  int k;
  for (k = 0; k < fmts.size(); ++k) {
    KLFAbstractLibEntryMimeEncoder *decoder = findDecoderFor(fmts[k], false);
    if (decoder == NULL)
      continue;
    bool result = decoder->decodeMime(mimeData->data(fmts[k]), fmts[k], entryListPtr, metaDataPtr);
    if ( result )
      return true;
    // else continue trying
  }
  return false;
}



KLFAbstractLibEntryMimeEncoder *KLFAbstractLibEntryMimeEncoder::findEncoderFor(const QString& mimeType,
									       bool warn)
{
  int k;
  for (k = 0; k < staticEncoderList.size(); ++k)
    if (staticEncoderList[k]->supportedEncodingMimeTypes().contains(mimeType))
      return staticEncoderList[k];
  if (warn)
    qWarning()<<KLF_FUNC_NAME<<": Failed to find encoder for mime-type "<<mimeType;
  return NULL;
}

KLFAbstractLibEntryMimeEncoder *KLFAbstractLibEntryMimeEncoder::findDecoderFor(const QString& mimeType,
									       bool warn)
{
  int k;
  for (k = 0; k < staticEncoderList.size(); ++k)
    if (staticEncoderList[k]->supportedDecodingMimeTypes().contains(mimeType))
      return staticEncoderList[k];
  if (warn)
    qWarning()<<KLF_FUNC_NAME<<": Failed to find decoder for mime-type "<<mimeType;
  return NULL;
}


QList<KLFAbstractLibEntryMimeEncoder*> KLFAbstractLibEntryMimeEncoder::staticEncoderList;




// The instance of the basic encoder, that will auto-register itself
KLFLibEntryMimeEncoder __klf_lib_mime_encoder;


// ---------------------------------------------------

KLFLibResourceEngine::KLFLibResourceEngine(const QUrl& url, uint featureflags,
					   QObject *parent)
  : QObject(parent), KLFPropertizedObject("KLFLibResourceEngine"), pUrl(url),
    pFeatureFlags(featureflags), pReadOnly(false), pDefaultSubResource(QString()),
    pProgressBlocked(false), pThisOperationProgressBlockedOnly(false)
{
  initRegisteredProperties();

  //  klfDbg( "KLFLibResourceEngine::KLFLibResourceEngine("<<url<<","<<pFeatureFlags<<","
  //	  <<parent<<")" ) ;

  QStringList rdonly = pUrl.allQueryItemValues("klfReadOnly");
  if (rdonly.size() && rdonly.last() == "true") {
    if (pFeatureFlags & FeatureReadOnly)
      pReadOnly = true;
  }
  pUrl.removeAllQueryItems("klfReadOnly");

  if (pFeatureFlags & FeatureSubResources) {
    QStringList defaultsubresource = pUrl.allQueryItemValues("klfDefaultSubResource");
    if (!defaultsubresource.isEmpty()) {
      pUrl.removeAllQueryItems("klfDefaultSubResource");
      pDefaultSubResource = defaultsubresource.last();
    }
  }

}
KLFLibResourceEngine::~KLFLibResourceEngine()
{
}

void KLFLibResourceEngine::initRegisteredProperties()
{
  KLF_FUNC_SINGLE_RUN

  registerBuiltInProperty(PropTitle, "Title");
  registerBuiltInProperty(PropLocked, "Locked");
  registerBuiltInProperty(PropViewType, "ViewType");
  registerBuiltInProperty(PropAccessShared, "AccessShared");
}

QUrl KLFLibResourceEngine::url(uint flags) const
{
  QUrl url = pUrl;
  if (flags & WantUrlDefaultSubResource &&
      (pFeatureFlags & FeatureSubResources) &&
      !pDefaultSubResource.isNull()) {
    url.addQueryItem("klfDefaultSubResource", pDefaultSubResource);
  }
  if (flags & WantUrlReadOnly) {
    url.addQueryItem("klfReadOnly", pReadOnly?QString("true"):QString("false"));
  }
  return url;
}


bool KLFLibResourceEngine::canModifyData(const QString& subResource,
					 ModifyType /*modifytype*/) const
{
  return baseCanModifyStatus(true, subResource) == MS_CanModify;
}

bool KLFLibResourceEngine::canModifyData(ModifyType modifytype) const
{
  KLFLIBRESOURCEENGINE_WARN_NO_DEFAULT_SUBRESOURCE("canModifyData")
  return canModifyData(pDefaultSubResource, modifytype);
}


bool KLFLibResourceEngine::canModifyProp(int propId) const
{
  ModifyStatus ms = baseCanModifyStatus(false);
  return ms == MS_CanModify ||
    (ms == MS_IsLocked && propId == PropLocked); // allow un-locking (!)
}
bool KLFLibResourceEngine::canRegisterProperty(const QString& /*propName*/) const
{
  return false;
}

bool KLFLibResourceEngine::hasSubResource(const QString& subResource) const
{
  if ( !(pFeatureFlags & FeatureSubResources) )
    return false;

  return subResourceList().contains(subResource);
}

QString KLFLibResourceEngine::defaultSubResource() const
{
  return pDefaultSubResource;
}

bool KLFLibResourceEngine::compareDefaultSubResourceEquals(const QString& subResourceName) const
{
  return QString::compare(pDefaultSubResource, subResourceName) == 0;
}

bool KLFLibResourceEngine::canCreateSubResource() const
{
  return false;
}

bool KLFLibResourceEngine::canRenameSubResource(const QString& /*subResource*/) const
{
  return false;
}
bool KLFLibResourceEngine::canDeleteSubResource(const QString& /*subResource*/) const
{
  return false;
}

QVariant KLFLibResourceEngine::subResourceProperty(const QString& /*subResource*/, int /*propId*/) const
{
  return QVariant();
}

QString KLFLibResourceEngine::subResourcePropertyName(int propId) const
{
  switch (propId) {
  case SubResPropTitle:
    return QLatin1String("Title");
  case SubResPropLocked:
    return QLatin1String("Locked");
  case SubResPropViewType:
    return QLatin1String("ViewType");
  default:
    ;
  }
  return QString::number(propId);
}

bool KLFLibResourceEngine::canModifySubResourceProperty(const QString& subResource, int propId) const
{
  ModifyStatus ms = baseCanModifyStatus(true, subResource);
  return ms == MS_CanModify ||
    (ms == MS_SubResLocked && propId == SubResPropLocked); // allow sub-resource un-locking
}

bool KLFLibResourceEngine::setTitle(const QString& title)
{
  return setResourceProperty(PropTitle, title);
}
bool KLFLibResourceEngine::setLocked(bool setlocked)
{
  // if locked feature is supported, setResourceProperty().
  // immediately return FALSE otherwise.
  if (pFeatureFlags & FeatureLocked) {
    return setResourceProperty(PropLocked, setlocked);
  }
  return false;
}

bool KLFLibResourceEngine::setViewType(const QString& viewType)
{
  return setResourceProperty(PropViewType, viewType);
}

bool KLFLibResourceEngine::setReadOnly(bool readonly)
{
  if ( !(pFeatureFlags & FeatureReadOnly) )
    return false;

  pReadOnly = readonly;
  return true;
}


void KLFLibResourceEngine::setDefaultSubResource(const QString& subResource)
{
  if (pDefaultSubResource == subResource)
    return;

  pDefaultSubResource = subResource;
  emit defaultSubResourceChanged(subResource);
}

bool KLFLibResourceEngine::setSubResourceProperty(const QString& /*subResource*/, int /*propId*/,
						  const QVariant& /*value*/)
{
  return false;
}

bool KLFLibResourceEngine::createSubResource(const QString& /*subResource*/,
					     const QString& /*subResourceTitle*/)
{
  return false;
}
bool KLFLibResourceEngine::createSubResource(const QString& subResource)
{
  return createSubResource(subResource, QString());
}
bool KLFLibResourceEngine::renameSubResource(const QString& /*old*/, const QString& /*new*/)
{
  return false;
}
bool KLFLibResourceEngine::deleteSubResource(const QString& /*subResource*/)
{
  return false;
}


KLFLibEntry KLFLibResourceEngine::entry(entryId id)
{
  KLFLIBRESOURCEENGINE_WARN_NO_DEFAULT_SUBRESOURCE("entry");
  return entry(pDefaultSubResource, id);
}
bool KLFLibResourceEngine::hasEntry(entryId id)
{
  KLFLIBRESOURCEENGINE_WARN_NO_DEFAULT_SUBRESOURCE("hasEntry");
  return hasEntry(pDefaultSubResource, id);
}
QList<KLFLibResourceEngine::KLFLibEntryWithId>
/* */ KLFLibResourceEngine::entries(const QList<KLFLib::entryId>& idList,
				    const QList<int>& wantedEntryProperties)
{
  KLFLIBRESOURCEENGINE_WARN_NO_DEFAULT_SUBRESOURCE("entries");
  return entries(pDefaultSubResource, idList, wantedEntryProperties);
}

QList<KLFLibResourceEngine::KLFLibEntryWithId>
/* */ KLFLibResourceEngine::allEntries(const QList<int>& wantedEntryProperties)
{
  KLFLIBRESOURCEENGINE_WARN_NO_DEFAULT_SUBRESOURCE("allEntries");
  return allEntries(pDefaultSubResource, wantedEntryProperties);
}
QList<KLFLib::entryId> KLFLibResourceEngine::allIds()
{
  KLFLIBRESOURCEENGINE_WARN_NO_DEFAULT_SUBRESOURCE("allIds");
  return allIds(pDefaultSubResource);
}

void KLFLibResourceEngine::blockProgressReportingForNextOperation()
{
  pProgressBlocked = true;
  pThisOperationProgressBlockedOnly = true;
}

void KLFLibResourceEngine::blockProgressReporting(bool block)
{
  pProgressBlocked = block;
  pThisOperationProgressBlockedOnly = false;
}

bool KLFLibResourceEngine::thisOperationProgressBlocked() const
{
  bool blocked = pProgressBlocked;
  if (pThisOperationProgressBlockedOnly)
    pProgressBlocked = false; // reset for next operation
  return blocked;
}


KLFLibResourceEngine::entryId KLFLibResourceEngine::insertEntry(const QString& subResource,
								const KLFLibEntry& entry)
{
  QList<entryId> ids = insertEntries(subResource, KLFLibEntryList() << entry);
  if (ids.size() == 0)
    return -1;

  return ids[0];
}
KLFLibResourceEngine::entryId KLFLibResourceEngine::insertEntry(const KLFLibEntry& entry)
{ 
  KLFLIBRESOURCEENGINE_WARN_NO_DEFAULT_SUBRESOURCE("insertEntry");
  return insertEntry(pDefaultSubResource, entry);
}
QList<KLFLibResourceEngine::entryId> KLFLibResourceEngine::insertEntries(const KLFLibEntryList& entrylist)
{
  KLFLIBRESOURCEENGINE_WARN_NO_DEFAULT_SUBRESOURCE("insertEntries");
  return insertEntries(pDefaultSubResource, entrylist);
}

bool KLFLibResourceEngine::changeEntries(const QList<entryId>& idlist, const QList<int>& properties,
					 const QList<QVariant>& values)
{
  KLFLIBRESOURCEENGINE_WARN_NO_DEFAULT_SUBRESOURCE("changeEntries");
  return changeEntries(pDefaultSubResource, idlist, properties, values);
}

bool KLFLibResourceEngine::deleteEntries(const QList<entryId>& idList)
{
  KLFLIBRESOURCEENGINE_WARN_NO_DEFAULT_SUBRESOURCE("deleteEntries");
  return deleteEntries(pDefaultSubResource, idList);
}



bool KLFLibResourceEngine::saveTo(const QUrl&)
{
  // not implemented by default. Subclasses must reimplement
  // to support this feature.
  return false;
}

QVariant KLFLibResourceEngine::resourceProperty(const QString& name) const
{
  return KLFPropertizedObject::property(name);
}

bool KLFLibResourceEngine::loadResourceProperty(const QString& propName, const QVariant& value)
{
  int propId = propertyIdForName(propName);
  if (propId < 0) {
    if (!canRegisterProperty(propName))
      return false;
    // register the property
    propId = registerProperty(propName);
    if (propId < 0)
      return false;
  }
  // finally set the property
  return setResourceProperty(propId, value);
}

bool KLFLibResourceEngine::setResourceProperty(int propId, const QVariant& value)
{
  if ( ! KLFPropertizedObject::propertyIdRegistered(propId) )
    return false;

  if ( !canModifyProp(propId) ) {
    return false;
  }

  bool result = saveResourceProperty(propId, value);
  if (!result) // operation not permitted or failed
    return false;

  // operation succeeded: set KLFPropertizedObject-based property.
  KLFPropertizedObject::doSetProperty(propId, value);
  emit resourcePropertyChanged(propId);
  return true;
}


KLFLibResourceEngine::ModifyStatus
/* */ KLFLibResourceEngine::baseCanModifyStatus(bool inSubResource, const QString& subResource) const
{
  if (pFeatureFlags & FeatureLocked) {
    if (locked())
      return MS_IsLocked;
    if (inSubResource &&
	(pFeatureFlags & FeatureSubResources) &&
	(pFeatureFlags & FeatureSubResourceProps)) {
      if (subResourceProperty(subResource, SubResPropLocked).toBool())
	return MS_SubResLocked;
    }
  }

  if (pFeatureFlags & FeatureReadOnly) {
    if (isReadOnly())
      return MS_NotModifiable;
  }

  return MS_CanModify;
}



// static
KLFLib::EntryMatchCondition KLFLib::EntryMatchCondition::mkMatchAll()
{
  return EntryMatchCondition(MatchAllType);
}
// static
KLFLib::EntryMatchCondition KLFLib::EntryMatchCondition::mkPropertyMatch(PropertyMatch pmatch)
{
  EntryMatchCondition c(PropertyMatchType);
  c.mPropertyMatch = pmatch;
  return c;
}
// static
KLFLib::EntryMatchCondition KLFLib::EntryMatchCondition::mkNegateMatch(const EntryMatchCondition& condition)
{
  EntryMatchCondition c(NegateMatchType);
  c.mConditionList = QList<EntryMatchCondition>() << condition;
  return c;
}
// static
KLFLib::EntryMatchCondition KLFLib::EntryMatchCondition::mkOrMatch(QList<EntryMatchCondition> conditions)
{
  EntryMatchCondition c(OrMatchType);
  c.mConditionList = conditions;
  return c;
}
// static
KLFLib::EntryMatchCondition KLFLib::EntryMatchCondition::mkAndMatch(QList<EntryMatchCondition> conditions)
{
  EntryMatchCondition c(AndMatchType);
  c.mConditionList = conditions;
  return c;
}






// -----

// DATA STREAM OPERATORS
KLF_EXPORT QDataStream& operator<<(QDataStream& stream, const KLFLibResourceEngine::KLFLibEntryWithId& entrywid)
{
  return stream << entrywid.id << entrywid.entry;
}
KLF_EXPORT QDataStream& operator>>(QDataStream& stream, KLFLibResourceEngine::KLFLibEntryWithId& entrywid)
{
  return stream >> entrywid.id >> entrywid.entry;
}

// DEBUG OPERATOR<<'S
KLF_EXPORT  QDebug& operator<<(QDebug& dbg, const KLFLib::StringMatch& smatch)
{
  return dbg << "StringMatch[ref="<<smatch.matchValueString()<<";flags="<<smatch.matchFlags()<<"]";
}
KLF_EXPORT  QDebug& operator<<(QDebug& dbg, const KLFLib::PropertyMatch& pmatch)
{
  //  KLF_DEBUG_BLOCK("operator<<(QDebug, KLFLib::PropertyMatch)") ;
  //  klfDbg("prop-id="<<pmatch.propertyId());
  return dbg << "PropertyMatch[prop-id="<<pmatch.propertyId()<<"; ref="<<pmatch.matchValueString()
	     <<"; flags="<<pmatch.matchFlags()<<"]";
}
KLF_EXPORT  QDebug& operator<<(QDebug& dbg, const KLFLib::EntryMatchCondition& c)
{
  //  KLF_DEBUG_BLOCK("operator<<(QDebug, KLFLib::EntryMatchCondition)") ;
  klfDbg("type="<<c.type()) ;
#ifdef Q_WS_MAC
  return dbg<<"EntryMatchCondition{...}";
#endif
  dbg << "EntryMatchCondition{type=";
  if (c.type() == KLFLib::EntryMatchCondition::MatchAllType) {
    return dbg << "match-all}";
  }
  if (c.type() == KLFLib::EntryMatchCondition::PropertyMatchType) {
    //    KLF_DEBUG_BLOCK("if-block...") ;
    dbg << "property-match; "<<c.propertyMatch();
    //    klfDbg("printed into dbg...") ;
    dbg <<"}";
    //    klfDbg("printed into dbg...") ;
    return dbg;
  }
  if (c.type() != KLFLib::EntryMatchCondition::NegateMatchType &&
      c.type() != KLFLib::EntryMatchCondition::AndMatchType &&
      c.type() != KLFLib::EntryMatchCondition::OrMatchType) {
    return dbg << "unknown-type}";
  }
  // NOT, AND or OR type:
  static const char *w_and = " AND ";
  static const char *w_or  = " OR ";
  static const char *w_not = " NOT ";
  const char * word =  (c.type()==KLFLib::EntryMatchCondition::NegateMatchType)? w_not :
			((c.type()==KLFLib::EntryMatchCondition::AndMatchType) ? w_and : w_or) ;
  dbg << (word+1/*nospace*/) << "; list: ";
  QList<KLFLib::EntryMatchCondition> conditions = c.conditionList();
  int k;
  for (k = 0; k < conditions.size(); ++k) {
    if (k > 0)
      dbg << word;
    dbg << conditions[k];
  }
  dbg << ".}";
  return dbg;
}
KLF_EXPORT  QDebug& operator<<(QDebug& dbg, const KLFLibResourceEngine::KLFLibEntryWithId& e)
{
  return dbg <<"KLFLibEntryWithId(id="<<e.id<<";"<<e.entry.category()<<","<<e.entry.tags()<<","
	     <<e.entry.latex()<<")";
}
KLF_EXPORT  QDebug& operator<<(QDebug& dbg, const KLFLibResourceEngine::Query& q)
{
  //  KLF_DEBUG_BLOCK("operator<<(QDebug, KLFLibRes.Eng.::Query)") ;
  return dbg << "Query(cond.="<<q.matchCondition<<"; skip="<<q.skip<<",limit="<<q.limit
	     <<"; orderpropid="<<q.orderPropId<<"/"<<(q.orderDirection==Qt::AscendingOrder ? "Asc":"Desc")
	     <<"; wanted props="<<q.wantedEntryProperties<<")" ;
}




// ---------------------------------------------------

QList<KLFLib::entryId> KLFLibResourceSimpleEngine::allIds(const QString& subResource)
{
  QList<KLFLib::entryId> idList;
  QList<KLFLibResourceEngine::KLFLibEntryWithId> elist = allEntries(subResource);
  int k;
  for (k = 0; k < idList.size(); ++k)
    idList << elist[k].id;
  return idList;
}

bool KLFLibResourceSimpleEngine::hasEntry(const QString& subResource, entryId id)
{
  /** \bug ............... BUG/TODO .......... concept problem here */
  return entry(subResource, id).latex().size();
}

QList<KLFLibResourceEngine::KLFLibEntryWithId>
/* */ KLFLibResourceSimpleEngine::entries(const QString& subResource, const QList<KLFLib::entryId>& idList,
					  const QList<int>& /*wantedEntryProperties*/)
{
  QList<KLFLibEntryWithId> elist;
  int k;
  for (k = 0; k < idList.size(); ++k)
    elist << KLFLibEntryWithId(idList[k], entry(subResource, idList[k]));
  return elist;
}


int KLFLibResourceSimpleEngine::query(const QString& subResource,
				      const Query& query,
				      QueryResult *result)
{
  return queryImpl(this, subResource, query, result);
}

QList<QVariant> KLFLibResourceSimpleEngine::queryValues(const QString& subResource, int entryPropId)
{
  return queryValuesImpl(this, subResource, entryPropId);
}


template<class T>
static void qlist_skip_and_limit(QList<T> *list, int skip, int limit)
{
  KLF_ASSERT_NOT_NULL( list, "list is NULL!", return; ) ;

  // skip `skip' first entries

  if (skip <= 0) {
    // nothing to do
  } else if (list->size() <= skip) {
    list->clear();
  } else {
    *list = list->mid(skip);
  }

  // and limit to `limit'
  if (limit < 0)
    return; // no limit

  if (list->size() > limit)
    *list = list->mid(0, limit);

  return;
}

// static
int KLFLibResourceSimpleEngine::queryImpl(KLFLibResourceEngine *resource, const QString& subResource,
					  const Query& query, QueryResult *result)
{
  /// \bug ............ some features UNTESTED (match conditions....) ......................

  KLF_DEBUG_TIME_BLOCK(KLF_FUNC_NAME) ;
  klfDbgSt("Query: "<<query);

  if (result == NULL) {
    qWarning()<<KLF_FUNC_NAME<<": expected valid `result' pointer";
    return -1;
  }

  QList<KLFLibEntryWithId> allEList = resource->allEntries(subResource);

  KLFLibEntrySorter sorter(query.orderPropId, query.orderDirection);
  QueryResultListSorter lsorter(&sorter, result);

  // we first need to order _all_ the entries (yes, since the order of allEntries()
  // is undefined ... and limit/skip refer to _ordered_ entry list...)
  int k;
  for (k = 0; k < allEList.size(); ++k) {
    // test match condition
    const KLFLibEntryWithId& ewid = allEList[k];
    if (testEntryMatchConditionImpl(query.matchCondition, ewid.entry)) {
      lsorter.insertIntoOrderedResult(ewid);
    }
  }

  klfDbgSt("queried ordered list. result->entryWithIdList: \n"<<result->entryWithIdList) ;

  // now we need to remove the first 'query.skip' number of results.
  // we can't do this while inserting because the order counts.

  qlist_skip_and_limit(&result->entryIdList, query.skip, query.limit);
  qlist_skip_and_limit(&result->rawEntryList, query.skip, query.limit);
  qlist_skip_and_limit(&result->entryWithIdList, query.skip, query.limit);

  klfDbgSt("About to return. Number of entries in TEE VALUE.") ;

  return KLF_DEBUG_TEE( lsorter.numberOfEntries() );
}

// static
QList<QVariant> KLFLibResourceSimpleEngine::queryValuesImpl(KLFLibResourceEngine *resource,
							    const QString& subResource, int entryPropId)
{
  QList<KLFLibEntryWithId> allEList = resource->allEntries(subResource);
  QList<QVariant> values;
  int k;
  for (k = 0; k < allEList.size(); ++k) {
    QVariant p = allEList[k].entry.property(entryPropId);
    if (!values.contains(p))
      values << p;
  }
  return values;
}


// static
bool KLFLibResourceSimpleEngine::testEntryMatchConditionImpl(const KLFLib::EntryMatchCondition& condition,
							     const KLFLibEntry& libentry)
{
  int k;
  KLFLib::PropertyMatch pmatch;
  QList<KLFLib::EntryMatchCondition> condlist;

  switch (condition.type()) {
  case KLFLib::EntryMatchCondition::MatchAllType:
    return true;
  case KLFLib::EntryMatchCondition::PropertyMatchType:
    pmatch = condition.propertyMatch();
    return klfMatch(libentry.property(pmatch.propertyId()), // test value
		    pmatch.matchValue(), // match value
		    pmatch.matchFlags(), // flags
		    pmatch.matchValueString()); // variant converted to string, cached
  case KLFLib::EntryMatchCondition::NegateMatchType:
    condlist = condition.conditionList(); // only first item is used
    if (condlist.isEmpty()) {
      qWarning()<<KLF_FUNC_NAME<<": NOT condition with no arguments!";
      return false;
    }
    return  ! testEntryMatchConditionImpl(condlist[0], libentry); // negate test
  case KLFLib::EntryMatchCondition::OrMatchType:
    condlist = condition.conditionList();
    if (condlist.isEmpty())
      return true;
    for (k = 0; k < condlist.size(); ++k) {
      if (testEntryMatchConditionImpl(condlist[k], libentry)) // recurse
	return true; // 'OR' -> find one that's OK and the condition is OK
    }
    return false; // but if none is OK then we're not OK
  case KLFLib::EntryMatchCondition::AndMatchType:
    condlist = condition.conditionList();
    if (condlist.isEmpty())
      return true;
    for (k = 0; k < condlist.size(); ++k) {
      if ( ! testEntryMatchConditionImpl(condlist[k], libentry) ) // recurse
	return false; // 'AND' -> find one that's not OK and the condition is globally false
    }
    return true; // but if all are OK then we're OK
  default:
    qWarning()<<KLF_FUNC_NAME<<": KLFLib::EntryMatchCondition type "<<condition.type()<<" not known!";
  }
  return false;
}


KLFLibResourceSimpleEngine::QueryResultListSorter::QueryResultListSorter(KLFLibEntrySorter *sorter,
									 QueryResult *result)
  : mSorter(sorter), mResult(result)
{
  KLF_ASSERT_NOT_NULL( result, "result ptr is NULL!", return ) ;

  fillflags = result->fillFlags;

  if (fillflags & QueryResult::FillRawEntryList) {
    reference_is_rawentrylist = true;
  } else if (fillflags & QueryResult::FillEntryWithIdList) {
    reference_is_rawentrylist = false;
  } else {
    // fill also the raw entry list to have a reference (!)
    fillflags |= QueryResult::FillRawEntryList;
    reference_is_rawentrylist = true;
  }
}
int KLFLibResourceSimpleEngine::QueryResultListSorter::numberOfEntries()
{
  if (reference_is_rawentrylist)
    return mResult->rawEntryList.size();
  else
    return mResult->entryWithIdList.size();
}


/*
KLFLibResourceSimpleEngine::QueryResultListSorter::QueryResultListSorter(const QueryResultListSorter& other)
  : mSorter(other.mSorter), mResult(other.mResult), fillflags(other.fillflags),
    reference_is_rawentrylist(other.reference_is_rawentrylist)
{
}
*/

#define klf_lower_bound_entry						\
  qLowerBound<KLFLibEntryList::iterator,KLFLibEntry,const KLFLibEntrySorter&>
#define klf_lower_bound_ewid						\
  qLowerBound<QList<KLFLibEntryWithId>::iterator,KLFLibEntryWithId,const QueryResultListSorter&>

void KLFLibResourceSimpleEngine::QueryResultListSorter::insertIntoOrderedResult(const KLFLibEntryWithId& ewid)
{
  int pos;

  if (mSorter->propId() == -1) {
    // just append
    if (reference_is_rawentrylist)
      pos = mResult->rawEntryList.size();
    else
      pos = mResult->entryWithIdList.size();
  } else {
    // insert at right place
    if (reference_is_rawentrylist) {
      KLFLibEntryList::iterator it =
	klf_lower_bound_entry(mResult->rawEntryList.begin(), mResult->rawEntryList.end(), ewid.entry, *mSorter);
      pos = it - mResult->rawEntryList.begin();
    } else {
      QList<KLFLibEntryWithId>::iterator it =
	klf_lower_bound_ewid(mResult->entryWithIdList.begin(), mResult->entryWithIdList.end(), ewid, *this);
      pos = it - mResult->entryWithIdList.begin();
    }
  }
  // actually insert the items into appropriate lists
  if (fillflags & QueryResult::FillEntryIdList)
    mResult->entryIdList.insert(pos, ewid.id);
  if (fillflags & QueryResult::FillRawEntryList)
    mResult->rawEntryList.insert(pos, ewid.entry);
  if (fillflags & QueryResult::FillEntryWithIdList)
    mResult->entryWithIdList.insert(pos, ewid);
}


// ---------------------------------------------------

// static
KLFFactoryManager KLFLibEngineFactory::pFactoryManager;


KLFLibEngineFactory::KLFLibEngineFactory(QObject *parent)
  : QObject(parent), KLFFactoryBase(&pFactoryManager)
{
}
KLFLibEngineFactory::~KLFLibEngineFactory()
{
}

uint KLFLibEngineFactory::schemeFunctions(const QString& /*scheme*/) const
{
  return FuncOpen;
}

KLFLibEngineFactory *KLFLibEngineFactory::findFactoryFor(const QUrl& url)
{
  return findFactoryFor(url.scheme());
}

KLFLibEngineFactory *KLFLibEngineFactory::findFactoryFor(const QString& urlscheme)
{
  return dynamic_cast<KLFLibEngineFactory*>(pFactoryManager.findFactoryFor(urlscheme));
}

QStringList KLFLibEngineFactory::allSupportedSchemes()
{
  return pFactoryManager.allSupportedTypes();
}

KLFLibResourceEngine *KLFLibEngineFactory::createResource(const QString& /*scheme*/,
							  const Parameters& /*param*/,
							  QObject */*parent*/)
{
  return NULL;
}

bool KLFLibEngineFactory::saveResourceTo(KLFLibResourceEngine */*resource*/, const QUrl& /*newLocation*/)
{
  return false;
}


KLFLibResourceEngine *KLFLibEngineFactory::openURL(const QUrl& url, QObject *parent)
{
  KLFLibEngineFactory *factory = findFactoryFor(url.scheme());
  if ( factory == NULL ) {
    qWarning()<<"KLFLibEngineFactory::openURL("<<url<<"): No suitable factory found!";
    return NULL;
  }
  return factory->openResource(url, parent);
}

// static
QMap<QString,QString> KLFLibEngineFactory::listSubResourcesWithTitles(const QUrl& urlbase)
{
  QUrl url = urlbase;
  url.addQueryItem("klfReadOnly", "true");
  KLFLibResourceEngine *resource = openURL(url, NULL); // NULL parent
  if ( resource == NULL ) {
    qWarning()<<"KLFLibEngineFactory::listSubResources("<<url<<"): Unable to open resource!";
    return QMap<QString,QString>();
  }
  if ( !(resource->supportedFeatureFlags() & KLFLibResourceEngine::FeatureSubResources) ) {
    qWarning()<<"KLFLibEngineFactory::listSubResources("<<url<<"): Resource does not support sub-resources!";
    return QMap<QString,QString>();
  }
  QStringList subreslist = resource->subResourceList();
  int k;
  QMap<QString,QString> subresmap;
  for (k = 0; k < subreslist.size(); ++k) {
    if (resource->supportedFeatureFlags() & KLFLibResourceEngine::FeatureSubResourceProps)
      subresmap[subreslist[k]]
	= resource->subResourceProperty(subreslist[k],
					KLFLibResourceEngine::SubResPropTitle).toString();
    else
      subresmap[subreslist[k]] = QString();
  }
  delete resource;
  return subresmap;
}

// static
QStringList KLFLibEngineFactory::listSubResources(const QUrl& urlbase)
{
  return listSubResourcesWithTitles(urlbase).keys();
}

