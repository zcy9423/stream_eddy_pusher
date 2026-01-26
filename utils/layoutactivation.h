#ifndef LAYOUTACTIVATION_H
#define LAYOUTACTIVATION_H

#include <QWidget>

/**
 * @brief Layout Activation Controller
 * 
 * Controls the timing and sequence of layout activation to ensure proper
 * component alignment and prevent layout misalignment issues.
 */
class LayoutActivationController
{
public:
    /**
     * @brief Timing options for layout activation
     */
    enum class ActivationTiming {
        Immediate,          ///< Activate layouts immediately
        Deferred,          ///< Defer activation to next event loop
        OnNextEventLoop    ///< Schedule activation for next event loop iteration
    };
    
    /**
     * @brief Activate layouts with specified timing
     * @param widget Target widget to activate layouts for
     * @param timing Activation timing strategy
     */
    static void activateLayouts(QWidget* widget, ActivationTiming timing = ActivationTiming::OnNextEventLoop);
    
    /**
     * @brief Schedule layout activation with delay
     * @param widget Target widget
     * @param delayMs Delay in milliseconds (0 for next event loop)
     */
    static void scheduleLayoutActivation(QWidget* widget, int delayMs = 0);
    
    /**
     * @brief Check if layout activation is complete
     * @param widget Widget to check
     * @return true if layout activation is complete
     */
    static bool isLayoutActivationComplete(QWidget* widget);
    
    /**
     * @brief Validate layout geometry after activation
     * @param widget Widget to validate
     * @return true if layout geometry is valid
     */
    static bool validateLayoutGeometry(QWidget* widget);
    
private:
    /**
     * @brief Perform the actual layout activation
     * @param widget Target widget
     */
    static void performLayoutActivation(QWidget* widget);
};

#endif // LAYOUTACTIVATION_H