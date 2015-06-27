/*------------------------------------------------------------------------------------------------------
Taranov Alex, 2014									     HEADER FILE
Class that wraps opencv functions into Qt SIGNAL/SLOT interface
The simplest way to use it - rewrite appropriate section in QOpencvProcessor::customProcess(...) slot
------------------------------------------------------------------------------------------------------*/

#ifndef QOPENCVPROCESSOR_H
#define QOPENCVPROCESSOR_H
//------------------------------------------------------------------------------------------------------

#include <QObject>
#include <opencv2/opencv.hpp>

#define CALIBRATION_VECTOR_LENGTH 25
#define FACE_RECT_VECTOR_LENGTH 9
#define FRAMES_WITHOUT_FACES_TRESHOLD 5

//------------------------------------------------------------------------------------------------------

class QOpencvProcessor : public QObject
{
    Q_OBJECT
public:
    explicit QOpencvProcessor(QObject *parent = 0);

signals:
    void frameProcessed(const cv::Mat& value, double frame_period, quint32 pixels_enrolled); //should be emited in the end of each frame processing
    void dataCollected(unsigned long red, unsigned long green, unsigned long blue, unsigned long area, double period);
    void selectRegion(const char * string);     // emit it if no objects has been detected or no regions are selected
    void mapCellProcessed(unsigned long red, unsigned long green, unsigned long blue, unsigned long area, double period);
    void mapRegionUpdated(const cv::Rect& rect);
    void calibrationDone(qreal mean, qreal stdev, quint16 samples);

public slots:
    void customProcess(const cv::Mat &input);   // just a template of how a program logic should work
    void updateTime();                          // use it in the beginning of any time-measurement operations
    void setRect(const cv::Rect &input_rect);   // sets m_cvrect
    void faceProcess(const cv::Mat &input);     // an algorithm that evaluates PPG from skin region, region evaluates by means of opencv's cascadeclassifier functions
    void rectProcess(const cv::Mat &input);     // an algorithm that evaluates PPG from skin region defined by user
    bool loadClassifier(const std::string& filename); // an interface to CascadeClassifier::load(...) function
    void mapProcess(const cv::Mat &input);
    void calibrate(bool value);
    void setBlurSize(int size);

    cv::Rect getRect(); // returns current m_cvRect
    void setMapRegion(const cv::Rect &input_rect); // sets up map region, see m_mapRect
    void setMapCellSize(quint16 sizeX, quint16 sizeY);
    void setSkinSearchingFlag(bool value);

private:
    bool m_skinFlag;
    int64 m_timeCounter;    // stores time of application/computer start
    double m_framePeriod;   // stores time of frame processing
    cv::Rect m_cvRect;      // this rect is used by process_rectregion_pulse slot
    cv::CascadeClassifier m_classifier; //object that manages opencv's image recognition functions

    quint16 m_mapCellSizeX;
    quint16 m_mapCellSizeY;
    cv::Rect m_mapRect;
    unsigned char **v_pixelSet; // memory should be allocated in setMapCellSize() call

    bool m_calibFlag;
    bool m_seekCalibColors;
    quint16 m_calibSamples;
    qreal m_calibMean;
    qreal m_calibError;
    quint8 v_calibValues[CALIBRATION_VECTOR_LENGTH];

    bool isSkinColor(unsigned char valueRed, unsigned char valueGreen, unsigned char valueBlue);
    bool isCalibColor(unsigned char value);
    bool isInEllips(unsigned int X, unsigned int Y);
    cv::Rect m_ellipsRect;

    int m_blurSize;

    quint16 m_position;
    int v_X[FACE_RECT_VECTOR_LENGTH];
    int v_Y[FACE_RECT_VECTOR_LENGTH];
    int v_W[FACE_RECT_VECTOR_LENGTH];
    int v_H[FACE_RECT_VECTOR_LENGTH];
    quint16 loop(qint16 value);
    cv::Rect enrollFaceRect(const cv::Rect &rect);
    cv::Rect getAverageFaceRect() const;

    quint8 m_framesWNF; // frames with no faces in a row
    bool f_previousFWNF;

};

inline bool QOpencvProcessor::isSkinColor(unsigned char valueRed, unsigned char valueGreen, unsigned char valueBlue)
{
    //from Ghazali Osman Muhammad, Suzuri Hitam and Mohd Nasir Ismail
    //"ENHANCED SKIN COLOUR CLASSIFIER USING RGB RATIO MODEL" International Journal on Soft Computing (IJSC) Vol.3, No.4, November 2012
    //Swift’s rule
    /*if( (valueBlue > valueRed)    &&
        (valueGreen < valueBlue)    &&
        (valueGreen > valueRed)     &&
        ( (valueBlue < (valueRed >> 2))  || (valueBlue > 200) ) ) {
        return false;
    } else return true;*/

    //Kovac's rule
    /*if( (valueRed > 95) && (valueRed > valueGreen)    &&
        (valueGreen > 40) && (valueBlue > 20)           &&
        ((valueRed - qMin(valueGreen,valueBlue)) > 15)  &&
        ((valueRed - valueGreen) > 15 ) ) {
        return true;
    } else return false;*/

    //Modified Kovac's rule
    if( (valueRed > 115) &&
        (valueRed > valueGreen) && (valueBlue > 45)     &&
        ((valueRed - qMin(valueGreen,valueBlue)) > 35)  &&
        ((valueRed - valueGreen) > 25 ) ) {
        return true;
    } else return false;
}

inline bool QOpencvProcessor::isCalibColor(unsigned char value)
{
    if( ((qreal)value < (m_calibMean + m_calibError)) && ((qreal)value > (m_calibMean - m_calibError))) {
        return true;
    } else {
        return false;
    }
}

inline bool QOpencvProcessor::isInEllips(unsigned int X, unsigned int Y)
{
    qreal cX = ((qreal)X - m_ellipsRect.x - m_ellipsRect.width / 2.0) / (m_ellipsRect.width / 2.0) ;
    qreal cY = ((qreal)Y - m_ellipsRect.y - m_ellipsRect.height / 2.0) / (m_ellipsRect.height / 2.0);

    if(  (cX*cX + cY*cY) < 1.0 ) {
        return true;
    } else {
        return false;
    }
}

inline quint16 QOpencvProcessor::loop(qint16 value)
{
    return  (FACE_RECT_VECTOR_LENGTH + (value % FACE_RECT_VECTOR_LENGTH)) % FACE_RECT_VECTOR_LENGTH;
}


//------------------------------------------------------------------------------------------------------
#endif // QOPENCVPROCESSOR_H
