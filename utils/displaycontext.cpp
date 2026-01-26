#include "displaycontext.h"
#include <QApplication>
#include <QScreen>
#include <QDebug>

// Static member definitions
QList<DisplayContextManager::DisplayInfo> DisplayContextManager::s_cachedDisplays;
QDateTime DisplayContextManager::s_cacheTimestamp;

DisplayContextManager::DisplayInfo DisplayContextManager::getPrimaryDisplayInfo()
{
    QScreen* primaryScreen = QApplication::primaryScreen();
    if (!primaryScreen) {
        qWarning() << "DisplayContextManager: No primary screen available";
        return DisplayInfo();
    }
    
    DisplayInfo info;
    info.screen = primaryScreen;
    info.availableGeometry = primaryScreen->availableGeometry();
    info.devicePixelRatio = primaryScreen->devicePixelRatio();
    info.logicalDpi = primaryScreen->logicalDotsPerInch();
    
    qDebug() << "DisplayContextManager: Primary display -" 
             << "Geometry:" << info.availableGeometry
             << "DPI:" << info.logicalDpi 
             << "Ratio:" << info.devicePixelRatio;
    
    return info;
}

DisplayContextManager::DisplayInfo DisplayContextManager::getDisplayInfo(int displayIndex)
{
    QList<QScreen*> screens = QApplication::screens();
    
    if (displayIndex < 0 || displayIndex >= screens.size()) {
        qWarning() << "DisplayContextManager: Invalid display index" << displayIndex 
                   << "Available displays:" << screens.size();
        return getPrimaryDisplayInfo();
    }
    
    QScreen* screen = screens[displayIndex];
    if (!screen) {
        qWarning() << "DisplayContextManager: Null screen at index" << displayIndex;
        return getPrimaryDisplayInfo();
    }
    
    DisplayInfo info;
    info.screen = screen;
    info.availableGeometry = screen->availableGeometry();
    info.devicePixelRatio = screen->devicePixelRatio();
    info.logicalDpi = screen->logicalDotsPerInch();
    
    qDebug() << "DisplayContextManager: Display" << displayIndex 
             << "Geometry:" << info.availableGeometry
             << "DPI:" << info.logicalDpi 
             << "Ratio:" << info.devicePixelRatio;
    
    return info;
}

QList<DisplayContextManager::DisplayInfo> DisplayContextManager::getAllDisplays()
{
    // Check cache validity
    if (isDisplayInfoCacheValid()) {
        qDebug() << "DisplayContextManager: Using cached display information";
        return s_cachedDisplays;
    }
    
    // Refresh cache
    cacheDisplayInformation();
    return s_cachedDisplays;
}

QRect DisplayContextManager::adjustForDPI(const QRect& geometry, qreal sourceDPI, qreal targetDPI)
{
    if (qFuzzyCompare(sourceDPI, targetDPI) || sourceDPI <= 0 || targetDPI <= 0) {
        return geometry;
    }
    
    qreal scaleFactor = targetDPI / sourceDPI;
    
    QRect adjustedGeometry;
    adjustedGeometry.setX(qRound(geometry.x() * scaleFactor));
    adjustedGeometry.setY(qRound(geometry.y() * scaleFactor));
    adjustedGeometry.setWidth(qRound(geometry.width() * scaleFactor));
    adjustedGeometry.setHeight(qRound(geometry.height() * scaleFactor));
    
    qDebug() << "DisplayContextManager: DPI adjustment -" 
             << "Source:" << geometry 
             << "Target:" << adjustedGeometry
             << "Scale:" << scaleFactor;
    
    return adjustedGeometry;
}

int DisplayContextManager::findBestDisplayForGeometry(const QRect& geometry)
{
    QList<QScreen*> screens = QApplication::screens();
    if (screens.isEmpty()) {
        return -1;
    }
    
    int bestDisplay = -1;
    int maxOverlapArea = 0;
    
    for (int i = 0; i < screens.size(); ++i) {
        QScreen* screen = screens[i];
        if (!screen) continue;
        
        QRect screenGeometry = screen->availableGeometry();
        QRect intersection = geometry.intersected(screenGeometry);
        int overlapArea = intersection.width() * intersection.height();
        
        if (overlapArea > maxOverlapArea) {
            maxOverlapArea = overlapArea;
            bestDisplay = i;
        }
    }
    
    qDebug() << "DisplayContextManager: Best display for geometry" << geometry 
             << "is display" << bestDisplay << "with overlap area" << maxOverlapArea;
    
    return bestDisplay;
}

int DisplayContextManager::getDisplayCount()
{
    return QApplication::screens().size();
}

bool DisplayContextManager::hasHighDPIDisplays()
{
    QList<DisplayInfo> displays = getAllDisplays();
    
    for (const DisplayInfo& display : displays) {
        if (display.devicePixelRatio > 1.0) {
            return true;
        }
    }
    
    return false;
}

void DisplayContextManager::cacheDisplayInformation()
{
    qDebug() << "DisplayContextManager: Caching display information";
    
    s_cachedDisplays.clear();
    QList<QScreen*> screens = QApplication::screens();
    
    for (QScreen* screen : screens) {
        if (!screen) continue;
        
        DisplayInfo info;
        info.screen = screen;
        info.availableGeometry = screen->availableGeometry();
        info.devicePixelRatio = screen->devicePixelRatio();
        info.logicalDpi = screen->logicalDotsPerInch();
        
        s_cachedDisplays.append(info);
    }
    
    s_cacheTimestamp = QDateTime::currentDateTime();
    
    qDebug() << "DisplayContextManager: Cached" << s_cachedDisplays.size() << "displays";
}

bool DisplayContextManager::isDisplayInfoCacheValid()
{
    if (s_cachedDisplays.isEmpty()) {
        return false;
    }
    
    QDateTime now = QDateTime::currentDateTime();
    qint64 elapsedMs = s_cacheTimestamp.msecsTo(now);
    
    return elapsedMs < CACHE_VALIDITY_MS;
}

void DisplayContextManager::clearDisplayCache()
{
    qDebug() << "DisplayContextManager: Clearing display cache";
    s_cachedDisplays.clear();
    s_cacheTimestamp = QDateTime();
}