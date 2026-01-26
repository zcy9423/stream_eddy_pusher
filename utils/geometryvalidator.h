#ifndef GEOMETRYVALIDATOR_H
#define GEOMETRYVALIDATOR_H

#include <QRect>
#include <QSize>
#include <QMargins>
#include <QString>

class QScreen;

/**
 * @brief Geometry Validator
 * 
 * Validates and adjusts window geometry to prevent Qt geometry errors
 * while respecting display boundaries and frame margins.
 */
class GeometryValidator
{
public:
    /**
     * @brief Result of geometry validation
     */
    struct ValidationResult {
        QRect adjustedGeometry;     ///< Final adjusted geometry
        bool wasAdjusted;          ///< Whether geometry was modified
        QString adjustmentReason;   ///< Reason for adjustment
        
        ValidationResult() : wasAdjusted(false) {}
    };
    
    /**
     * @brief Validate and adjust geometry for display bounds
     * @param requestedGeometry Original geometry request
     * @param minimumSize Minimum allowed window size
     * @param targetDisplay Target display index (-1 for auto-detect)
     * @return Validation result with adjusted geometry
     */
    static ValidationResult validateGeometry(const QRect& requestedGeometry, 
                                           const QSize& minimumSize,
                                           int targetDisplay = -1);
    
    /**
     * @brief Adjust geometry to fit within display bounds
     * @param geometry Original geometry
     * @param screen Target screen
     * @return Adjusted geometry that fits within screen
     */
    static QRect adjustForDisplayBounds(const QRect& geometry, const QScreen* screen);
    
    /**
     * @brief Adjust geometry to account for window frame margins
     * @param geometry Client area geometry
     * @param frameMargins Estimated frame margins
     * @return Adjusted geometry including frame
     */
    static QRect adjustForFrameMargins(const QRect& geometry, const QMargins& frameMargins);
    
    /**
     * @brief Check if geometry is valid for the given screen
     * @param geometry Geometry to check
     * @param screen Target screen
     * @return true if geometry is valid
     */
    static bool isGeometryValid(const QRect& geometry, const QScreen* screen);
    
private:
    /**
     * @brief Estimate window frame margins for current platform
     * @return Estimated frame margins
     */
    static QMargins estimateFrameMargins();
};

#endif // GEOMETRYVALIDATOR_H