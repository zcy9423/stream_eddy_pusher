#include "geometryvalidator.h"
#include "displaycontext.h"
#include <QApplication>
#include <QScreen>
#include <QDebug>
#include <QStyleHints>

GeometryValidator::ValidationResult GeometryValidator::validateGeometry(
    const QRect& requestedGeometry, const QSize& minimumSize, int targetDisplay)
{
    ValidationResult result;
    result.adjustedGeometry = requestedGeometry;
    
    // Get target display info
    DisplayContextManager::DisplayInfo displayInfo;
    if (targetDisplay >= 0) {
        displayInfo = DisplayContextManager::getDisplayInfo(targetDisplay);
    } else {
        displayInfo = DisplayContextManager::getPrimaryDisplayInfo();
    }
    
    if (!displayInfo.screen) {
        result.wasAdjusted = true;
        result.adjustmentReason = "No valid display found, using fallback geometry";
        result.adjustedGeometry = QRect(100, 100, qMax(minimumSize.width(), 1200), qMax(minimumSize.height(), 800));
        return result;
    }
    
    // 1. Ensure minimum size constraints
    QSize adjustedSize = requestedGeometry.size();
    if (adjustedSize.width() < minimumSize.width()) {
        adjustedSize.setWidth(minimumSize.width());
        result.wasAdjusted = true;
        result.adjustmentReason += "Width adjusted to minimum size; ";
    }
    if (adjustedSize.height() < minimumSize.height()) {
        adjustedSize.setHeight(minimumSize.height());
        result.wasAdjusted = true;
        result.adjustmentReason += "Height adjusted to minimum size; ";
    }
    
    // 2. Account for window frame margins
    QMargins frameMargins = estimateFrameMargins();
    QRect geometryWithFrame = QRect(requestedGeometry.topLeft(), adjustedSize);
    geometryWithFrame = adjustForFrameMargins(geometryWithFrame, frameMargins);
    
    // 3. Adjust for display bounds
    QRect finalGeometry = adjustForDisplayBounds(geometryWithFrame, displayInfo.screen);
    
    if (finalGeometry != requestedGeometry) {
        result.wasAdjusted = true;
        if (result.adjustmentReason.isEmpty()) {
            result.adjustmentReason = "Adjusted to fit display bounds";
        } else {
            result.adjustmentReason += "Adjusted to fit display bounds";
        }
    }
    
    result.adjustedGeometry = finalGeometry;
    
    qDebug() << "GeometryValidator: Requested" << requestedGeometry 
             << "-> Final" << result.adjustedGeometry
             << "Adjusted:" << result.wasAdjusted;
    
    return result;
}

QRect GeometryValidator::adjustForDisplayBounds(const QRect& geometry, const QScreen* screen)
{
    if (!screen) {
        qWarning() << "GeometryValidator: null screen pointer";
        return geometry;
    }
    
    QRect availableGeometry = screen->availableGeometry();
    QRect adjustedGeometry = geometry;
    
    // Ensure window fits within display width
    if (adjustedGeometry.width() > availableGeometry.width()) {
        adjustedGeometry.setWidth(availableGeometry.width());
    }
    
    // Ensure window fits within display height
    if (adjustedGeometry.height() > availableGeometry.height()) {
        adjustedGeometry.setHeight(availableGeometry.height());
    }
    
    // Adjust position to keep window within bounds
    if (adjustedGeometry.right() > availableGeometry.right()) {
        adjustedGeometry.moveRight(availableGeometry.right());
    }
    if (adjustedGeometry.bottom() > availableGeometry.bottom()) {
        adjustedGeometry.moveBottom(availableGeometry.bottom());
    }
    if (adjustedGeometry.left() < availableGeometry.left()) {
        adjustedGeometry.moveLeft(availableGeometry.left());
    }
    if (adjustedGeometry.top() < availableGeometry.top()) {
        adjustedGeometry.moveTop(availableGeometry.top());
    }
    
    return adjustedGeometry;
}

QRect GeometryValidator::adjustForFrameMargins(const QRect& geometry, const QMargins& frameMargins)
{
    // Subtract frame margins from the geometry to account for window decorations
    QRect adjustedGeometry = geometry;
    adjustedGeometry.adjust(-frameMargins.left(), -frameMargins.top(), 
                           frameMargins.right(), frameMargins.bottom());
    return adjustedGeometry;
}

bool GeometryValidator::isGeometryValid(const QRect& geometry, const QScreen* screen)
{
    if (!screen || geometry.isEmpty()) {
        return false;
    }
    
    QRect availableGeometry = screen->availableGeometry();
    
    // Check if window is completely outside the screen
    if (!geometry.intersects(availableGeometry)) {
        return false;
    }
    
    // Check if window is reasonably positioned (at least 50% visible)
    QRect intersection = geometry.intersected(availableGeometry);
    int visibleArea = intersection.width() * intersection.height();
    int totalArea = geometry.width() * geometry.height();
    
    return (visibleArea >= totalArea / 2);
}

QMargins GeometryValidator::estimateFrameMargins()
{
    // Platform-specific frame margin estimation
    // These are conservative estimates for window decorations
    
#ifdef Q_OS_WIN
    // Windows typically has larger frames
    return QMargins(8, 31, 8, 8); // left, top, right, bottom
#elif defined(Q_OS_MAC)
    // macOS has a title bar but minimal side frames
    return QMargins(0, 22, 0, 0);
#else
    // Linux/X11 - varies by window manager, use conservative estimate
    return QMargins(4, 24, 4, 4);
#endif
}