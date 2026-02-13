/*
 * Sky Culture Maker plug-in for Stellarium
 *
 * Copyright (C) 2025 Moritz Rätz
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

#ifndef SCMEDITMODEPATHITEM_HPP
#define SCMEDITMODEPATHITEM_HPP

#include <QGraphicsPathItem>

//! @class ScmEditModePathItem
//! Simple QGraphicsEllipseItem defined by a ellipse, startTime and EndTime ???.
class ScmEditModePathItem : public QGraphicsPathItem
{
public:
	ScmEditModePathItem(int edgeID);

	// public functions
	int getStartTime() const {return startTime;}
	int getEndTime() const {return endTime;}
	int getEdgeID() const {return edgeID;}
	bool existsAtPointInTime(int year) const;
	void setPosition(const QPointF &pos);
	//void setSize();

public slots:

signals:

protected:
	void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
	void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
	QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value) override;

private:
	int startTime;
	int endTime;
	int edgeID;
};

#endif // SCMEDITMODEPATHITEM_HPP
