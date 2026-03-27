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
    QString original;
    int major;
    int minor;
    int patch;
    QString prerelease;
    QString build;
    bool valid;

    static QString getRegExp();
};

} // namespace dblsqd

#endif // DBLSQD_SEMVER_H
