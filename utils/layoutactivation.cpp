#include "layoutactivation.h"
#include <QLayout>
#include <QTimer>
#include <QApplication>
#include <QDebug>

void LayoutActivationController::activateLayouts(QWidget* widget, ActivationTiming timing)
{
    if (!widget) {
        qWarning() << "LayoutActivationController: null widget pointer";
        return;
    }
    
    switch (timing) {
    case ActivationTiming::Immediate:
        performLayoutActivation(widget);
        break;
        
    case ActivationTiming::Deferred:
    case ActivationTiming::OnNextEventLoop:
        scheduleLayoutActivation(widget, 0);
        break;
    }
}

void LayoutActivationController::scheduleLayoutActivation(QWidget* widget, int delayMs)
{
    if (!widget) {
        qWarning() << "LayoutActivationController: null widget pointer in scheduleLayoutActivation";
        return;
    }
    
    QTimer::singleShot(delayMs, widget, [widget]() {
        performLayoutActivation(widget);
    });
}

bool LayoutActivationController::isLayoutActivationComplete(QWidget* widget)
{
    if (!widget) {
        return false;
    }
    
    QLayout* layout = widget->layout();
    if (!layout) {
        return true; // No layout means "complete" by default
    }
    
    // Check if layout has been activated and geometry is valid
    return layout->geometry().isValid() && !layout->geometry().isEmpty();
}

bool LayoutActivationController::validateLayoutGeometry(QWidget* widget)
{
    if (!widget) {
        return false;
    }
    
    // Check widget geometry
    if (!widget->geometry().isValid() || widget->geometry().isEmpty()) {
        qDebug() << "LayoutActivationController: Invalid widget geometry" << widget->geometry();
        return false;
    }
    
    // Check layout geometry if present
    QLayout* layout = widget->layout();
    if (layout) {
        if (!layout->geometry().isValid() || layout->geometry().isEmpty()) {
            qDebug() << "LayoutActivationController: Invalid layout geometry" << layout->geometry();
            return false;
        }
        
        // Recursively check child widgets
        for (int i = 0; i < layout->count(); ++i) {
            QLayoutItem* item = layout->itemAt(i);
            if (item && item->widget()) {
                if (!validateLayoutGeometry(item->widget())) {
                    return false;
                }
            }
        }
    }
    
    return true;
}

void LayoutActivationController::performLayoutActivation(QWidget* widget)
{
    if (!widget) {
        qWarning() << "LayoutActivationController: null widget pointer in performLayoutActivation";
        return;
    }
    
    qDebug() << "LayoutActivationController: Activating layouts for widget" << widget->objectName();
    
    // Activate the main layout
    QLayout* layout = widget->layout();
    if (layout) {
        layout->activate();
        layout->update();
        
        qDebug() << "LayoutActivationController: Main layout activated, geometry:" << layout->geometry();
        
        // Recursively activate child layouts
        for (int i = 0; i < layout->count(); ++i) {
            QLayoutItem* item = layout->itemAt(i);
            if (item && item->widget()) {
                QWidget* childWidget = item->widget();
                if (childWidget->layout()) {
                    performLayoutActivation(childWidget);
                }
            }
        }
    }
    
    // Force widget to update its geometry
    widget->updateGeometry();
    widget->update();
    
    // Validate the final layout
    if (!validateLayoutGeometry(widget)) {
        qWarning() << "LayoutActivationController: Layout validation failed for widget" << widget->objectName();
    } else {
        qDebug() << "LayoutActivationController: Layout activation completed successfully";
    }
}