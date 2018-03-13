#ifndef SENSORFILTER_H
#define SENSORFILTER_H

#include <QObject>
#include <QVector>

class SensorFilter : public QObject
{
    Q_OBJECT
public:
    enum Type {
        TypeLowPass,
        TypeHighPass,
        TypeAverage
    };
    Q_ENUM(Type)

    explicit SensorFilter(Type filterType, QObject *parent = nullptr);

    double filterValue(double value);

    bool isReady() const;
    void reset();

    Type filterType() const;

    QVector<double> inputData() const;
    QVector<double> outputData() const;

    // Filter configuration
    uint windowSize() const;
    void setFilterWindowSize(uint windowSize = 20);

    double lowPassAlpha() const;
    void setLowPassAlpha(double alpha = 0.2);

    double highPassAlpha() const;
    void setHighPassAlpha(double alpha = 0.2);

private:
    Type m_filterType = TypeLowPass;
    int m_filterWindowSize = 20;
    double m_lowPassAlpha = 0.2;
    double m_highPassAlpha = 0.2;

    double m_averageSum = 0;

    QVector<double> m_inputData;
    QVector<double> m_outputData;

    void addInputValue(double value);

    // Filter methods
    double lowPassFilterValue(double value);
    double highPassFilterValue(double value);
    double averageFilterValue(double value);

};

#endif // SENSORFILTER_H
