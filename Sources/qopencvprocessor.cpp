/*------------------------------------------------------------------------------------------------------
Taranov Alex, 2014									     SOURCE FILE
Class that wraps opencv functions into Qt SIGNAL/SLOT interface
The simplest way to use it - rewrite appropriate section in QOpencvProcessor::custom_algorithm(...) slot
------------------------------------------------------------------------------------------------------*/

#include "qopencvprocessor.h"

//------------------------------------------------------------------------------------------------------

QOpencvProcessor::QOpencvProcessor(QObject *parent):
    QObject(parent)
{
    //Initialization
    m_cvRect.width = 0;
    m_cvRect.height = 0;
    m_framePeriod = 0.0;
}

//-----------------------------------------------------------------------------------------------------

void QOpencvProcessor::updateClock()
{
    m_timeCounter = cv::getTickCount();
}

//------------------------------------------------------------------------------------------------------

void QOpencvProcessor::updateRect(const cv::Rect &input_rect)
{
     m_cvRect = input_rect;
}

//------------------------------------------------------------------------------------------------------

void QOpencvProcessor::processRegion(const cv::Mat &input)
{
    cv::Mat output(input); //Copy constructor
    long rectwidth = m_cvRect.width;
    long rectheight = m_cvRect.height;
    long X = m_cvRect.x;
    long Y = m_cvRect.y;

    if( (output.rows <= (Y + rectheight)) || (output.cols <= (X + rectwidth)) )
    {
        rectheight = 0;
        rectwidth = 0;
    }

    unsigned long red = 0;
    unsigned long green = 0;
    unsigned long blue = 0;
    //-------------------------------------------------------------------------
    if((rectheight > 0) && (rectwidth > 0))
    {
        unsigned char *p; // a pointer to store the adresses of image rows
        if(output.channels() == 3)
        {
            for( int j = Y; j < Y + rectheight; j++)
            {
                p = output.ptr(j); //takes pointer to beginning of data on rows
                for( int i = X; i < X + rectwidth; i++)
                {
                    blue += p[3*i];
                    green += p[3*i+1];
                    red += p[3*i+2];
                }
            }
        }
        else
        {
            for( int j = Y; j < Y + rectheight; j++)
            {
                p = output.ptr(j);//pointer to beginning of data on rows
                for( int i = X; i < X + rectwidth; i++)
                {
                    green += p[i];
                }
            }
        }
    }
    //------end of if((rectheight > 0) && (rectwidth > 0))
    m_framePeriod = ((double)cv::getTickCount() -  m_timeCounter)*1000.0 / cv::getTickFrequency();
    m_timeCounter = cv::getTickCount();
    if((rectheight > 0) && (rectwidth > 0))
    {
        cv::rectangle( output , m_cvRect, cv::Scalar(15,250,15),1);
        emit colorsEvaluated(red, green, blue, rectwidth*rectheight, m_framePeriod);
    }
    else
    {
        emit selectRegion("Select region on image");
    }
    emit frameProcessed(output, m_framePeriod);
}

//-----------------------------------------------------------------------------------------------
