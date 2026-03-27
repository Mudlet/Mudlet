#ifndef DBLSQD_SEMVER_H
#define DBLSQD_SEMVER_H

#include <QString>

namespace dblsqd {

class SemVer
{
public:
    SemVer(QString version);

    bool operator<(const SemVer& other) const;

    bool isValid() const;

private:
    QString mOriginal;
    int mMajor;
    int mMinor;
    int mPatch;
    QString mPrerelease;
    QString mBuild;
    bool mValid;

    static QString getRegExp();
};

} // namespace dblsqd

#endif // DBLSQD_SEMVER_H
