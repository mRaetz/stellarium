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

#include "ScmGeoLocGraphicsView.hpp"
#include "ScmEditModeEllipseItem.hpp"
#include "ScmEditModePathItem.hpp"
#include <qevent.h>
#include <qguiapplication.h>
#include <qjsonarray.h>
#include <QGraphicsSvgItem>
#include <qscrollbar.h>
#include <QFileDialog>

#include <QJsonObject>
#include <QJsonDocument>
#include <cmath> // for M_PI

ScmGeoLocGraphicsView::ScmGeoLocGraphicsView(QWidget *parent)
	: QGraphicsView(parent)
	, viewScrolling(false)
	, firstShow(true)
	, editMode(EditMode::INACTIVE)
	, currentYear(0)
	, mouseLastXY(0, 0)
{
	QGraphicsScene *scene = new QGraphicsScene(this);
	setScene(scene);
	setInteractive(true);

	setRenderHint(QPainter::Antialiasing); // maybe unnecessary for this project
	setTransformationAnchor(AnchorUnderMouse);

	setCursor(Qt::CrossCursor);
	setMouseTracking(true);

	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	QGraphicsSvgItem *baseMap = new QGraphicsSvgItem(":/graphicGui/skyCultureWorldMap.svgz");
	scene->addItem(baseMap);
	scene->setSceneRect(- baseMap->boundingRect().width() * 0.75, - baseMap->boundingRect().height() * 0.5, baseMap->boundingRect().width() * 2.5, baseMap->boundingRect().height() * 2);
	this->defaultRect = baseMap->boundingRect();

	scene->addItem(currentCapturePolygon);
	scene->addItem(previewCapturePath);
}

void ScmGeoLocGraphicsView::wheelEvent(QWheelEvent *event)
{
	qreal zoomFactor = std::pow(2.0, event->angleDelta().y() / 240.0);
	qreal ctrZoomFactor = 0.0;
	if ( event->modifiers() & Qt::ControlModifier )
	{
		//holding ctrl while wheel zooming results in a finer zoom
		ctrZoomFactor = 1.0 + ( zoomFactor - 1.0 ) / 15.0;
		scaleView(ctrZoomFactor);
		return;
	}
	scaleView(zoomFactor); // faster scrolling = faster zoom
}

void ScmGeoLocGraphicsView::showEvent(QShowEvent *event)
{
	// fit the base map to the current view when the widget is first shown
	// (This cannot be done beforehand because the calculation is based on the current size of the viewPort.
	// The viewPort is smaller than it should be until the first show because the widget is located on the stackedWidget in ViewDialog.
	// The boolean firstShow is used so the map doesn't reset itself every time the user changes pages or closes the ViewDialog.)
	if (firstShow)
	{
		qreal ratio = calculateScaleRatio(defaultRect.width(), defaultRect.height());
		scale(ratio, ratio);
		centerOn(defaultRect.center());
		firstShow = false;
	}

	QGraphicsView::showEvent(event);
}

void ScmGeoLocGraphicsView::mouseMoveEvent(QMouseEvent *event)
{
	if (!currentCapturePolygon->polygon().empty() || editMode == EditMode::MOVEPOINT || editMode == EditMode::ADDPOINT)
	{
		previewCapturePath->setMousePoint(mapToScene(event->pos()));
	}

	// reimplementation of default ScrollHandDrag in QGraphicsView
	if (viewScrolling) {
		QScrollBar *hBar = horizontalScrollBar();
		QScrollBar *vBar = verticalScrollBar();
		QPoint delta = event->pos() - mouseLastXY;
		hBar->setValue(hBar->value() + (isRightToLeft() ? delta.x() : -delta.x()));
		vBar->setValue(vBar->value() - delta.y());
	}
	mouseLastXY = event->pos();

	QGraphicsView::mouseMoveEvent(event);
}

void ScmGeoLocGraphicsView::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		if(event->modifiers() & Qt::ShiftModifier)
		{
			viewScrolling = true;
			QGuiApplication::setOverrideCursor(Qt::ClosedHandCursor);
		}
	}
	else if (event->button() == Qt::MiddleButton)
	{
		viewScrolling = true;
		QGuiApplication::setOverrideCursor(Qt::ClosedHandCursor);
	}
	// if event is not accepted (mouse not over item) mouseReleaseEvent is not triggered
	event->setAccepted(true);
}

void ScmGeoLocGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
	setFocus();
	if (!(editMode == EditMode::INACTIVE))
	{
		if ((event->button() == Qt::RightButton || event->button() == Qt::LeftButton) && !viewScrolling)
		{
			if (editMode == EditMode::ACTIVE)
			{
				// check if item under mouse is edit ellipse
				QGraphicsItem *currentTopmostMouseGrabberItem = itemAt(event->pos());
				currentVertexEllipseItem = qgraphicsitem_cast<ScmEditModeEllipseItem *>(currentTopmostMouseGrabberItem);
				if (currentVertexEllipseItem)
				{
					QPolygonF currentParentPolygon = qgraphicsitem_cast<ScmPreviewPolygonItem *>(currentVertexEllipseItem->parentItem())->polygon();
					int currentVertexIndex = std::find(editModeVertexList.begin(), editModeVertexList.end(), currentVertexEllipseItem->getVertexID()) - editModeVertexList.begin();

					previewCapturePath->setFirstPoint(currentParentPolygon[(currentVertexIndex - 1) < 0 ? (currentParentPolygon.size() - 1) : (currentVertexIndex - 1)]);
					previewCapturePath->setLastPoint(currentParentPolygon[(currentVertexIndex + 1) == currentParentPolygon.size() ? 0 : (currentVertexIndex + 1)]);
					previewCapturePath->setMousePoint(mapToScene(event->pos()));
					previewCapturePath->setFillPath(false);

					currentVertexEllipseItem->setVisible(false);
					QGuiApplication::setOverrideCursor(Qt::CrossCursor);
					editMode = EditMode::MOVEPOINT;
				}
				currentEdgePathItem = qgraphicsitem_cast<ScmEditModePathItem *>(currentTopmostMouseGrabberItem);
				if (currentEdgePathItem)
				{
					QPolygonF currentParentPolygon = qgraphicsitem_cast<ScmPreviewPolygonItem *>(currentEdgePathItem->parentItem())->polygon();
					int currentEdgeIndex = std::find_if(editModeEdgeList.begin(), editModeEdgeList.end(),
														[this](const ScmEditModePathItem* i) {
															return i->getEdgeID() == currentEdgePathItem->getEdgeID();
														}) - editModeEdgeList.begin();

					previewCapturePath->setFirstPoint(currentParentPolygon[currentEdgeIndex]);
					previewCapturePath->setLastPoint(currentParentPolygon[(currentEdgeIndex + 1) == currentParentPolygon.size() ? 0 : (currentEdgeIndex + 1)]);
					previewCapturePath->setMousePoint(mapToScene(event->pos()));
					previewCapturePath->setFillPath(false);

					currentEdgePathItem->setVisible(false);
					QGuiApplication::setOverrideCursor(Qt::CrossCursor);
					editMode = EditMode::ADDPOINT;
				}
			}
			else if (editMode == EditMode::MOVEPOINT)
			{
				auto *currentParentItem = qgraphicsitem_cast<ScmPreviewPolygonItem *>(currentVertexEllipseItem->parentItem());
				QPolygonF newPolygon = currentParentItem->polygon();

				// set point of polygon at index (vertexID) to new value
				int currentVertexIndex = std::find(editModeVertexList.begin(), editModeVertexList.end(), currentVertexEllipseItem->getVertexID()) - editModeVertexList.begin();
				newPolygon[currentVertexIndex] = mapToScene(event->pos());
				currentParentItem->setPolygon(newPolygon);

				// set pos and vis of ellipse item
				currentVertexEllipseItem->setVisible(true);
				currentVertexEllipseItem->setPosition(mapToScene(event->pos()));

				// reposition adjacent edgeItems
				// predecessor edge
				int evalLowerBoundIdx = (currentVertexIndex - 1) < 0 ? newPolygon.size() - 1 : (currentVertexIndex - 1);
				qreal centerX = (newPolygon[evalLowerBoundIdx].x() + newPolygon[currentVertexIndex].x()) / 2;
				qreal centerY = (newPolygon[evalLowerBoundIdx].y() + newPolygon[currentVertexIndex].y()) / 2;
				editModeEdgeList[evalLowerBoundIdx]->setPosition(QPointF(centerX, centerY));
				// successor edge
				int evalUpperBoundIdx = (currentVertexIndex + 1) == newPolygon.size() ? 0 : (currentVertexIndex + 1);
				centerX = (newPolygon[currentVertexIndex].x() + newPolygon[evalUpperBoundIdx].x()) / 2;
				centerY = (newPolygon[currentVertexIndex].y() + newPolygon[evalUpperBoundIdx].y()) / 2;
				editModeEdgeList[currentVertexIndex]->setPosition(QPointF(centerX, centerY));

				// cleanup / reset
				previewCapturePath->reset();
				QGuiApplication::restoreOverrideCursor();
				currentVertexEllipseItem = nullptr;
				editMode = EditMode::ACTIVE;
			}
			else if (editMode == EditMode::ADDPOINT)
			{
				auto *currentParentItem = qgraphicsitem_cast<ScmPreviewPolygonItem *>(currentEdgePathItem->parentItem());
				QPolygonF newPolygon = currentParentItem->polygon();

				// add new point to polygon
				int currentEdgeIndex = std::find_if(editModeEdgeList.begin(), editModeEdgeList.end(),
													[this](const ScmEditModePathItem* i) {
														return i->getEdgeID() == currentEdgePathItem->getEdgeID();
													}) - editModeEdgeList.begin();
				newPolygon.insert(currentEdgeIndex + 1, mapToScene(event->pos()));
				currentParentItem->setPolygon(newPolygon);

				// add new vertexItem
				//neues Vertex Item, eine neue Kante und alte Kante verschieben,
				const auto vertexItem = new ScmEditModeEllipseItem(*std::max_element(editModeVertexList.begin(), editModeVertexList.end()) + 1);
				editModeVertexList.insert(currentEdgeIndex + 1, vertexItem->getVertexID());
				vertexItem->setPosition(newPolygon[currentEdgeIndex + 1]);
				vertexItem->setParentItem(currentParentItem);
				// reposition old edgeItem (predecessor)
				int evalUpperBoundIdx = (currentEdgeIndex + 1) == newPolygon.size() ? 0 : (currentEdgeIndex + 1);
				qreal centerX = (newPolygon[currentEdgeIndex].x() + newPolygon[evalUpperBoundIdx].x()) / 2;
				qreal centerY = (newPolygon[currentEdgeIndex].y() + newPolygon[evalUpperBoundIdx].y()) / 2;
				currentEdgePathItem->setPosition(QPointF(centerX, centerY));
				// add new edgeItem (successor)
				const auto edgeItem = new ScmEditModePathItem((*std::max_element(editModeEdgeList.begin(), editModeEdgeList.end(), [this]
																				( ScmEditModePathItem* a,  ScmEditModePathItem* b)
																				{ return a->getEdgeID() < b->getEdgeID(); }))->getEdgeID() + 1);
				editModeEdgeList.insert(currentEdgeIndex + 1, edgeItem);
				evalUpperBoundIdx = (currentEdgeIndex + 2) == newPolygon.size() ? 0 : (currentEdgeIndex + 2);
				centerX = (newPolygon[currentEdgeIndex + 1].x() + newPolygon[evalUpperBoundIdx].x()) / 2;
				centerY = (newPolygon[currentEdgeIndex + 1].y() + newPolygon[evalUpperBoundIdx].y()) / 2;
				edgeItem->setPosition(QPointF(centerX, centerY));
				edgeItem->setParentItem(currentParentItem);

				// cleanup / reset
				previewCapturePath->reset();
				QGuiApplication::restoreOverrideCursor();
				currentEdgePathItem->setVisible(true);
				currentEdgePathItem = nullptr;
				editMode = EditMode::ACTIVE;
			}
		}
	}
	else
	{
		if (event->button() == Qt::LeftButton)
		{
			// open dialog (range of time) + save and reset the current capture polygon
			if(event->modifiers() & Qt::AltModifier)
			{
				if (currentCapturePolygon->polygon().size() < 3)
				{
					return;
				}
				// open dialog 'popup'
				emit showAddPolyDialog();
			}
			// do not set a point after a scrolling operation (maybe the user unintentionally released SHIFT)
			else if (!viewScrolling)
			{
				if (currentCapturePolygon->polygon().size() < 1)
				{
					previewCapturePath->setFirstPoint(mapToScene(event->pos()));
				}
				else
				{
					previewCapturePath->setLastPoint(mapToScene(event->pos()));
				}

				currentCapturePolygon->setPolygon(currentCapturePolygon->polygon() << mapToScene(event->pos()));
			}
		}
		else if (event->button() == Qt::RightButton)
		{
			// open dialog (range of time) + save and reset the current capture polygon
			if(event->modifiers() & Qt::AltModifier)
			{
				if (currentCapturePolygon->polygon().size() < 3)
				{
					return;
				}
				// open dialog 'popup'
				emit showAddPolyDialog();
			}
			// do not set a point after a scrolling operation (maybe the user unintentionally released SHIFT)
			else
			{
				if (currentCapturePolygon->polygon().size() < 1)
				{
					previewCapturePath->setFirstPoint(mapToScene(event->pos()));
				}
				else
				{
					previewCapturePath->setLastPoint(mapToScene(event->pos()));
				}

				currentCapturePolygon->setPolygon(currentCapturePolygon->polygon() << mapToScene(event->pos()));
			}
		}
	}

	if(viewScrolling)
	{
		viewScrolling = false;
		QGuiApplication::restoreOverrideCursor();
	}
}

void ScmGeoLocGraphicsView::keyPressEvent(QKeyEvent *event)
{
	if (!(editMode == EditMode::INACTIVE))
	{
		if (editMode == EditMode::ACTIVE)
		{
			if (event->key() == Qt::Key_C && event->modifiers() & Qt::ControlModifier)
			{
				temporaryPolygonCopy = polygonIdentifierMap.value(editModeBackupPolygon.first)->polygon();
			}
		}
		else if (editMode == EditMode::MOVEPOINT)
		{
			if (event->key() == Qt::Key_Backspace)
			{
				auto *currentParentItem = qgraphicsitem_cast<ScmPreviewPolygonItem *>(currentVertexEllipseItem->parentItem());
				if (currentParentItem->polygon().size() > 3)
				{
					// delete point in graphicsItem
					QPolygonF newPolygon = currentParentItem->polygon();
					int currentVertexIndex = std::find(editModeVertexList.begin(), editModeVertexList.end(), currentVertexEllipseItem->getVertexID()) - editModeVertexList.begin();
					newPolygon.erase(std::begin(newPolygon) + currentVertexIndex);
					currentParentItem->setPolygon(newPolygon);
					editModeVertexList.erase(std::begin(editModeVertexList) + currentVertexIndex);
					// update currentVertexIndex (only necessary if deleted point was last point in poly)

					// delete current vertexItem
					delete currentVertexEllipseItem;
					currentVertexEllipseItem = nullptr;

					// delete successor
					auto successor = editModeEdgeList[currentVertexIndex];
					editModeEdgeList.erase(std::begin(editModeEdgeList) + currentVertexIndex);
					delete successor;
					// reposition predecessor
					currentVertexIndex = currentVertexIndex == newPolygon.size() ? 0 : currentVertexIndex;;
					int evalLowerBoundIdx = (currentVertexIndex - 1) < 0 ? newPolygon.size() - 1 : (currentVertexIndex - 1);
					qreal centerX = (newPolygon[evalLowerBoundIdx].x() + newPolygon[currentVertexIndex].x()) / 2;
					qreal centerY = (newPolygon[evalLowerBoundIdx].y() + newPolygon[currentVertexIndex].y()) / 2;
					editModeEdgeList[evalLowerBoundIdx]->setPosition(QPointF(centerX, centerY));

					QGuiApplication::restoreOverrideCursor();
					previewCapturePath->reset();
					editMode = EditMode::ACTIVE;
				}

			}
			else if (event->key() == Qt::Key_Escape)
			{
				QGuiApplication::restoreOverrideCursor();
				currentVertexEllipseItem->setVisible(true);
				currentVertexEllipseItem = nullptr;
				previewCapturePath->reset();
				editMode = EditMode::ACTIVE;
			}
		}
		else if (editMode == EditMode::ADDPOINT)
		{
			if (event->key() == Qt::Key_Escape)
			{
				QGuiApplication::restoreOverrideCursor();
				currentEdgePathItem->setVisible(true);
				currentEdgePathItem = nullptr;
				previewCapturePath->reset();
				editMode = EditMode::ACTIVE;
			}
		}
	}
	else
	{
		// delete the last point in the capture poylgon (except the very first one)
		if (event->key() == Qt::Key_Backspace)
		{
			if (currentCapturePolygon->polygon().size() > 1)
			{
				QPolygonF newPoly = currentCapturePolygon->polygon();
				newPoly.removeLast();

				previewCapturePath->setLastPoint(newPoly.last());
				currentCapturePolygon->setPolygon(newPoly);
			}
		}
		// abort current capture ---> reset all points (including first one)
		else if (event->key() == Qt::Key_Escape)
		{
			currentCapturePolygon->setPolygon(QPolygonF());
			previewCapturePath->reset();
		}
		if (event->key() == Qt::Key_V && event->modifiers() & Qt::ControlModifier)
		{
			if (!temporaryPolygonCopy.empty())
			{
				currentCapturePolygon->setPolygon(temporaryPolygonCopy);
				previewCapturePath->setFirstPoint(temporaryPolygonCopy.first());
				previewCapturePath->setLastPoint(temporaryPolygonCopy.last());
			}
		}
	}
}

void ScmGeoLocGraphicsView::scaleView(double scaleFactor)
{
	// calculate requested zoom before executing the zoom operation to limit the min / max zoom level
	const double scaling = transform().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();

	if (scaling < 0.1 || scaling > 2300.0) // scaling < min or scaling > max zoom level
	{
		return;
	}

	scale(scaleFactor, scaleFactor);
}

void ScmGeoLocGraphicsView::updateTime(int year)
{
	currentYear = year;
	updateCultureVisibility();
}

void ScmGeoLocGraphicsView::selectPolygon(int id)
{
	ScmPreviewPolygonItem *poly = polygonIdentifierMap.value(id);

	// if no fitting polygon was found --> return to prevent errors
	if(poly == nullptr)
	{
		return;
	}

	if(!poly->existsAtPointInTime(currentYear))
	{
		// signal connects to updateSkyCultureTimeValue in ScmSkyCultureDialog which invokes updateTime (in this class)
		emit timeValueChanged(poly->getStartTime());
	}

	const QRectF polyBbox = poly->boundingRect();
	fitInView(QRectF(polyBbox.x() - polyBbox.width() / 8, polyBbox.y() - polyBbox.height() / 8, polyBbox.width() * 1.25, polyBbox.height() * 1.25), Qt::KeepAspectRatio);
}

void ScmGeoLocGraphicsView::updateCultureVisibility()
{
	const auto itemList = scene()->items();
	for(const auto &item : itemList) {
		ScmPreviewPolygonItem *previewPItem = qgraphicsitem_cast<ScmPreviewPolygonItem *>(item);

		// if cast was unsuccessful (item is not an SkyCulturePolygonItem) --> look at the next item
		if(!previewPItem)
			continue;

		// if the current year is between the start and end time of the polygon --> show (otherwise hide the item)
		if(previewPItem->existsAtPointInTime(currentYear))
		{
			previewPItem->setVisible(true);
		}
		else
		{
			previewPItem->setVisible(false);
		}
	}
}

void ScmGeoLocGraphicsView::reset()
{
	if (!(editMode == EditMode::INACTIVE))
	{
		exitEditMode(true);
	}
	// remove all polygons from the scene
	for (auto it = polygonIdentifierMap.cbegin(); it != polygonIdentifierMap.cend(); ++it)
	{
		delete it.value();
	}

	// clear the map used for identifying the polygon items
	polygonIdentifierMap.clear();

	// set firstShow to true, so that the default extent is shown
	firstShow = true;

	// clear the digitization previews
	currentCapturePolygon->setPolygon(QPolygonF());
	previewCapturePath->reset();
}

qreal ScmGeoLocGraphicsView::calculateScaleRatio(qreal width, qreal height)
{
	// Rect of the current view with a margin of 2
	QRectF viewRect = viewport()->rect().adjusted(2, 2, - 2, - 2);
	if (viewRect.isEmpty())
	{
		return 0;
	}

	// Rect of the current transformation in scene coordinates
	// values of x / y of sceneRect are not important since only the width / height are used for the calculation
	QRectF sceneRect = transform().mapRect(QRectF(2, 2, width, height));
	if (sceneRect.isEmpty())
	{
		return 0;
	}

	// calculate the x / y ratio for scaling
	qreal xratio = viewRect.width() / sceneRect.width();
	qreal yratio = viewRect.height() / sceneRect.height();

	// keep original aspect ratio
	return std::min(xratio, yratio);
}

void ScmGeoLocGraphicsView::addCurrentPoly(int startTime, int endTime)
{
	// add the polygon to the scene so users can see the progress while digitizing other polygons
	ScmPreviewPolygonItem *poly = new ScmPreviewPolygonItem(startTime, endTime, currentCapturePolygon->polygon());
	scene()->addItem(poly);

	// save poly
	if (polygonIdentifierMap.empty())
	{
		polygonIdentifierMap.insert(0, poly);
	}
	else
	{
		polygonIdentifierMap.insert(polygonIdentifierMap.lastKey() + 1, poly);
	}

	// convert the view coordinates to real world coordinates
	QPolygonF transformedPolygon = convertViewToWGS84(currentCapturePolygon->polygon());

	emit addPolygonToCulture(scm::CulturePolygon(polygonIdentifierMap.lastKey(), startTime, QString::number(endTime), transformedPolygon));

	// reset capture poly and path
	currentCapturePolygon->setPolygon(QPolygonF());
	previewCapturePath->reset();

	// update the map
	updateCultureVisibility();
}

void ScmGeoLocGraphicsView::removePolygon(int id)
{
	// remove item from scene and delete the corresponding entry in polygonIdentifierMap
	scene()->removeItem(polygonIdentifierMap.value(id));
	polygonIdentifierMap.remove(id);
}

void ScmGeoLocGraphicsView::editPolygon(int id)
{
	// new item for every point of polygon
	editModeBackupPolygon = QPair<int, QPolygonF>(id, polygonIdentifierMap.value(id)->polygon());
	editModeVertexList = QList<int>();
	editModeEdgeList = QList<ScmEditModePathItem *>();
	for (int vertexIdx = 0; vertexIdx < editModeBackupPolygon.second.size(); vertexIdx++)
	{
		// add vertexItem
		const auto vertexItem = new ScmEditModeEllipseItem(vertexIdx);
		editModeVertexList.append(vertexItem->getVertexID());
		vertexItem->setPosition(editModeBackupPolygon.second[vertexIdx]);
		vertexItem->setParentItem(polygonIdentifierMap.value(id));
		// add edgeItem
		const auto edgeItem = new ScmEditModePathItem(vertexIdx);
		editModeEdgeList.append(edgeItem);
		// calc position from neighbouring vertices
		int evalUpperBoundIdx = (vertexIdx + 1) == editModeBackupPolygon.second.size() ? 0 : (vertexIdx + 1);
		qreal centerX = (editModeBackupPolygon.second[vertexIdx].x() + editModeBackupPolygon.second[evalUpperBoundIdx].x()) / 2;
		qreal centerY = (editModeBackupPolygon.second[vertexIdx].y() + editModeBackupPolygon.second[evalUpperBoundIdx].y()) / 2;
		edgeItem->setPosition(QPointF(centerX, centerY));
		edgeItem->setParentItem(polygonIdentifierMap.value(id));
	}

	editMode = EditMode::ACTIVE;
	setCursor(Qt::ArrowCursor);
}

void ScmGeoLocGraphicsView::exitEditMode(bool discardProgress)
{
	auto *backupPolygonItem = polygonIdentifierMap.value(editModeBackupPolygon.first);
	if (discardProgress)
	{
		// restore old polygon geometry
		backupPolygonItem->setPolygon(editModeBackupPolygon.second);
	}
	else
	{
		// convert the view coordinates to real world coordinates
		QPolygonF transformedPolygon = convertViewToWGS84(backupPolygonItem->polygon());
		emit addPolygonToCulture(scm::CulturePolygon(editModeBackupPolygon.first, backupPolygonItem->getStartTime(), QString::number(backupPolygonItem->getEndTime()), transformedPolygon));
	}

	// remove vertexItems
	for (const auto &item : backupPolygonItem->childItems())
	{
		delete item;
	}
	previewCapturePath->reset();
	QGuiApplication::restoreOverrideCursor();
	editMode = EditMode::INACTIVE;
	setCursor(Qt::CrossCursor);
}

QPolygonF ScmGeoLocGraphicsView::convertViewToWGS84(const QPolygonF &viewCoordinatePolygon)
{
	QPolygonF result;

	// convert view coordinates to native coordinate system of the current map (EPSG: 3857)
	for (const auto &point : viewCoordinatePolygon)
	{
		qreal xInMeter = ((point.x() / defaultRect.width()) * 40075014.1343236863613128) - 20037507.0671618431806564;
		qreal yInMeter = ((point.y() / defaultRect.height()) * -37274855.60442495346069336) + 18418386.3090785145759583;
		result.append(QPointF(xInMeter, yInMeter));
	}

	// convert map coordinates to Lat/Lon coordinates (EPSG: 4326)
	for (auto &point : result)
	{
		qreal xInLon= (point.x() * 180.0) / 20037508.3427892439067363739014;
		qreal yInLat = (std::atan(std::exp(((point.y() * 180.0) / 20037508.3427892439067363739014) * (M_PI / 180.0))) * (360.0 / M_PI)) - 90.0;

		point.setX(xInLon);
		point.setY(yInLat);
	}

	return result;
}


