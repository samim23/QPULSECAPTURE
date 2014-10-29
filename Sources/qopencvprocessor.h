/*------------------------------------------------------------------------------------------------------
Taranov Alex, 2014									     HEADER FILE
Class that wraps opencv functions into Qt SIGNAL/SLOT interface
The simplest way to use it - rewrite appropriate section in QOpencvProcessor::custom_algorithm(...) slot
------------------------------------------------------------------------------------------------------*/

#ifndef QOPENCVPROCESSOR_H
#define QOPENCVPROCESSOR_H
//------------------------------------------------------------------------------------------------------

#include <QObject>
#include <opencv2/opencv.hpp>

//------------------------------------------------------------------------------------------------------

class QOpencvProcessor : public QObject
{
    Q_OBJECT
public:
    explicit QOpencvProcessor(QObject *parent = 0);

signals:
    void frameProcessed(const cv::Mat& value, double frame_period); //should be emited in the end of each frame processing
    void colorsEvaluated(unsigned long red, unsigned long green, unsigned long blue, unsigned long area, double period);
    void selectRegion(const char * string); // emit it if no objects has been detected or no regions are selected

public slots:
    void updateClock();                      // use it in the beginning of any time-measurement operations
    void updateRect(const cv::Rect &input_rect);       // sets m_cvrec
    void processRegion(const cv::Mat &input); // an algorithm that evaluates PPG from skin region defined by user

private:
    int64 m_timeCounter;    // stores time of application/computer start
    double m_framePeriod;   // stores time of frame processing
    cv::Rect m_cvRect;      // this rect is used by process_rectregion_pulse slot
};

//------------------------------------------------------------------------------------------------------
#endif // QOPENCVPROCESSOR_H
