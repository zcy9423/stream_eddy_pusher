#ifndef DISPLAYCONTEXT_H
#define DISPLAYCONTEXT_H

#include <QRect>
#include <QList>
#include <QDateTime>

class QScreen;

/**
 * @brief Display Context Manager
 * 
 * Handles multi-monitor awareness and DPI scaling for consistent window display
 * across different display configurations.
 */
class DisplayContextManager
{
public:
    /**
     * @brief Information about a display
     */
    struct DisplayInfo {
        QScreen* screen;              ///< Screen object
        QRect availableGeometry;      ///< Available geometry (excluding taskbars)
        qreal devicePixelRatio;       ///< Device pixel ratio for HiDPI
        int logicalDpi;              ///< Logical DPI
        
        DisplayInfo() : screen(nullptr), devicePixelRatio(1.0), logicalDpi(96) {}
    };
    
    /**
     * @brief Get information about the primary display
     * @return Primary display information
     */
    static DisplayInfo getPrimaryDisplayInfo();
    
    /**
     * @brief Get information about a specific display
     * @param displayIndex Display index (0-based)
     * @return Display information, or primary display if index invalid
     */
    static DisplayInfo getDisplayInfo(int displayIndex);
    
    /**
     * @brief Get information about all available displays
     * @return List of all display information
     */
    static QList<DisplayInfo> getAllDisplays();
    
    /**
     * @brief Adjust geometry for DPI scaling
     * @param geometry Original geometry
     * @param sourceDPI Source DPI
     * @param targetDPI Target DPI
     * @return DPI-adjusted geometry
     */
    static QRect adjustForDPI(const QRect& geometry, qreal sourceDPI, qreal targetDPI);
    
    /**
     * @brief Find the best display for given geometry
     * @param geometry Window geometry
     * @return Index of best display, or -1 for primary
     */
    static int findBestDisplayForGeometry(const QRect& geometry);
    
    /**
     * @brief Get the number of available displays
     * @return Number of displays
     */
    static int getDisplayCount();
    
    /**
     * @brief Check if system has high-DPI displays
     * @return true if any display has DPI > 1.0
     */
    static bool hasHighDPIDisplays();
    
private:
    /**
     * @brief Cache display information for performance
     */
    static void cacheDisplayInformation();
    
    /**
     * @brief Check if cached display information is still valid
     * @return true if cache is valid
     */
    static bool isDisplayInfoCacheValid();
    
    /**
     * @brief Clear cached display information
     */
    static void clearDisplayCache();
    
    // Cache management
    static QList<DisplayInfo> s_cachedDisplays;
    static QDateTime s_cacheTimestamp;
    static const int CACHE_VALIDITY_MS = 5000; // 5 seconds
};

#endif // DISPLAYCONTEXT_H