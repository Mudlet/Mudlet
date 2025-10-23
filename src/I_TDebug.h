#ifndef I_TDEBUG_H
#define I_TDEBUG_H

class I_Host; 

class I_TDebug {
public:
    virtual ~I_TDebug() = default;

    virtual I_TDebug& operator<<(const QString& text) = 0;
    virtual I_TDebug& operator<<(const QChar& ch) = 0;
    virtual I_TDebug& operator<<(int value) = 0;
    virtual I_TDebug& operator>>(I_Host* host) = 0;
};

#endif
