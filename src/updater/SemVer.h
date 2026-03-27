#ifndef DBLSQD_SEMVER_H
#define DBLSQD_SEMVER_H

#include <QString>

namespace dblsqd {

class SemVer
{
public:
    SemVer(const QString& version);

    bool operator<(const SemVer& other) const;

    bool isValid() const;

private:
    QString mOriginal;
    int mMajor{0};
    int mMinor{0};
    int mPatch{0};
    QString mPrerelease;
    QString mBuild;
    bool mValid{false};

    static QString getRegExp();
};

} // namespace dblsqd

#endif // DBLSQD_SEMVER_H
