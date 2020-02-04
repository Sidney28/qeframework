/*  QEArchiveStatus.cpp
 *
 *  This file is part of the EPICS QT Framework, initially developed at the
 *  Australian Synchrotron.
 *
 *  Copyright (c) 2013-2020 Australian Synchrotron
 *
 *  The EPICS QT Framework is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  The EPICS QT Framework is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with the EPICS QT Framework.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Author:
 *    Andrew Starritt
 *  Contact details:
 *    andrew.starritt@synchrotron.org.au
 */

#include "QEArchiveStatus.h"
#include <QDebug>
#include <QLabel>
#include <QFrame>
#include <QECommon.h>

#define DEBUG  qDebug () << "QEArchiveStatus" << __LINE__ << __FUNCTION__ << "  "


//==============================================================================
//
void QEArchiveStatus::setStatusRowVisible (const int j, const bool visible)
{
   if (j < 0 || j >= ARRAY_LENGTH (this->rowList)) return;   // sanity check

   QEArchiveStatus::Rows* row = &this->rowList [j];
   row->hostNamePort->setVisible (visible);
   row->endPoint->setVisible (visible);
   row->state->setVisible (visible);
   row->numberPVs->setVisible (visible);
   if (archiveAccess->getArchiverType() == QEArchiveAccess::CA) {
      row->available->setVisible (visible);
      row->read->setVisible (visible);
      row->pending->setVisible (visible);
   }
}

//------------------------------------------------------------------------------
//
void QEArchiveStatus::createInternalWidgets ()
{
   static const int frameHeight = 15;
   static const int horMargin = 4;    // 19 - 2 - 2 => widget height is 15
   static const int horSpacing = 4;

   // j, col, row and sheet are CREATE_LABEL globals
   //
   int j;           // rowList row
   int row;         // grid layout row
   int col;         // grid layout column
   QString sheet;   // style sheet

#define CREATE_LABEL(member, align, text)  {                             \
   this->rowList [j].member = new QLabel (text, NULL);                   \
   this->rowList [j].member->setIndent (6);                              \
   this->rowList [j].member->setMinimumWidth (68);                       \
   this->rowList [j].member->setFixedHeight (frameHeight);               \
   this->rowList [j].member->setAlignment (align);                       \
   this->rowList [j].member->setStyleSheet (sheet);                      \
   this->gridLayout->addWidget (this->rowList [j].member, row, col++);   \
}


   this->archiveAccess = new QEArchiveAccess (this);
   this->archiveAccess->setMessageSourceId (9001);

   const QEArchiveAccess::ArchiverTypes archType = QEArchiveAccess::getArchiverType();

   this->gridLayout = new QGridLayout (this);
   this->gridLayout->setContentsMargins (horMargin, 6, horMargin, 2);  // left, top, right, bottom
   this->gridLayout->setVerticalSpacing (1);
   this->gridLayout->setHorizontalSpacing (horSpacing);

   // Use use the last row as a header row.
   // We don't need to keep a reference to header labels, but
   // this does allow us to use CREATE_LABEL.
   j = NumberRows;
   sheet = "";

   row = 0;
   col = 0;
   CREATE_LABEL (hostNamePort,  Qt::AlignLeft,    "Host:Port");
   CREATE_LABEL (endPoint,      Qt::AlignLeft,    "End Point");
   CREATE_LABEL (state,         Qt::AlignHCenter, "Status");
   if (archType == QEArchiveAccess::CA) {
      CREATE_LABEL (available,     Qt::AlignRight,   "Available");
      CREATE_LABEL (read,          Qt::AlignRight,   "Read");
   } else {
      this->rowList [j].available = NULL;
      this->rowList [j].read = NULL;
   }
   CREATE_LABEL (numberPVs,     Qt::AlignRight,   "Num PVs");
   if (archType == QEArchiveAccess::CA) {
      CREATE_LABEL (pending,       Qt::AlignRight,   "Pending");
   } else {
      this->rowList [j].pending = NULL;
   }

   QColor background = QColor (240, 240, 240, 255);
   sheet = QEUtilities::colourToStyle (background);

   for (j = 0; j < NumberRows; j++ ) {
      row = j+1;
      col = 0;
      CREATE_LABEL (hostNamePort, Qt::AlignLeft,     " - ");
      CREATE_LABEL (endPoint,     Qt::AlignLeft,     " - ");
      CREATE_LABEL (state,        Qt::AlignHCenter,  " - ");
      if (archType == QEArchiveAccess::CA) {
         CREATE_LABEL (available,    Qt::AlignRight,    " - ");
         CREATE_LABEL (read,         Qt::AlignRight,    " - ");
      } else {
         this->rowList [j].available = NULL;
         this->rowList [j].read = NULL;
      }
      CREATE_LABEL (numberPVs,    Qt::AlignRight,    " - ");
      if (archType == QEArchiveAccess::CA) {
         CREATE_LABEL (pending,      Qt::AlignRight,    " - ");
      } else {
         this->rowList [j].pending = NULL;
      }

      // Set only two rows visible until we know better.
      //
      this->setStatusRowVisible (j, j < 2);
   }

   // this->gridLayout->addStretch ();

#undef CREATE_LABEL
}

//---------------------------------------------------------------------------------
//
void QEArchiveStatus::calcMinimumHeight ()
{
   int count;
   int delta_top;

   // Allow +1 for titles.
   //
   count = this->inUseCount + 1;

   delta_top = 20;
   this->setMinimumHeight ((delta_top * count) + 24);
}

//------------------------------------------------------------------------------
//
QEArchiveStatus::QEArchiveStatus (QWidget* parent) : QEGroupBox (parent)
{
   this->createInternalWidgets();

   const QEArchiveAccess::ArchiverTypes archType = QEArchiveAccess::getArchiverType();
   switch (archType) {
      case QEArchiveAccess::CA:
         this->setTitle (" Channel Archiver Host Status ");
         break;
      case QEArchiveAccess::ARCHAPPL:
         this->setTitle (" Archive Appliance Host Status ");
         break;
      case QEArchiveAccess::Error:
         this->setTitle (" Archive Status Summary ");
         break;
   }


   this->inUseCount = 2;
   this->calcMinimumHeight ();
   this->inUseCount = 0;

   this->setMinimumWidth (776);

   // Connect archiveStatus signal to this object slot.
   //
   QObject::connect (this->archiveAccess,
                     SIGNAL     (archiveStatus (const QEArchiveAccess::StatusList&)),
                     this, SLOT (archiveStatus (const QEArchiveAccess::StatusList&)));

   // This info re-emitted on change, but we need to stimulate an initial update.
   //
   this->archiveAccess->resendStatus ();
}

//------------------------------------------------------------------------------
//
QEArchiveStatus::~QEArchiveStatus () { }

//------------------------------------------------------------------------------
//
QSize QEArchiveStatus::sizeHint () const
{
   return QSize (776, 84);   // two rows
}

//------------------------------------------------------------------------------
//
void QEArchiveStatus::reReadAvailablePVs ()
{
   if (this->archiveAccess) this->archiveAccess->reReadAvailablePVs ();
}

//------------------------------------------------------------------------------
//
void QEArchiveStatus::archiveStatus (const QEArchiveAccess::StatusList& statusList)
{
   this->inUseCount = statusList.count ();
   this->calcMinimumHeight ();

   for (int j = 0; j < QEArchiveStatus::NumberRows; j++ ) {
      QEArchiveStatus::Rows* row = &this->rowList [j];

      if (j <  statusList.count ()) {
         QEArchiveAccess::Status state = statusList.value (j);

         // Note the extra space at end - indent only applies as per alignment
         row->hostNamePort->setText (QString ("%1:%2 ").arg (state.hostName).arg (state.portNumber));
         row->endPoint->setText (QString("%1 ").arg (state.endPoint));
         row->state->setText (QEUtilities::enumToString(QEArchapplInterface::staticMetaObject, QString("States"), state.state));
         row->numberPVs->setText (QString ("%1").arg (state.numberPVs));

         if (archiveAccess->getArchiverType() == QEArchiveAccess::CA) {
            row->available->setText (QString ("%1").arg (state.available));
            row->read->setText (QString ("%1").arg (state.read));
            row->pending->setText (QString ("%1").arg (state.pending));
         }
         this->setStatusRowVisible (j, true);
      } else {
         this->setStatusRowVisible (j, false);
      }
   }
}

// end
