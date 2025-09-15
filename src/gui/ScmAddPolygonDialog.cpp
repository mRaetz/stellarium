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

#include "ScmAddPolygonDialog.hpp"
#include "ui_scmAddPolygonDialog.h"
#include <cassert>
#include <QDebug>

ScmAddPolygonDialog::ScmAddPolygonDialog(QObject *parent)
	: StelDialogSeparate("ScmAddPolygonDialog")
{
	//assert(maker != nullptr);
	ui = new Ui_scmAddPolygonDialog;
}

ScmAddPolygonDialog::~ScmAddPolygonDialog()
{
	if (ui != nullptr)
	{
		delete ui;
	}

	qDebug() << "SkyCultureMaker: Unloaded the ScmHideOrAbortMakerDialog";
}

void ScmAddPolygonDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
	}
}

void ScmAddPolygonDialog::createDialogContent()
{
	ui->setupUi(dialog);

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->titleBar, &TitleBar::closeClicked, this, &ScmAddPolygonDialog::cancelAddPolygonDialog); // Close = Cancel
	connect(ui->titleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint))); // moveWindow

	// Buttons
	connect(ui->dialogButtonBox, &QDialogButtonBox::accepted, this, &ScmAddPolygonDialog::confirmAddPolygonDialog); // Confirm
	connect(ui->dialogButtonBox, &QDialogButtonBox::rejected, this, &ScmAddPolygonDialog::cancelAddPolygonDialog); // Cancel

	// SpinBox
	connect(ui->startTimeSpinBox, &QSpinBox::editingFinished, this, &ScmAddPolygonDialog::changeStartTime);
	connect(ui->endTimeSpinBox, &QSpinBox::editingFinished, this, &ScmAddPolygonDialog::changeEndTime);
}

void ScmAddPolygonDialog::confirmAddPolygonDialog()
{
	int startTime = ui->startTimeSpinBox->value();
	int endTime = ui->endTimeSpinBox->value();
	emit(addPolygonDialogConfirmed(startTime, endTime));

	setVisible(false);
}

void ScmAddPolygonDialog::cancelAddPolygonDialog()
{
	//setVisible(false);
	close();
}

void ScmAddPolygonDialog::changeStartTime()
{
	int startYear = ui->startTimeSpinBox->value();
	int endYear = ui->endTimeSpinBox->value();

	if (startYear > endYear)
	{
		ui->startTimeSpinBox->blockSignals(true);
		ui->startTimeSpinBox->setValue(endYear);
		ui->startTimeSpinBox->blockSignals(false);
	}
}

void ScmAddPolygonDialog::changeEndTime()
{
	int startYear = ui->startTimeSpinBox->value();
	int endYear = ui->endTimeSpinBox->value();

	if (endYear < startYear)
	{
		ui->endTimeSpinBox->blockSignals(true);
		ui->endTimeSpinBox->setValue(startYear);
		ui->endTimeSpinBox->blockSignals(false);
	}
}

void ScmAddPolygonDialog::setStartTime(int Year)
{
	ui->startTimeSpinBox->blockSignals(true);
	ui->endTimeSpinBox->blockSignals(true);

	ui->startTimeSpinBox->setValue(Year);
	ui->endTimeSpinBox->setValue(Year);

	ui->startTimeSpinBox->blockSignals(false);
	ui->endTimeSpinBox->blockSignals(false);
}

void ScmAddPolygonDialog::setTimeLimits(int minYear, int maxYear)
{
	ui->startTimeSpinBox->setMinimum(minYear);
	ui->startTimeSpinBox->setMaximum(maxYear);

	ui->endTimeSpinBox->setMinimum(minYear);
	ui->endTimeSpinBox->setMaximum(maxYear);
}

