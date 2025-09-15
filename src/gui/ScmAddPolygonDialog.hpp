/*
 * Sky Culture Maker plug-in for Stellarium
 *
 * Copyright (C) 2025 Vincent Gerlach
 * Copyright (C) 2025 Luca-Philipp Grumbach
 * Copyright (C) 2025 Fabian Hofer
 * Copyright (C) 2025 Mher Mnatsakanyan
 * Copyright (C) 2025 Richard Hofmann
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SCMADDPOLYGONDIALOG_HPP
#define SCMADDPOLYGONDIALOG_HPP

#include "StelDialogSeparate.hpp"
#include <QObject>

class Ui_scmAddPolygonDialog;

class ScmAddPolygonDialog : public StelDialogSeparate
{
	Q_OBJECT
protected:
	void createDialogContent() override;

public:
	ScmAddPolygonDialog(QObject *parent = nullptr);
	~ScmAddPolygonDialog() override;

	void setStartTime(int Year);
	void setTimeLimits(int minYear, int maxYear);

public slots:
	void retranslate() override;

signals:
	void addPolygonDialogConfirmed(int startTime, int endTime);

private slots:
	void confirmAddPolygonDialog();
	void cancelAddPolygonDialog();
	void changeStartTime();
	void changeEndTime();

private:
	Ui_scmAddPolygonDialog *ui = nullptr;
};

#endif // SCMADDPOLYGONDIALOG_HPP
