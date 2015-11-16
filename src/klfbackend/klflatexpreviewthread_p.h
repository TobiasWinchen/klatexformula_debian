/***************************************************************************
 *   file klflatexpreviewthread_p.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2012 by Philippe Faist
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
/* $Id: klflatexpreviewthread_p.h 903 2014-08-10 02:15:11Z phfaist $ */

/** \file
 * This header contains (in principle private) auxiliary classes for
 * library routines defined in klflatexpreviewthread.cpp */

#ifndef KLFLATEXPREVIEWTHREAD_P_H
#define KLFLATEXPREVIEWTHREAD_P_H

#include <QObject>
#include <QThread>
#include <QQueue>
#include <QAtomicInt>

#include "klflatexpreviewthread.h"



class KLFLatexPreviewThreadWorker : public QObject
{
  Q_OBJECT

public:
  KLFLatexPreviewThreadWorker()
    : QObject(NULL)
  {
    _abort = 0;
    newTasks = QQueue<Task>();
  };

  typedef KLFLatexPreviewThread::TaskId TaskId;

  struct Task {
    Task() { }

    KLFBackend::klfInput input;
    KLFBackend::klfSettings settings;
    QSize previewSize;
    QSize largePreviewSize;

    KLFLatexPreviewHandler * handler;

    TaskId taskid;
  };


signals:
  void threadStartedProcessingJob(TaskId taskid);
  void threadFinishedJob(TaskId taskid);

public slots:
  void threadSubmitTask(KLFLatexPreviewThreadWorker::Task task, bool clearOtherJobs = false,
			KLFLatexPreviewThread::TaskId replaceJobId = -1);
  void threadProcessJobs();
  bool threadCancelTask(KLFLatexPreviewThread::TaskId task);
  void threadClearPendingTasks();

  // this slot may be called by direct connection, it is thread-safe.
  inline void abort() { _abort.fetchAndStoreOrdered(1); }

private:
  // the thread will stop if it notices this has become 1
  QAtomicInt _abort;

  QQueue<Task> newTasks;
};



class KLFLatexPreviewThreadPrivate : public QObject
{
  Q_OBJECT
public:
  KLF_PRIVATE_QOBJ_HEAD(KLFLatexPreviewThread, QObject)
  {
    previewSize = QSize(280, 80);
    largePreviewSize = QSize(640, 480);

    taskIdCounter = 1;
  }

  QSize previewSize;
  QSize largePreviewSize;


  KLFLatexPreviewThread::TaskId taskIdCounter;

  KLFLatexPreviewThread::TaskId submitTask(KLFLatexPreviewThreadWorker::Task t, bool clear,
					   KLFLatexPreviewThread::TaskId replaceId)
  {
    //    if (replaceId >= 0) // if we're replacing a job, use the same ID
    //      t.taskid = replaceId;
    //    else
    t.taskid = taskIdCounter++;

    emit internalRequestSubmitNewTask(t, clear, replaceId);

    klfDbg("new task submitted, id="<<t.taskid) ;
    return t.taskid;
  }


signals:
  /** \internal
   *
   * This signal is meant to be received by the inner worker, but others can access it too. It
   * is emitted _before_ the abort process has completed. */
  void internalRequestAbort();

  /** \internal
   *
   * This signal is meant to be only received by the inner worker. */
  void internalRequestSubmitNewTask(const KLFLatexPreviewThreadWorker::Task& task, bool clearOtherJobs,
				    KLFLatexPreviewThread::TaskId replaceTaskId);

  /** \internal
   *
   * This signal is meant to be only received by the inner worker. */
  void internalRequestClearPendingTasks();

  /** \internal
   *
   * This signal is meant to be only received by the inner worker. */
  void internalRequestCancelTask(KLFLatexPreviewThread::TaskId id);


  friend class KLFLatexPreviewThread;

};

/* not needed
QDataStream& operator<<(QDataStream& str, KLFLatexPreviewThreadPrivate::Task& task)
{
  return str << task.input << task.settings << task.previewSize << task.largePreviewSize
	     << (int)task.handler << task.isfresh << task.isrunning << task.taskid;
}
QDataStream& operator>>(QDataStream& str, KLFLatexPreviewThreadPrivate::Task& task)
{
  int ptrhandler;
  str >> task.input >> task.settings >> task.previewSize >> task.largePreviewSize
      >> ptrhandler >> task.isfresh >> task.isrunning >> task.taskid;
  task.handler = (void*)ptrhandler;
  return str;
}
*/




// -----------------------------------





class KLFContLatexPreviewPrivate : public KLFLatexPreviewHandler
{
  Q_OBJECT
public:
  KLF_PRIVATE_QOBJ_HEAD(KLFContLatexPreview, KLFLatexPreviewHandler)
  {
    thread = NULL;

    curTask = -1;

    input = KLFBackend::klfInput();
    settings = KLFBackend::klfSettings();
    previewSize = QSize(280, 80);
    largePreviewSize = QSize(640, 480);
  }
  virtual ~KLFContLatexPreviewPrivate()
  {
  }

  KLFLatexPreviewThread * thread;

  KLFLatexPreviewThread::TaskId curTask;

  KLFBackend::klfInput input;
  KLFBackend::klfSettings settings;
  QSize previewSize;
  QSize largePreviewSize;

  void refreshPreview()
  {
    KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;

    KLF_ASSERT_NOT_NULL(thread, "Thread is NULL! Can't refresh preview!", return; ) ;

    curTask = thread->replaceSubmitPreviewTask(curTask, input, settings, this,
					       previewSize, largePreviewSize);
    if (curTask == -1) {
      klfWarning("Failed to submit preview task to thread.") ;
    } else {
      emit K->compiling(true);
    }
  }

public slots:

  void latexPreviewReset()
  {
    emit K->compiling(false);
    emit K->previewReset();
  }

  void latexOutputAvailable(const KLFBackend::klfOutput& output)
  {
    emit K->compiling(false);
    emit K->outputAvailable(output);
  }
  void latexPreviewAvailable(const QImage& preview, const QImage& largePreview, const QImage& fullPreview)
  {
    // compiling(false) emitted in latexOutputAvailable().
    emit K->previewAvailable(preview, largePreview, fullPreview);
  }
  void latexPreviewImageAvailable(const QImage& preview)
  {
    // compiling(false) emitted in latexOutputAvailable().
    emit K->previewImageAvailable(preview);
  }
  void latexPreviewLargeImageAvailable(const QImage& largePreview)
  {
    // compiling(false) emitted in latexOutputAvailable().
    emit K->previewLargeImageAvailable(largePreview);
  }
  void latexPreviewFullImageAvailable(const QImage& fullPreview)
  {
    // compiling(false) emitted in latexOutputAvailable().
    emit K->previewFullImageAvailable(fullPreview);
  }

  void latexPreviewError(const QString& errorString, int errorCode)
  {
    emit K->compiling(false);
    emit K->previewError(errorString, errorCode);
  }
};






#endif


