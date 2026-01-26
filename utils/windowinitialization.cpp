#include "windowinitialization.h"
#include "geometryvalidator.h"
#include "displaycontext.h"
#include "layoutactivation.h"
#include <QApplication>
#include <QScreen>
#include <QDebug>

void WindowInitializationManager::initializeWindow(QMainWindow* window, const InitializationConfig& config)
{
    if (!window) {
        qWarning() << "WindowInitializationManager: null window pointer";
        return;
    }
    
    qDebug() << "WindowInitializationManager: Initializing window with size" << config.preferredSize;
    
    // 1. Set minimum size first
    window->setMinimumSize(config.minimumSize);
    
    // 2. Calculate optimal geometry
    QRect optimalGeometry = calculateOptimalGeometry(config.preferredSize, config.displayIndex);
    
    // 3. Validate and adjust geometry
    GeometryValidator::ValidationResult validation = GeometryValidator::validateGeometry(
        optimalGeometry, config.minimumSize, config.displayIndex);
    
    if (validation.wasAdjusted) {
        qDebug() << "WindowInitializationManager: Geometry adjusted -" << validation.adjustmentReason;
    }
    
    // 4. Apply geometry before showing window
    applyGeometryWithValidation(window, validation.adjustedGeometry);
    
    qDebug() << "WindowInitializationManager: Window initialized with geometry" << validation.adjustedGeometry;
}

void WindowInitializationManager::prepareGeometry(QMainWindow* window, const QSize& size)
{
    if (!window) {
        qWarning() << "WindowInitializationManager: null window pointer in prepareGeometry";
        return;
    }
    
    // Calculate geometry for current display
    QRect geometry = calculateOptimalGeometry(size, -1);
    
    // Validate geometry
    GeometryValidator::ValidationResult validation = GeometryValidator::validateGeometry(
        geometry, window->minimumSize(), -1);
    
    // Apply validated geometry
    applyGeometryWithValidation(window, validation.adjustedGeometry);
}

bool WindowInitializationManager::validateDisplayBounds(const QRect& geometry, int displayIndex)
{
    DisplayContextManager::DisplayInfo displayInfo;
    
    if (displayIndex >= 0) {
        displayInfo = DisplayContextManager::getDisplayInfo(displayIndex);
    } else {
        displayInfo = DisplayContextManager::getPrimaryDisplayInfo();
    }
    
    if (!displayInfo.screen) {
        qWarning() << "WindowInitializationManager: Invalid display info";
        return false;
    }
    
    return GeometryValidator::isGeometryValid(geometry, displayInfo.screen);
}

QRect WindowInitializationManager::calculateOptimalGeometry(const QSize& size, int displayIndex)
{
    DisplayContextManager::DisplayInfo displayInfo;
    
    if (displayIndex >= 0) {
        displayInfo = DisplayContextManager::getDisplayInfo(displayIndex);
    } else {
        displayInfo = DisplayContextManager::getPrimaryDisplayInfo();
    }
    
    if (!displayInfo.screen) {
        qWarning() << "WindowInitializationManager: No valid display found, using fallback";
        // Fallback to simple centering
        return QRect(100, 100, size.width(), size.height());
    }
    
    QRect availableGeometry = displayInfo.availableGeometry;
    
    // Center the window on the display
    int x = availableGeometry.x() + (availableGeometry.width() - size.width()) / 2;
    int y = availableGeometry.y() + (availableGeometry.height() - size.height()) / 2;
    
    // Ensure window is not positioned outside display bounds
    x = qMax(availableGeometry.x(), x);
    y = qMax(availableGeometry.y(), y);
    
    return QRect(x, y, size.width(), size.height());
}

void WindowInitializationManager::applyGeometryWithValidation(QMainWindow* window, const QRect& geometry)
{
    if (!window) {
        qWarning() << "WindowInitializationManager: null window pointer in applyGeometryWithValidation";
        return;
    }
    
    try {
        // Set geometry before the window is shown to prevent Qt geometry errors
        window->setGeometry(geometry);
        
        qDebug() << "WindowInitializationManager: Applied geometry" << geometry;
        
    } catch (const std::exception& e) {
        qWarning() << "WindowInitializationManager: Exception applying geometry:" << e.what();
        
        // Fallback: try with a safe default geometry
        QRect fallbackGeometry(100, 100, 1200, 800);
        window->setGeometry(fallbackGeometry);
        
    } catch (...) {
        qWarning() << "WindowInitializationManager: Unknown exception applying geometry";
        
        // Fallback: try with a safe default geometry
        QRect fallbackGeometry(100, 100, 1200, 800);
        window->setGeometry(fallbackGeometry);
    }
}