#ifndef WINDOWINITIALIZATION_H
#define WINDOWINITIALIZATION_H

#include <QMainWindow>
#include <QSize>
#include <QRect>
#include <QString>

/**
 * @brief Window Initialization Manager
 * 
 * Manages the complete window initialization sequence to prevent geometry errors
 * and layout misalignment. Handles proper timing of geometry setting and window display.
 */
class WindowInitializationManager
{
public:
    /**
     * @brief Configuration for window initialization
     */
    struct InitializationConfig {
        QSize preferredSize;      ///< Preferred window size
        QSize minimumSize;        ///< Minimum allowed window size
        bool centerOnScreen;      ///< Whether to center window on screen
        int displayIndex;         ///< Target display index (-1 for primary)
        
        InitializationConfig() 
            : preferredSize(1600, 1000)
            , minimumSize(1200, 800)
            , centerOnScreen(true)
            , displayIndex(-1) {}
    };
    
    /**
     * @brief Initialize window with proper geometry and timing
     * @param window Target window to initialize
     * @param config Initialization configuration
     */
    static void initializeWindow(QMainWindow* window, const InitializationConfig& config);
    
    /**
     * @brief Prepare window geometry before showing
     * @param window Target window
     * @param size Desired window size
     */
    static void prepareGeometry(QMainWindow* window, const QSize& size);
    
    /**
     * @brief Validate that geometry fits within display bounds
     * @param geometry Geometry to validate
     * @param displayIndex Target display index (-1 for auto-detect)
     * @return true if geometry is valid
     */
    static bool validateDisplayBounds(const QRect& geometry, int displayIndex = -1);
    
private:
    /**
     * @brief Calculate optimal geometry for given size and display
     * @param size Desired window size
     * @param displayIndex Target display index
     * @return Calculated geometry rectangle
     */
    static QRect calculateOptimalGeometry(const QSize& size, int displayIndex);
    
    /**
     * @brief Apply geometry with validation and error handling
     * @param window Target window
     * @param geometry Geometry to apply
     */
    static void applyGeometryWithValidation(QMainWindow* window, const QRect& geometry);
};

#endif // WINDOWINITIALIZATION_H