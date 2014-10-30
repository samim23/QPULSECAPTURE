/*--------------------------------------------------------------------------------------------
Taranov Alex, 2014                              		               CPP SOURCE FILE
The Develnoter [internet], Marcelo Mottalli, 2014
This class is a descendant of QWidget with QImageWidget::updateImage(...) slot that constructs
QImage instance from cv::Mat image. The QImageWidget should be used as widget for video display  
 * ------------------------------------------------------------------------------------------*/

#include "qimagewidget.h"

//-----------------------------------------------------------------------------------

QImageWidget::QImageWidget(QWidget *parent): QWidget(parent)
{
    pt_data = NULL;
    m_margin = 10;
}

//-----------------------------------------------------------------------------------

void QImageWidget::updateImage(const cv::Mat& image, qreal frame_period)
{
    m_informationString = QString::number(frame_period, 'f', 1) + " ms, " + QString::number(image.cols) + "x" + QString::number(image.rows);

    switch ( image.type() )
    {
        case CV_8UC1:
            cv::cvtColor(image, opencv_image, CV_GRAY2RGB);
            break;

        case CV_8UC3:
            cv::cvtColor(image, opencv_image, CV_BGR2RGB);
            break;
    }
    assert(opencv_image.isContinuous()); // QImage needs the data to be stored continuously in memory
    qt_image = QImage(opencv_image.data, opencv_image.cols, opencv_image.rows, opencv_image.cols * 3, QImage::Format_RGB888);  // Assign OpenCV's image buffer to the QImage
    update();
}

//------------------------------------------------------------------------------------

void QImageWidget::paintEvent(QPaintEvent* )
{
    QPainter painter( this );
    painter.setRenderHint(QPainter::Antialiasing);
    QRect temp_rect = make_proportional_rect(this->rect(), opencv_image.cols, opencv_image.rows);
    painter.drawImage( temp_rect, qt_image); // Draw inside widget, the image is scaled to fit the rectangle   
    drawStrings(painter, temp_rect);          // Will draw m_informationString on the widget


}

//------------------------------------------------------------------------------------

void QImageWidget::mousePressEvent(QMouseEvent *event)
{
    x0 = event->x();
    y0 = event->y();
    m_aimrect.setX( x0 );
    m_aimrect.setY( y0 );
}

void QImageWidget::mouseMoveEvent(QMouseEvent *event)
{
    if( event->x() > x0)
    {
        m_aimrect.setWidth(event->x() - x0);
    }
    else
    {
        m_aimrect.setX( event->x() );
        m_aimrect.setWidth( x0 - event->x() );
    }
    if( event->y() > y0)
    {
        m_aimrect.setHeight(event->y() - y0);
    }
    else
    {
        m_aimrect.setY( event->y() );
        m_aimrect.setHeight( y0 - event->y() );
    }
    emit rect_was_entered( crop_aimrect() );
}

//------------------------------------------------------------------------------------

void QImageWidget::drawStrings(QPainter &painter, const QRect &input_rect)
{
        if(!m_informationString.isEmpty())
        {
            qreal startX = input_rect.x() + m_margin;
            qreal startY = input_rect.y();
            qreal pointsize = (qreal)input_rect.height()/28;

            QPen pen(Qt::NoBrush, 1.0, Qt::SolidLine);
            pen.setColor(Qt::black);
            painter.setPen( pen );
            painter.setBrush(Qt::white);
            QFont font("Calibri", pointsize, QFont::Black);
            font.setItalic(true);
            QPainterPath path;
            path.addText( startX, startY + input_rect.height() - m_margin, font, m_informationString); // Draws resolution and frametime

            if(!m_warningString.isEmpty())
            {
                font.setPointSizeF( pointsize * 2);
                qreal pX = input_rect.x() + (input_rect.width() - m_warningString.size()*pointsize - 2*m_margin) / 2.0;
                qreal pY = input_rect.y() + font.pointSizeF() + (input_rect.height() - font.pointSizeF() ) / 2.0;

                QPainterPath path_warning;
                path_warning.addText(pX, pY, font, m_warningString);
                painter.setBrush(Qt::red);
                painter.drawPath(path_warning);
                painter.setBrush(Qt::white);
                m_warningString.clear();
            }
            else
            {
                QPainterPath path_freq;
                font.setPointSizeF( pointsize * 3 );                          
                startY += font.pointSize() + m_margin;
                path_freq.addText(startX, startY, font, m_frequencyString); // Draws frequency value
                painter.setBrush(m_frequencyColor);
                painter.drawPath(path_freq);

                painter.setBrush(Qt::white);
                font.setPointSizeF( pointsize );
                if(m_frequencyString.isEmpty())
                {
                    path.addText(startX, startY, font ,"Unreliable");
                }
                else
                {
                    path.addText(startX + m_frequencyString.size() * pointsize * 2.25, startY, font ,"bpm");
                }
                path.addText(startX, startY + pointsize*1.5, font, m_snrString);
            }
            painter.drawPath(path);
        }
}

//-----------------------------------------------------------------------------------

void QImageWidget::updateValues(qreal value1, qreal value2, bool flag) // value1 for HRfrequency and value2 for SNR
{
    if(flag)
    {
        m_frequencyColor = QColor(Qt::green);
    }
    else
    {
        m_frequencyColor = QColor(Qt::red);
    }
    m_frequencyString = QString::number(value1, 'f', 0);
    m_snrString = "SNR: " +QString::number(value2,'f',2) + " dB";
}

//-----------------------------------------------------------------------------------

void QImageWidget::set_warning_status(const char * input_string)
{
    m_frequencyString.clear();
    m_warningString = QString( input_string );
}

//----------------------------------------------------------------------------------

void QImageWidget::clearFrequencyString(qreal value)
{
    m_frequencyString.clear();
    m_snrString = "SNR: " + QString::number(value,'f',2) + " dB";
}
