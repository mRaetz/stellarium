#ifndef SKYCULTUREMAPGRAPHICSVIEW_HPP
#define SKYCULTUREMAPGRAPHICSVIEW_HPP

#include "ScmAddPolygonDialog.hpp"
#include "PreviewPathItem.hpp"
#include "SkyculturePolygonItem.hpp"
#include "PreviewPolygonItem.hpp"
#include <qtimeline.h>
#include <QGraphicsView>

//! @class SkyCultureMapGraphicsView
//! Special GraphicsView that shows a world map and several (culture) polygons
class SkycultureMapGraphicsView : public QGraphicsView
{
	Q_OBJECT

public:
	SkycultureMapGraphicsView(QWidget *parent = nullptr);

	// public functions
	//void initializeTime();
	void initializeGraphicsView();

public slots:
	void selectCulture(const QString &skycultureId);
	void updateTime(int year);
	void rotateMap(bool isRotated);
	void changeProjection(bool isChanged);
	// ===================================
	void addCurrentPoly(int startTime, int endTime);


signals:
	void cultureSelected(const QString &skycultureId);
	void timeValueChanged(int year);
	void timeRangeChanged(int minYear, int maxYear);
	// =============================================
	void addPolyDialogShown();


protected:
	void wheelEvent(QWheelEvent *event) override;
	// void mouseMoveEvent(QMouseEvent *event) override;
	// void mousePressEvent(QMouseEvent *event) override;
	// void mouseReleaseEvent(QMouseEvent *event) override;
	void showEvent(QShowEvent *event) override;
	void scaleView(double scaleFactor);
	// ===============================
	void mouseMoveEvent( QMouseEvent *e ) override;
	void mousePressEvent( QMouseEvent *e ) override;
	void mouseReleaseEvent( QMouseEvent *e ) override;
	void keyPressEvent( QKeyEvent *e ) override;


private: // projection change bestenfalls mit ENUM oder so
	// variables
	bool viewScrolling;
	bool firstShow;
	int currentYear;
	QPoint mouseLastXY;
	QString oldSkyCulture;
	QTimeLine zoomToDefaultTimer;
	QTimeLine zoomOnTargetTimer;
	QRectF startingRect;
	QRectF defaultRect;
	QRectF targetRect;

	// functions
	QList<QPointF> convertLatLonToMeter(const QList<QPointF> &irl, qreal mapWidth, qreal mapHeight);
	QList<QPointF> convertMeterToView(const QList<QPointF> &irl, qreal mapWidth, qreal mapHeight);
	void updateCultureVisibility();
	void smoothFitInView(QRectF targetRect);
	void selectAllCulturePolygon(const QString &skycultureId);
	void drawMapContent(const QString &baseMap);
	qreal calculateScaleRatio(qreal width, qreal height);
	// ================================

	struct CulturePolygon
	{
		QPolygonF polygon;
		int startTime = 0;
		int endTime = 0;

		CulturePolygon() = default;
		CulturePolygon(QPolygonF polygon, int startTime, int endTime)
			: polygon(polygon), startTime(startTime), endTime(endTime)
		{
		}
	};

	//QList<...Polygon(F) mit start und endzeit> *
	QList<CulturePolygon> digitizedPolygons;
	PreviewPolygonItem *currentCapturePolygon = new PreviewPolygonItem(true);

	//QPointF captureFirstPoint;
	//QPointF captureLastPoint;
	PreviewPathItem *previewCapturePath = new PreviewPathItem();

	// functions
	void updatePreviewPath();
	void exportCulturePolygons();



private slots:
	void zoomToDefault(qreal factor);
	void zoomOnTarget(qreal factor);
};

#endif // SKYCULTUREMAPGRAPHICSVIEW_HPP
