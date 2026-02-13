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

#include "ScmEditModeEllipseItem.hpp"
#include <qgraphicsscene.h>
#include <qpen.h>
#include <qstyleoption.h>

ScmEditModeEllipseItem::ScmEditModeEllipseItem(int vertexID)
	: QGraphicsEllipseItem()
	, vertexID(vertexID)
{
	setPen(QPen(QColor(255, 0, 0), 1));
	setAcceptHoverEvents(true);
}

void ScmEditModeEllipseItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
	setPen(QPen(QColor(0, 0, 255), 2));
	//setBrush(QBrush());
	QGraphicsEllipseItem::hoverEnterEvent(event);
}

void ScmEditModeEllipseItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
	setPen(QPen(QColor(255, 0, 0), 1));
	QGraphicsEllipseItem::hoverEnterEvent(event);
}

void ScmEditModeEllipseItem::setPosition(const QPointF &pos)
{
	setRect(pos.x() - 5, pos.y() - 5, 10, 10);
}

bool ScmEditModeEllipseItem::existsAtPointInTime(int year) const
{
	return true;
}

QVariant ScmEditModeEllipseItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
	// prevent de-selection when hiding the item

	if (change == QGraphicsItem::ItemVisibleChange)
	{
		setPen(QPen(QColor(255, 0, 0), 1));
		return value;
	}

	return QGraphicsEllipseItem::itemChange(change, value);
}
