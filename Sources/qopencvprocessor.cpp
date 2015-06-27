/*------------------------------------------------------------------------------------------------------
Taranov Alex, 2014									     SOURCE FILE
Class that wraps opencv functions into Qt SIGNAL/SLOT interface
The simplest way to use it - rewrite appropriate section in QOpencvProcessor::customProcess(...) slot
------------------------------------------------------------------------------------------------------*/

#include "qopencvprocessor.h"

#define OBJECT_MINSIZE 128
#define LEVEL_SHIFT 32
//------------------------------------------------------------------------------------------------------

QOpencvProcessor::QOpencvProcessor(QObject *parent):
    QObject(parent)
{
    //Initialization
    m_cvRect.width = 0;
    m_cvRect.height = 0;
    m_framePeriod = 0.0;
    m_skinFlag = true;   
    v_pixelSet = NULL;
    m_seekCalibColors = false;
    m_calibFlag = false;
    m_blurSize = 4;
    //--------------------
    m_position = 0;
    for(quint16 i = 0; i < FACE_RECT_VECTOR_LENGTH; i++)
    {
        v_X[i] = 0;
        v_Y[i] = 0;
        v_H[i] = 0;
        v_Y[i] = 0;
    }
    //--------------------
    m_framesWNF = 0;
    f_previousFWNF = false;
}

//-----------------------------------------------------------------------------------------------------

cv::Rect QOpencvProcessor::getRect()
{
    return m_cvRect;
}

//-----------------------------------------------------------------------------------------------------

void QOpencvProcessor::setMapRegion(const cv::Rect &input_rect)
{
    m_mapRect = input_rect;
    emit mapRegionUpdated(input_rect);
}

//-----------------------------------------------------------------------------------------------------

void QOpencvProcessor::updateTime()
{
    m_timeCounter = cv::getTickCount();
}

//------------------------------------------------------------------------------------------------------

void QOpencvProcessor::customProcess(const cv::Mat &input)
{
    cv::Mat output(input); // Copy the header and pointer to data of input object
    cv::Mat temp; // temporary object

    //-------------CUSTOM ALGORITHM--------------

    //You can do here almost whatever you wnat...
    cv::cvtColor(output,temp,CV_BGR2GRAY);
    cv::equalizeHist(temp,temp);
    cv::Canny(temp,output,10,100);

    //----------END OF CUSTOM ALGORITHM----------

    //---------Drawing of rectangle--------------
    if( (m_cvRect.width > 0) && (m_cvRect.height > 0) )
    {
        cv::rectangle( output , m_cvRect, cv::Scalar(255,255,255)); // white color
    }

    //-------------Time measurement--------------
    m_framePeriod = (cv::getTickCount() -  m_timeCounter) * 1000.0 / cv::getTickFrequency(); // result is calculated in milliseconds
    m_timeCounter = cv::getTickCount();

    emit frameProcessed(output, m_framePeriod, output.cols*output.rows);
}

//------------------------------------------------------------------------------------------------------

void QOpencvProcessor::setRect(const cv::Rect &input_rect)
{
    m_cvRect = input_rect;
}

//------------------------------------------------------------------------------------------------------

bool QOpencvProcessor::loadClassifier(const std::string &filename)
{
    return m_classifier.load( filename );
}

//------------------------------------------------------------------------------------------------------
void QOpencvProcessor::faceProcess(const cv::Mat &input)
{
    cv::Mat output(input);  // Copy the header and pointer to data of input object
    cv::Mat gray; // Create an instance of cv::Mat for temporary image storage
    cv::cvtColor(output, gray, CV_BGR2GRAY);
    cv::equalizeHist(gray, gray);
    std::vector<cv::Rect> faces_vector;
    m_classifier.detectMultiScale(gray, faces_vector, 1.1, 11, cv::CASCADE_DO_ROUGH_SEARCH|cv::CASCADE_FIND_BIGGEST_OBJECT, cv::Size(OBJECT_MINSIZE, OBJECT_MINSIZE)); // Detect faces (list of flags CASCADE_DO_CANNY_PRUNING, CASCADE_DO_ROUGH_SEARCH, CASCADE_FIND_BIGGEST_OBJECT, CASCADE_SCALE_IMAGE )
    cv::Rect face(0,0,0,0);
    if(faces_vector.size() == 0) {
        if(f_previousFWNF) {
            m_framesWNF++;
        }
        f_previousFWNF = true;
        if(m_framesWNF < FRAMES_WITHOUT_FACES_TRESHOLD) {
            face = getAverageFaceRect();
        }
    } else {
        f_previousFWNF = false;
        m_framesWNF = 0;
        face = enrollFaceRect(faces_vector[0]);
    }
    unsigned int X = face.x; // the top-left corner horizontal coordinate of future rectangle
    unsigned int Y = face.y; // the top-left corner vertical coordinate of future rectangle
    unsigned int rectwidth = face.width; //...
    unsigned int rectheight = face.height; //...
    unsigned int dX = rectwidth/16;
    unsigned int dY = rectheight/30;
    unsigned long red = 0; // an accumulator for red color channel
    unsigned long green = 0; // an accumulator for green color channel
    unsigned long blue = 0; // an accumulator for blue color channel
    unsigned long area = 0;

    if(face.area() > 0)
    {
        cv::Mat blurRegion(output, face);
        cv::blur(blurRegion, blurRegion, cv::Size(m_blurSize, m_blurSize));
        unsigned char *p; // this pointer will be used to store adresses of the image rows
        unsigned char tempBlue;
        unsigned char tempRed;
        unsigned char tempGreen;
        m_ellipsRect = cv::Rect(X + dX , Y-6*dY, rectwidth - 2*dX , rectheight+6*dY);
        if(output.channels() == 3)
        {
            if(m_skinFlag)
            {
                for(unsigned int j = Y - 2*dY; j < Y + rectheight; j++) // it is lucky that unsigned int saves from out of image memory cells processing from image top bound, but not from bottom where you should check this issue explicitly
                {
                    p = output.ptr(j); //takes pointer to beginning of data on row
                    for(unsigned int i = X; i < X + rectwidth; i++)
                    {
                        tempBlue = p[3*i];
                        tempGreen = p[3*i+1];
                        tempRed = p[3*i+2];
                        if( isSkinColor(tempRed, tempGreen, tempBlue) && isInEllips(i,j))
                        {
                            area++;
                            blue += tempBlue;
                            green += tempGreen;
                            red += tempRed;
                            //p[3*i] = 255;
                            //p[3*i+1] %= LEVEL_SHIFT;
                            p[3*i+2] %= LEVEL_SHIFT;
                        }
                    }
                }
            }
        }
    }


    //-----end of if(faces_vector.size() != 0)-----
    m_framePeriod = ((double)cv::getTickCount() -  m_timeCounter)*1000.0 / cv::getTickFrequency();
    m_timeCounter = cv::getTickCount();

    if(area > 0)
    {
        cv::rectangle( output, face , cv::Scalar(255,25,25));
        emit dataCollected( red , green, blue, area, m_framePeriod);
    }
    else
    {
        if(m_classifier.empty())
        {
            emit selectRegion( QT_TRANSLATE_NOOP("QImageWidget", "Load cascade for detection") );
        }
        else
        {
            emit selectRegion( QT_TRANSLATE_NOOP("QImageWidget", "Come closer or change light") );
        }
    }
    emit frameProcessed(output, m_framePeriod, area);
}

//------------------------------------------------------------------------------------------------

void QOpencvProcessor::rectProcess(const cv::Mat &input)
{
    cv::Mat output(input); //Copy constructor
    unsigned int rectwidth = m_cvRect.width;
    unsigned int rectheight = m_cvRect.height;
    unsigned int X = m_cvRect.x;
    unsigned int Y = m_cvRect.y;

    if( (output.rows < (Y + rectheight)) || (output.cols < (X + rectwidth)) )
    {
        rectheight = 0;
        rectwidth = 0;
    }

    unsigned long red = 0;
    unsigned long green = 0;
    unsigned long blue = 0;
    unsigned long area = 0;
    //-------------------------------------------------------------------------
    if((rectheight > 0) && (rectwidth > 0))
    {
        cv::Mat blurRegion(output, m_cvRect);
        cv::blur(blurRegion, blurRegion, cv::Size(m_blurSize, m_blurSize));

        unsigned char *p; // a pointer to store the adresses of image rows
        if(output.channels() == 3)
        {
            if(m_seekCalibColors)
            {
                unsigned char tempRed;
                unsigned char tempGreen;
                unsigned char tempBlue;
                for(unsigned int j = Y; j < Y + rectheight; j++)
                {
                    p = output.ptr(j); //takes pointer to beginning of data on rows
                    for(unsigned int i = X; i < X + rectwidth; i++)
                    {
                        tempBlue = p[3*i];
                        tempGreen = p[3*i+1];
                        tempRed = p[3*i+2];
                        if( isCalibColor(tempGreen) && isSkinColor(tempRed, tempGreen, tempBlue) ) {
                            area++;
                            blue += tempBlue;
                            green += tempGreen;
                            red += tempRed;
                            p[3*i] %= LEVEL_SHIFT;
                            //p[3*i+1] %= LEVEL_SHIFT;
                            p[3*i+2] %= LEVEL_SHIFT;
                        }
                    }
                }
            } else {
            if(m_skinFlag)
            {
                unsigned char tempRed;
                unsigned char tempGreen;
                unsigned char tempBlue;
                for(unsigned int j = Y; j < Y + rectheight; j++)
                {
                    p = output.ptr(j); //takes pointer to beginning of data on rows
                    for(unsigned int i = X; i < X + rectwidth; i++)
                    {
                        tempBlue = p[3*i];
                        tempGreen = p[3*i+1];
                        tempRed = p[3*i+2];
                        if( isSkinColor(tempRed, tempGreen, tempBlue)) {
                            area++;
                            blue += tempBlue;
                            green += tempGreen;
                            red += tempRed;
                            //p[3*i] = 255;
                            //p[3*i+1] %= LEVEL_SHIFT;
                            p[3*i+2] %= LEVEL_SHIFT;
                        }
                    }
                }
            }
            else
            {
                for(unsigned int j = Y; j < Y + rectheight; j++)
                {
                    p = output.ptr(j); //takes pointer to beginning of data on rows
                    for(unsigned int i = X; i < X + rectwidth; i++)
                    {
                        blue += p[3*i];
                        green += p[3*i+1];
                        red += p[3*i+2];
                            //p[3*i] = 255;
                            //p[3*i+1] = 255;
                            //p[3*i+2] = 0;
                    }
                }
                area = rectwidth*rectheight;
            }
            }
        }
        else
        {
            for(unsigned int j = Y; j < Y + rectheight; j++)
            {
                p = output.ptr(j);//pointer to beginning of data on rows
                for(unsigned int i = X; i < X + rectwidth; i++)
                {
                    green += p[i];
                        //p[i] = 0;
                }
            }
            area = rectwidth*rectheight;
        }
    }
    //------end of if((rectheight > 0) && (rectwidth > 0))
    m_framePeriod = ((double)cv::getTickCount() -  m_timeCounter)*1000.0 / cv::getTickFrequency();
    m_timeCounter = cv::getTickCount();
    if( area > 0 )
    {
        cv::rectangle( output , m_cvRect, cv::Scalar(255,25,25));
        emit dataCollected(red, green, blue, area, m_framePeriod);      
        if(m_calibFlag)
        {
            v_calibValues[m_calibSamples] = (qreal)green/area;
            m_calibMean += v_calibValues[m_calibSamples];
            m_calibSamples++;
            if(m_calibSamples == CALIBRATION_VECTOR_LENGTH)
            {
                m_calibMean /= CALIBRATION_VECTOR_LENGTH;
                m_calibError = 0.0;
                for(quint16 i = 0; i < CALIBRATION_VECTOR_LENGTH; i++)
                {
                    m_calibError += (v_calibValues[i] - m_calibMean)*(v_calibValues[i] - m_calibMean);
                }
                m_calibError = 10 * sqrt( m_calibError /(CALIBRATION_VECTOR_LENGTH - 1) );
                qWarning("mean: %f; error: %f; samples: %d", m_calibMean,m_calibError, m_calibSamples);
                m_calibSamples = 0;
                m_calibFlag = false;
                m_seekCalibColors = true;
                emit calibrationDone(m_calibMean, m_calibError/10, m_calibSamples);
            }
        }
    }
    else
    {
        emit selectRegion( QT_TRANSLATE_NOOP("QImageWidget", "Select region on image" ) );
    }
    emit frameProcessed(output, m_framePeriod, area);
}

//-----------------------------------------------------------------------------------------------

void QOpencvProcessor::mapProcess(const cv::Mat &input)
{
    cv::Mat output(input);
    int X = m_mapRect.x;
    int Y = m_mapRect.y;
    int W = m_mapRect.width;
    int H = m_mapRect.height;

    if((output.cols >= (X+W)) && (output.rows >= (Y+H)))
    {
        int stepsY = H / m_mapCellSizeY;
        int stepsX = W / m_mapCellSizeX;
        int area = m_mapCellSizeY*m_mapCellSizeX;

        unsigned long sumRed = 0;
        unsigned long sumBlue = 0;
        unsigned long sumGreen = 0;

        int performance_pill;

        if(input.channels() == 3)
        {
            for(int i = 0; i < stepsY; i++)
            {
                for(int p = 0; p < m_mapCellSizeY; p++)
                {
                    v_pixelSet[p] = output.ptr(Y + i*m_mapCellSizeY + p);
                }
                for(int j = 0; j < stepsX; j++)
                {
                    for(int k = 0; k < m_mapCellSizeX; k++)
                    {
                        performance_pill = 3*(X + j*m_mapCellSizeX + k);
                        for(int p = 0; p < m_mapCellSizeY; p++)
                        {
                            sumBlue += v_pixelSet[p][performance_pill];
                            sumGreen += v_pixelSet[p][performance_pill + 1];
                            sumRed += v_pixelSet[p][performance_pill + 2];
                        }
                    }
                    emit mapCellProcessed(sumRed, sumGreen, sumBlue, area, m_framePeriod);
                    sumBlue = 0;
                    sumGreen = 0;
                    sumRed = 0;
                }
            }
        }
        else
        {
            for(int i = 0; i < stepsY; i++)
            {
                for(int p = 0; p < m_mapCellSizeY; p++)
                {
                    v_pixelSet[p] = output.ptr(Y + i*m_mapCellSizeY + p);
                }
                for(int j = 0; j < stepsX; j++)
                {
                    for(int k = 0; k < m_mapCellSizeX; k++)
                    {
                        performance_pill = X + j*m_mapCellSizeX + k;
                        for(int p = 0; p < m_mapCellSizeY; p++)
                        {
                            sumBlue += v_pixelSet[p][performance_pill];
                        }
                    }
                    emit mapCellProcessed(sumBlue, sumBlue, sumBlue, area, m_framePeriod);
                    sumBlue = 0;
                }
            }
        }
    }
}

void QOpencvProcessor::setMapCellSize(quint16 sizeX, quint16 sizeY)
{
    m_mapCellSizeX = sizeX;
    m_mapCellSizeY = sizeY;
    if(v_pixelSet)
    {
        delete[] v_pixelSet;
        v_pixelSet = NULL;
    }
    v_pixelSet = new unsigned char*[m_mapCellSizeY];
}

void QOpencvProcessor::setSkinSearchingFlag(bool value)
{
    m_skinFlag = value;
}

void QOpencvProcessor::calibrate(bool value)
{
    if(!value) {
        m_seekCalibColors = false;
        return;
    } else {
        m_calibFlag = true;
        m_calibSamples = 0;
        m_calibMean = 0.0;
    }
}

void QOpencvProcessor::setBlurSize(int size)
{
    if(size > 1)
    {
        m_blurSize = size;
    }
}

cv::Rect QOpencvProcessor::enrollFaceRect(const cv::Rect &rect)
{
    v_X[m_position] = rect.x;
    v_Y[m_position] = rect.y;
    v_H[m_position] = rect.height;
    v_W[m_position] = rect.width;
    m_position = (++m_position) % FACE_RECT_VECTOR_LENGTH;
    return getAverageFaceRect();
}

cv::Rect QOpencvProcessor::getAverageFaceRect() const
{
    qreal x = 0.0;
    qreal y = 0.0;
    qreal w = 0.0;
    qreal h = 0.0;
    for(quint16 i = 0; i < FACE_RECT_VECTOR_LENGTH; i++)
    {
        x += v_X[i];
        y += v_Y[i];
        w += v_W[i];
        h += v_H[i];
    }
    x /= FACE_RECT_VECTOR_LENGTH;
    y /= FACE_RECT_VECTOR_LENGTH;
    w /= FACE_RECT_VECTOR_LENGTH;
    h /= FACE_RECT_VECTOR_LENGTH;
    return cv::Rect(x,y,w,h);
}
