/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2020 Equinor ASA
//
//  ResInsight is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  ResInsight is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.
//
//  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
//  for more details.
//
/////////////////////////////////////////////////////////////////////////////////

#include "RiuFileDialogTools.h"

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RiuFileDialogTools::getSaveFileName( QWidget*       parent /*= nullptr*/,
                                             const QString& caption /*= QString()*/,
                                             const QString& dir /*= QString()*/,
                                             const QString& filter /*= QString()*/,
                                             QString*       selectedFilter /*= nullptr */ )
{
    QFileDialog::Options options = defaultOptions();

    return QFileDialog::getSaveFileName( parent, caption, dir, filter, selectedFilter, options );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QStringList RiuFileDialogTools::getOpenFileNames( QWidget*       parent /*= nullptr*/,
                                                  const QString& caption /*= QString()*/,
                                                  const QString& dir /*= QString()*/,
                                                  const QString& filter /*= QString()*/,
                                                  QString*       selectedFilter /*= nullptr */ )
{
    QFileDialog::Options options = defaultOptions();

    return QFileDialog::getOpenFileNames( parent, caption, dir, filter, selectedFilter, options );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RiuFileDialogTools::getExistingDirectory( QWidget*       parent /*= nullptr*/,
                                                  const QString& caption /*= QString()*/,
                                                  const QString& dir /*= QString() */ )
{
    QFileDialog::Options options = defaultOptions();

#ifndef WIN32
    options |= QFileDialog::ShowDirsOnly;
    options |= QFileDialog::DontResolveSymlinks;
#endif

    return QFileDialog::getExistingDirectory( parent, caption, dir, options );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RiuFileDialogTools::getOpenFileName( QWidget*       parent /*= nullptr*/,
                                             const QString& caption /*= QString()*/,
                                             const QString& dir /*= QString()*/,
                                             const QString& filter /*= QString()*/,
                                             QString*       selectedFilter /*= nullptr */ )
{
    QFileDialog::Options options = defaultOptions();

    return QFileDialog::getOpenFileName( parent, caption, dir, filter, selectedFilter, options );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QFileDialog::Options RiuFileDialogTools::defaultOptions()
{
    // Large performance improvements have been measured related to DontUseCustomDirectoryIcons
    // https://github.com/yayapoi/qtbase/commit/46685f755b01288fd53c4483cb97a22c426a57f0
    //
    // Never use native dialog on Linux
    // https://github.com/OPM/ResInsight/issues/6345

#ifdef WIN32
    return QFileDialog::DontUseCustomDirectoryIcons;
#endif
    return QFileDialog::DontUseCustomDirectoryIcons | QFileDialog::DontUseNativeDialog;
}
